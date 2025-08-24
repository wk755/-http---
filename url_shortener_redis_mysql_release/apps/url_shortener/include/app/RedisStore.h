#pragma once
#include "app/IStore.h"
#ifdef WITH_REDIS
#include "app/RedisClient.h"
#include <optional>
namespace app {
class RedisStore : public IStore {
  RedisClient* rc_; std::string keyPrefix_;
public:
  RedisStore(RedisClient* rc, std::string prefix="short:") : rc_(rc), keyPrefix_(std::move(prefix)) {}
  std::optional<std::string> get(const std::string& key) override {
    auto full = keyPrefix_ + key; redisReply* r = rc_->cmd("GET %s", full.c_str());
    if (!r) return std::nullopt; std::optional<std::string> out;
    if (r->type == REDIS_REPLY_STRING) out = std::string(r->str, r->len); RedisClient::free(r); return out;
  }
  bool setNX(const std::string& key, const std::string& value, int ttlSec=0) override {
    auto full = keyPrefix_ + key;
    redisReply* r = rc_->cmd("SET %s %b NX %s %d", full.c_str(), value.data(), (size_t)value.size(),
                             ttlSec>0 ? "EX" : "", ttlSec>0? ttlSec : 0);
    if (!r) return false;
    bool ok = (r->type == REDIS_REPLY_STATUS && std::string(r->str, r->len) == "OK"); RedisClient::free(r); return ok;
  }
  void set(const std::string& key, const std::string& value, int ttlSec=0) override {
    auto full = keyPrefix_ + key;
    redisReply* r = rc_->cmd("SET %s %b %s %d", full.c_str(), value.data(), (size_t)value.size(),
                             ttlSec>0 ? "EX" : "", ttlSec>0? ttlSec : 0); RedisClient::free(r);
  }
  void del(const std::string& key) override {
    auto full = keyPrefix_ + key; redisReply* r = rc_->cmd("DEL %s", full.c_str()); RedisClient::free(r);
  }
}; } // namespace app
#endif
