#pragma once
#include <map>
#include <string>
namespace app {
class HashRing {
  std::map<uint64_t,int> ring_;  //有序映射，把哈希值映射到物理分片编号
  int replicas_; //虚拟节点个数
public:
  explicit HashRing(int replicas=100): replicas_(replicas) {}
  void build(int shards){ 
    ring_.clear(); 
    for(int s=0; s<shards; ++s){
      for(int r=0;r<replicas_;++r)
      { 
        auto key="node"+std::to_string(s)+"#"+std::to_string(r); 
        ring_[h(key)] = s; 
      } 
    } 
  }

  int pick(const std::string& key) const 
  { 
    if(ring_.empty()) return 0; 
    auto it=ring_.lower_bound(h(key)); 
    if(it==ring_.end()) return ring_.begin()->second; 
    return it->second; 
  }
private: 
  static uint64_t h(const std::string& s)
  { 
    uint64_t H=1469598103934665603ull; 
    for(unsigned char c: s)
    { 
      H^=c; H*=1099511628211ull; 
    }  
    return H; 
  }
}; } // namespace app
