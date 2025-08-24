
#pragma once
#include "CacheKey.h"
#include "CacheEntry.h"
#include <optional>
#include <string>

namespace http::cache {

class ICacheStore {
public:
  virtual ~ICacheStore() = default;
  virtual std::optional<CachedEntry> get(const CacheKey& key) = 0;
  virtual void set(const CacheKey& key, const CachedEntry& e) = 0;
  virtual void del(const CacheKey& key) = 0;
  virtual void purgePrefix(const std::string& pathPrefix) = 0;
};

} // namespace http::cache
