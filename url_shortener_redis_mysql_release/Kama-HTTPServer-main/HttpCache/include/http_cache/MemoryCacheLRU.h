
#pragma once
#include "ICacheStore.h"
#include <list>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

namespace http::cache {

class MemoryCacheLRU : public ICacheStore {
  using ListIt = std::list<std::pair<CacheKey, CachedEntry>>::iterator;

  size_t capBytes_;
  size_t usedBytes_{0};
  std::list<std::pair<CacheKey, CachedEntry>> lru_;
  std::unordered_map<CacheKey, ListIt, CacheKeyHash> map_;
  mutable std::shared_mutex mu_;

  void evictIfNeeded();

public:
  explicit MemoryCacheLRU(size_t capBytes);

  std::optional<CachedEntry> get(const CacheKey& key) override;
  void set(const CacheKey& key, const CachedEntry& e) override;
  void del(const CacheKey& key) override;
  void purgePrefix(const std::string& pathPrefix) override;
};

} // namespace http::cache
