
#pragma once
#include "CachePolicy.h"
#include "ICacheStore.h"
#include <memory>
#include <vector>
#include <utility>

namespace http {
  class HttpRequest;
  class HttpResponse;
}

namespace http::cache {

class CacheMiddleware {
  CachePolicy policy_;
  std::shared_ptr<ICacheStore> store_;

  static std::string getMethod(const http::HttpRequest& req);
  static std::string getPathWithQuery(const http::HttpRequest& req);
  static std::string getHeader(const http::HttpRequest& req, const std::string& k);
  static int         getStatus(const http::HttpResponse& resp);
  static std::string getBody(const http::HttpResponse& resp);
  static void        addHeader(http::HttpResponse* resp, const std::string& k, const std::string& v);
  static void        setFromEntry(const CachedEntry& e, http::HttpResponse* out);
  static bool        hasNoStore(const http::HttpResponse& resp);
  static std::vector<std::pair<std::string,std::string>> dumpHeaders(const http::HttpResponse& resp);
  static std::string dumpStatusLine(const http::HttpResponse& resp);

  static bool isCacheableRequest(const http::HttpRequest& req, const CachePolicy& p);
  static bool isCacheableResponse(const http::HttpResponse& resp, const CachePolicy& p);
  static CacheKey makeKey(const http::HttpRequest& req, const CachePolicy& p);
  static CachedEntry pack(const http::HttpResponse& resp,
                          std::chrono::steady_clock::time_point now,
                          const CachePolicy& p);

public:
  CacheMiddleware(CachePolicy p, std::shared_ptr<ICacheStore> s)
    : policy_(p), store_(std::move(s)) {}

  bool before(const http::HttpRequest& req, http::HttpResponse* resp);
  void after(const http::HttpRequest& req, const http::HttpResponse& resp);
  void purgePrefix(const std::string& prefix) { store_->purgePrefix(prefix); }
};

} // namespace http::cache
