#pragma once
#include "app/RateLimiter.h"
#include "app/Metrics.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "middleware/Middleware.h"
#include <memory>
namespace app {
class RateLimitMiddleware : public http::middleware::Middleware {
  RateLimiter limiter_; Metrics* metrics_;
public:
  RateLimitMiddleware(double rps, double burst, Metrics* m) : limiter_(rps, burst), metrics_(m) {}
  void before(http::HttpRequest& request) override {
    std::string key = request.getHeader("X-Client-Id"); if (key.empty()) key = "global";
    if (!limiter_.allow(key)) {
      if (metrics_) metrics_->ratelimit_drops.fetch_add(1);
      http::HttpResponse resp(false);
      resp.setStatusCode(http::HttpResponse::k403Forbidden);
      resp.addHeader("Retry-After", "1");
      resp.setBody("rate limited");
      throw resp;
    }
  }
  void after(http::HttpResponse&) override {}
}; } // namespace app
