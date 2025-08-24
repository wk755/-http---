#pragma once
#include <string>
#include <random>
#include <chrono>
namespace app {
inline std::string genTraceId(){
  static thread_local std::mt19937_64 rng(
    std::chrono::high_resolution_clock::now().time_since_epoch().count());
  uint64_t x=rng(); char buf[17];
  snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)x);
  return std::string(buf);
}} // namespace app
