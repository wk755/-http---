
#pragma once
#include <functional>
#include <string>

namespace http::cache {

struct CacheKey {
  std::string method;
  std::string pathAndQuery;
  std::string acceptEncoding;

  bool operator==(const CacheKey& o) const {
    return method==o.method && pathAndQuery==o.pathAndQuery
           && acceptEncoding==o.acceptEncoding;
  }
};

struct CacheKeyHash {
  size_t operator()(const CacheKey& k) const noexcept {
    std::hash<std::string> h;
    return h(k.method) ^ (h(k.pathAndQuery)<<1) ^ (h(k.acceptEncoding)<<2);
  }
};

} // namespace http::cache
