#pragma once
#ifdef WITH_REDIS
#include "app/RedisClient.h"
namespace app {
class RedisBloom {
  RedisClient* rc_; std::string bloomKey_; std::string setKey_; bool bloomOk_{true};
public:
  RedisBloom(RedisClient* rc, std::string bloomKey="bloom:ids", std::string setKey="set:ids")
  : rc_(rc), bloomKey_(std::move(bloomKey)), setKey_(std::move(setKey)) {
    auto r = rc_->cmd("BF.RESERVE %s 0.001 1000000", bloomKey_.c_str());
    if (r && (r->type == REDIS_REPLY_ERROR)) bloomOk_ = false; RedisClient::free(r);
  }
  void add(const std::string& s){
    if (bloomOk_) { auto r = rc_->cmd("BF.ADD %s %s", bloomKey_.c_str(), s.c_str()); RedisClient::free(r); }
    else { auto r = rc_->cmd("SADD %s %s", setKey_.c_str(), s.c_str()); RedisClient::free(r); }
  }
  bool mightContain(const std::string& s){
    if (bloomOk_) { auto r = rc_->cmd("BF.EXISTS %s %s", bloomKey_.c_str(), s.c_str());
      bool res = (r && r->type==REDIS_REPLY_INTEGER && r->integer==1); RedisClient::free(r); return res; }
    else { auto r = rc_->cmd("SISMEMBER %s %s", setKey_.c_str(), s.c_str());
      bool res = (r && r->type==REDIS_REPLY_INTEGER && r->integer==1); RedisClient::free(r); return res; }
  }
}; } // namespace app
#endif
