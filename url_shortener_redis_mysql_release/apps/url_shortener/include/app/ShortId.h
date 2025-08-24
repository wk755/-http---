#pragma once
#include <string>
#include <random>
#include <chrono>
namespace app {
struct ShortId {
  static const std::string& charset(){
    static const std::string s="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    return s;
  }
  static std::string random(size_t len=7){
    static thread_local std::mt19937_64 rng(
      std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<size_t> dist(0, charset().size()-1);
    std::string out; out.reserve(len);
    for(size_t i=0;i<len;++i) out.push_back(charset()[dist(rng)]);
    return out;
  }
}; } // namespace app
