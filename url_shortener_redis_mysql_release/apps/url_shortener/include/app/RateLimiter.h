#pragma once
#include <unordered_map>
#include <chrono>
#include <string>
#include <mutex>
namespace app {
class RateLimiter {
  struct Bucket{ double tokens; std::chrono::steady_clock::time_point last; };
  std::mutex mu_; std::unordered_map<std::string,Bucket> m_; double rate_, burst_;
public:
  RateLimiter(double rate, double burst): rate_(rate), burst_(burst) {}
  bool allow(const std::string& key){
    using clock=std::chrono::steady_clock; auto now=clock::now();
    std::lock_guard<std::mutex> lk(mu_); auto &b=m_[key];
    if (b.last.time_since_epoch().count()==0) { b.tokens=burst_; b.last=now; }
    double delta=std::chrono::duration<double>(now-b.last).count();
    b.tokens=std::min(burst_, b.tokens + delta*rate_); b.last=now;
    if (b.tokens>=1.0){ b.tokens-=1.0; return true; } return false;
  }
}; } // namespace app
