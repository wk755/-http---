#pragma once
#include <vector>
#include <string>
#include <cstdint>
namespace app {
class Bloom {
  std::vector<uint64_t> bits_; size_t m_;
public:
  explicit Bloom(size_t bits=1<<20) : bits_((bits+63)/64), m_(bits) {}
  void add(const std::string& s){ for(int i=0;i<3;++i) set(h(s,i)%m_); }
  bool mightContain(const std::string& s) const {
    for(int i=0;i<3;++i) if(!get(h(s,i)%m_)) return false; return true;
  }
private:
  static uint64_t h(const std::string& s, uint64_t seed){
    uint64_t hash=1469598103934665603ull ^ seed;
    for(unsigned char c: s){ hash^=c; hash*=1099511628211ull; } return hash ^ (seed*0x9e3779b97f4a7c15ull);
  } //哈希函数
  void set(size_t i){ bits_[i>>6]|=(1ull<<(i&63)); } //就是i/64 + i%64
  bool get(size_t i) const { return (bits_[i>>6]>>(i&63))&1ull; }
}; } // namespace app
