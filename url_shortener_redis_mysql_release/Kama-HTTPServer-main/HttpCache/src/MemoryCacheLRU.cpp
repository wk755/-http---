
#include "http_cache/MemoryCacheLRU.h"

using namespace http::cache;

MemoryCacheLRU::MemoryCacheLRU(size_t capBytes) : capBytes_(capBytes) {}

void MemoryCacheLRU::evictIfNeeded() {
  while (usedBytes_ > capBytes_ && !lru_.empty()) {
    usedBytes_ -= lru_.back().second.bytes();
    map_.erase(lru_.back().first);
    lru_.pop_back();
  }
}

std::optional<CachedEntry> MemoryCacheLRU::get(const CacheKey& key) {
  std::unique_lock lock(mu_);
  auto it = map_.find(key);
  if (it == map_.end()) return std::nullopt;
  lru_.splice(lru_.begin(), lru_, it->second);
  return it->second->second;
}

void MemoryCacheLRU::set(const CacheKey& key, const CachedEntry& e) {
  std::unique_lock lock(mu_);
  size_t sz = e.bytes();
  if (sz > capBytes_) return;

  auto it = map_.find(key);
  if (it != map_.end()) {
    usedBytes_ -= it->second->second.bytes();
    it->second->second = e;
    usedBytes_ += sz;
    lru_.splice(lru_.begin(), lru_, it->second);
  } else {
    lru_.emplace_front(key, e);
    map_[key] = lru_.begin();
    usedBytes_ += sz;
  }
  evictIfNeeded();
}

void MemoryCacheLRU::del(const CacheKey& key) {
  std::unique_lock lock(mu_);
  auto it = map_.find(key);
  if (it == map_.end()) return;
  usedBytes_ -= it->second->second.bytes();
  lru_.erase(it->second);
  map_.erase(it);
}

void MemoryCacheLRU::purgePrefix(const std::string& prefix) {
  std::unique_lock lock(mu_);
  for (auto it = lru_.begin(); it != lru_.end(); ) {
    if (it->first.pathAndQuery.rfind(prefix, 0) == 0) {
      usedBytes_ -= it->second.bytes();
      map_.erase(it->first);
      it = lru_.erase(it);
    } else ++it;
  }
}
