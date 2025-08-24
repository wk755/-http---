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

#include <memory>
#include <iostream>
#include <cstdlib>

#ifdef WITH_REDIS
#include "app/RedisClient.h"
#include "app/RedisStore.h"
#include "app/RedisBloom.h"
#endif

using namespace http;
int main(int argc, char** argv){
  int port = 8080; if (argc > 1) port = std::stoi(argv[1]);
  HttpServer server(port, "url_shortener", /*useSSL*/false);
  server.setThreadNum(4);

  app::Metrics metrics;
  app::Bloom    bloom(1<<20);
  const int SHARDS = 8;
  app::KVStore kv(SHARDS);

  // Backend selection
  std::unique_ptr<app::IStore> storePtr;
#ifdef WITH_REDIS
  app::RedisClient redis;
  try {
    const char* rh = getenv("REDIS_HOST"); if (!rh) rh = "127.0.0.1";
    const char* rp = getenv("REDIS_PORT"); int rport = rp? std::atoi(rp) : 6379;
    redis.connect(rh, rport, 200);
    storePtr.reset(new app::RedisStore(&redis, "short:"));
    std::cout << "[init] Redis connected at " << rh << ":" << rport << std::endl;
  } catch (const std::exception& e){
    std::cerr << "[init] Redis disabled: " << e.what() << " -> fallback MemoryStore" << std::endl;
    storePtr.reset(new app::MemoryStore(&kv, SHARDS));
  }
#else
  storePtr.reset(new app::MemoryStore(&kv, SHARDS));
#endif

  app::State st{storePtr.get(), &bloom, &metrics, SHARDS};

  // Middlewares
  server.addMiddleware(std::make_shared<http::middleware::CorsMiddleware>());
#ifdef WITH_REDIS
  if (redis.ok()) server.addMiddleware(std::make_shared<app::RateLimitRedis>(&redis, &metrics, 2000));
  else
#endif
  server.addMiddleware(std::make_shared<app::RateLimitMiddleware>(2000 /*rps*/, 2000 /*burst*/, &metrics));
  server.addMiddleware(std::make_shared<app::BlacklistMiddleware>(std::vector<std::string>{"javascript:", "data:"}));

  // Cache adapter (LRU)
  http::cache::CachePolicy pol; pol.memoryCapacityBytes = 64ull<<20; pol.ttl = std::chrono::seconds(300);
  auto store = std::make_shared<http::cache::MemoryCacheLRU>(pol.memoryCapacityBytes);
  server.addMiddleware(std::make_shared<app::CacheAdapter>(pol, store, &metrics));

  // Routes
  server.Post("/api/shorten", std::make_shared<app::ShortenHandler>(st));
  server.addRoute(http::HttpRequest::kGet, R"(^/s/([A-Za-z0-9_-]{3,64})$)", std::make_shared<app::RedirectHandler>(st)); //动态路由处理器
  server.Get("/metrics", std::make_shared<app::MetricsHandler>(&metrics));
  server.Get("/healthz", std::make_shared<app::HealthHandler>());

  std::cout << "URL Shortener listening on http://0.0.0.0:" << port << std::endl;
  server.start();
  return 0;
}
