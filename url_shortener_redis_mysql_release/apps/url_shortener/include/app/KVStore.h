#pragma once
#include <vector>
#include <shared_mutex>
#include <unordered_map>
#include <optional>
#include <mutex>
#include <string>
namespace app {
class KVStore {
  struct Shard { mutable std::shared_mutex mu; std::unordered_map<std::string,std::string> m; };
  std::vector<Shard> shards_;
public:
  explicit KVStore(int shards): shards_(shards) {}
  std::optional<std::string> get(int shard, const std::string& k) const {
    const auto& s = shards_[shard]; std::shared_lock lk(s.mu);
    auto it = s.m.find(k); if (it==s.m.end()) return std::nullopt; return it->second;
  }
  void set(int shard, const std::string& k, const std::string& v){
    auto& s = shards_[shard]; std::unique_lock lk(s.mu); s.m[k]=v;
  }
}; } // namespace app
