#pragma once
#include "app/IStore.h"
#ifdef WITH_REDIS
#include "app/RedisStore.h"
#endif
#ifdef WITH_MYSQL
#include "app/MySqlStore.h"
#endif

namespace app {

// Redis 作为缓存、MySQL 作为权威存储的组合 Store（Cache-Aside 模式）
// 读：先 Redis，miss 再 MySQL；命中 MySQL 则以短 TTL 回填 Redis
// 写：先 MySQL（唯一性/持久），成功后写 Redis
class CacheAsideStore : public IStore {
public:
  CacheAsideStore(
#ifdef WITH_REDIS
    RedisStore* r,
#else
    void*,
#endif
#ifdef WITH_MYSQL
    MySqlStore* m,
#else
    void*,
#endif
    int backfillTtlSec = 60)
#ifdef WITH_REDIS
  : redis_(r)
#endif
#ifdef WITH_MYSQL
  , mysql_(m)
#endif
  , backfillTtlSec_(backfillTtlSec) {}

  std::optional<std::string> get(const std::string& key) override {
#ifdef WITH_REDIS
    if (redis_) { auto v = redis_->get(key); if (v) return v; }
#endif
#ifdef WITH_MYSQL
    if (mysql_) {
      auto v = mysql_->get(key);
      if (v) {
#ifdef WITH_REDIS
        if (redis_) redis_->set(key, *v, backfillTtlSec_);
#endif
        return v;
      }
    }
#endif
    return std::nullopt;
  }

  bool setNX(const std::string& key, const std::string& value, int ttlSec=0) override {
#ifdef WITH_MYSQL
    if (mysql_) { if (!mysql_->setNX(key, value, ttlSec)) return false; }
#endif
#ifdef WITH_REDIS
    if (redis_) redis_->set(key, value, ttlSec);
#endif
    return true;
  }

  void set(const std::string& key, const std::string& value, int ttlSec=0) override {
#ifdef WITH_MYSQL
    if (mysql_) mysql_->set(key, value, ttlSec);
#endif
#ifdef WITH_REDIS
    if (redis_) redis_->set(key, value, ttlSec);
#endif
  }

  void del(const std::string& key) override {
#ifdef WITH_MYSQL
    if (mysql_) mysql_->del(key);
#endif
#ifdef WITH_REDIS
    if (redis_) redis_->del(key);
#endif
  }

private:
#ifdef WITH_REDIS
  RedisStore* redis_{nullptr};
#endif
#ifdef WITH_MYSQL
  MySqlStore* mysql_{nullptr};
#endif
  int backfillTtlSec_{60};
};

} // namespace app
