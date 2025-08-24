#pragma once
#ifdef WITH_REDIS
#include "app/Metrics.h"
#include "app/RedisClient.h"
#include "middleware/Middleware.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include <string>
namespace app {
class RateLimitRedis : public http::middleware::Middleware {
  RedisClient* rc_; Metrics* metrics_; int rps_; int windowSec_;
public:
  RateLimitRedis(RedisClient* rc, Metrics* m, int rps, int windowSec=1) : rc_(rc), metrics_(m), rps_(rps), windowSec_(windowSec) {}
  void before(http::HttpRequest& request) override {
    std::string key = request.getHeader("X-Client-Id"); if (key.empty()) key = "global";
    long sec = time(nullptr);
    std::string rkey = "rl:" + key + ":" + std::to_string(sec);
    redisReply* r = rc_->cmd("INCR %s", rkey.c_str()); long val = (r && r->type==REDIS_REPLY_INTEGER)? r->integer : 1; app::RedisClient::free(r);
    (void)rc_->cmd("EXPIRE %s %d", rkey.c_str(), windowSec_+1);
    if (val > rps_) {
      if (metrics_) metrics_->ratelimit_drops.fetch_add(1);
      http::HttpResponse resp(false); resp.setStatusCode(http::HttpResponse::k403Forbidden);
      resp.addHeader("Retry-After", "1"); resp.setBody("rate limited"); throw resp;
    }
  }
  void after(http::HttpResponse&) override {}
}; } // namespace app
#endif
