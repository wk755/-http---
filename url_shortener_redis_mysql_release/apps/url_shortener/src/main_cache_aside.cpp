#include "http/HttpServer.h"
#include "router/Router.h"
#include "router/RouterHandler.h"
#include "middleware/Middleware.h"
#include "middleware/MiddlewareChain.h"
#include "middleware/cors/CorsMiddleware.h"

#include "http_cache/CachePolicy.h"
#include "http_cache/MemoryCacheLRU.h"

#include "app/RouterSetup.h"
#include "app/Metrics.h"
#include "app/Bloom.h"
#include "app/KVStore.h"
#include "app/IStore.h"
#include "app/MemoryStore.h"
#include "app/CacheAdapter.h"
#include "app/mw_RateLimit.h"
#include "app/mw_Blacklist.h"
#include "app/mw_RateLimitRedis.h"

#include "app/CacheAsideStore.h"

#include <memory>
#include <iostream>
#include <cstdlib>

#ifdef WITH_REDIS
#include "app/RedisClient.h"
#include "app/RedisStore.h"
#endif

#ifdef WITH_MYSQL
#include "app/MySqlStore.h"
#endif

using namespace http;

int main(int argc, char** argv){
  int port = 8080; if (argc > 1) port = std::stoi(argv[1]);
  HttpServer server(port, "url_shortener_cache_aside", /*useSSL*/false);
  server.setThreadNum(4);

  app::Metrics metrics;
  app::Bloom    bloom(1<<20);
  const int SHARDS = 8;
  app::KVStore kv(SHARDS);

  // ========== Backend selection (Redis + MySQL 优先; 否则降级) ==========
  std::unique_ptr<app::IStore> storePtr;

  // MySQL 回填到 Redis 的短 TTL
  int backfillTtl = 60; if (const char* env = std::getenv("CACHE_BACKFILL_TTL")) {
    int v = std::atoi(env); if (v>=0 && v<=3600) backfillTtl = v;
  }

#ifdef WITH_REDIS
  app::RedisClient redis;
  std::unique_ptr<app::RedisStore> redisStore;
  bool redis_ok = false;
  try {
    const char* rh = getenv("REDIS_HOST"); if (!rh) rh = "127.0.0.1";
    const char* rp = getenv("REDIS_PORT"); int rport = rp? std::atoi(rp) : 6379;
    redis.connect(rh, rport, 200);
    redisStore.reset(new app::RedisStore(&redis, "short:"));
    std::cout << "[init] Redis connected at " << rh << ":" << rport << std::endl;
    redis_ok = true;
  } catch (const std::exception& e){
    std::cerr << "[init] Redis disabled: " << e.what() << std::endl;
  }
#endif

#ifdef WITH_MYSQL
  std::unique_ptr<app::MySqlStore> mysqlStore;
  bool mysql_ok = false;
  try {
    const char* url  = getenv("MYSQL_URL");  if (!url)  url  = "tcp://127.0.0.1:3306";
    const char* user = getenv("MYSQL_USER"); if (!user) user = "root";
    const char* pass = getenv("MYSQL_PASS"); if (!pass) pass = "123456";
    const char* db   = getenv("MYSQL_DB");   if (!db)   db   = "shortener";
    mysqlStore.reset(new app::MySqlStore(url, user, pass, db));
    std::cout << "[init] MySQL connected at " << url << " db=" << db << std::endl;
    mysql_ok = true;
  } catch (const std::exception& e){
    std::cerr << "[init] MySQL disabled: " << e.what() << std::endl;
  }
#endif

#if defined(WITH_REDIS) && defined(WITH_MYSQL)
  if (redis_ok && mysql_ok){
    storePtr.reset(new app::CacheAsideStore(redisStore.get(), mysqlStore.get(), backfillTtl));
  } else if (redis_ok){
    storePtr = std::move(redisStore);
  } else if (mysql_ok){
    storePtr = std::move(mysqlStore);
  } else {
    storePtr.reset(new app::MemoryStore(&kv, SHARDS));
    std::cerr << "[init] Fallback to MemoryStore" << std::endl;
  }
#elif defined(WITH_REDIS)
  if (redis_ok) storePtr = std::move(redisStore);
  else { storePtr.reset(new app::MemoryStore(&kv, SHARDS)); }
#elif defined(WITH_MYSQL)
  if (mysql_ok) storePtr = std::move(mysqlStore);
  else { storePtr.reset(new app::MemoryStore(&kv, SHARDS)); }
#else
  storePtr.reset(new app::MemoryStore(&kv, SHARDS));
#endif

  app::State st{storePtr.get(), &bloom, &metrics, SHARDS};

  // ========== Middlewares ==========
  server.addMiddleware(std::make_shared<http::middleware::CorsMiddleware>());
#ifdef WITH_REDIS
  if (redis_ok) server.addMiddleware(std::make_shared<app::RateLimitRedis>(&redis, &metrics, 2000));
  else
#endif
  server.addMiddleware(std::make_shared<app::RateLimitMiddleware>(2000 /*rps*/, 2000 /*burst*/, &metrics));
  server.addMiddleware(std::make_shared<app::BlacklistMiddleware>(std::vector<std::string>{"javascript:", "data:"}));

  // LRU Cache 中间件（HTTP 层缓存，不改动）
  http::cache::CachePolicy pol; pol.memoryCapacityBytes = 64ull<<20; pol.ttl = std::chrono::seconds(300);
  auto store = std::make_shared<http::cache::MemoryCacheLRU>(pol.memoryCapacityBytes);
  server.addMiddleware(std::make_shared<app::CacheAdapter>(pol, store, &metrics));

  // ========== Routes ==========
  server.Post("/api/shorten", std::make_shared<app::ShortenHandler>(st));
  server.addRoute(http::HttpRequest::kGet, R"(^/s/([A-Za-z0-9_-]{3,64})$)", std::make_shared<app::RedirectHandler>(st));
  server.Get("/metrics", std::make_shared<app::MetricsHandler>(&metrics));
  server.Get("/healthz", std::make_shared<app::HealthHandler>());

  std::cout << "URL Shortener (cache-aside) listening on http://0.0.0.0:" << port << std::endl;
  server.start();
  return 0;
}
