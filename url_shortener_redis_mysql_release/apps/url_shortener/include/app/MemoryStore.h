
#pragma once
#include "app/IStore.h"
#include "app/KVStore.h"
namespace app {
class MemoryStore : public IStore {
  KVStore* kv_; int shards_;
public:
  MemoryStore(KVStore* kv, int shards) : kv_(kv), shards_(shards) {}
  std::optional<std::string> get(const std::string& key) override {
    int shard = (unsigned char)key[0] % shards_;
    return kv_->get(shard, key);
  }
  bool setNX(const std::string& key, const std::string& value, int ttlSec=0) override {
    int shard = (unsigned char)key[0] % shards_;
    auto v = kv_->get(shard, key); if (v.has_value()) return false;
    kv_->set(shard, key, value); (void)ttlSec; return true;
  }
  void set(const std::string& key, const std::string& value, int ttlSec=0) override {
    int shard = (unsigned char)key[0] % shards_; kv_->set(shard, key, value); (void)ttlSec;
  }
  void del(const std::string& key) override { (void)key; }
}; } // namespace app
