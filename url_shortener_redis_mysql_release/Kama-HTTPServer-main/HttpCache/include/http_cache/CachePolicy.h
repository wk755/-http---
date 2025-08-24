
#pragma once
#include <chrono>
#include <string>

namespace http::cache {

struct CachePolicy {
  enum class Store { Memory /*, Redis*/ };
  Store store = Store::Memory;

  bool cacheGET  = true;
  bool cacheHEAD = true;
  bool cache200  = true;
  bool cache301  = true;
  bool cache404  = true;

  size_t memoryCapacityBytes = 64ull * 1024 * 1024; // 64MB
  size_t maxObjectBytes      = 1ull * 1024 * 1024;  // 1MB

  std::chrono::seconds ttl{300};
  std::chrono::seconds staleWhileRevalidate{60};

  bool varyAcceptEncoding   = true;
  bool respectNoStore       = true;
  bool respectAuthorization = true;
};

} // namespace http::cache
