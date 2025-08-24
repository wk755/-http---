#pragma once
#include "app/Metrics.h"
#include "http_cache/CacheMiddleware.h"
#include "http_cache/MemoryCacheLRU.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "middleware/Middleware.h"
#include <memory>
namespace app {
class CacheAdapter : public http::middleware::Middleware {
  http::cache::CachePolicy policy_;
  std::shared_ptr<http::cache::ICacheStore> store_;
  std::unique_ptr<http::cache::CacheMiddleware> cache_;
  Metrics* metrics_;
public:
  CacheAdapter(const http::cache::CachePolicy& p,
               std::shared_ptr<http::cache::ICacheStore> s,
               Metrics* metrics)
  : policy_(p), store_(std::move(s)), metrics_(metrics)
  { cache_ = std::make_unique<http::cache::CacheMiddleware>(policy_, store_); }
  void before(http::HttpRequest& request) override {
    http::HttpResponse resp(false);
    if (cache_->before(request, &resp)) {
      if (metrics_) metrics_->cache_hits.fetch_add(1);
      throw resp;
    }
  }
  void after(http::HttpResponse& response) override {
    cache_->after(http::HttpRequest{}, response);
    if (metrics_) metrics_->cache_sets.fetch_add(1);
  }
}; } // namespace app
