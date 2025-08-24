
#pragma once
#include <chrono>
#include <string>
#include <utility>
#include <vector>

namespace http::cache {

struct CachedEntry {
  std::string statusLine;
  std::vector<std::pair<std::string,std::string>> headers;
  std::string body;

  std::chrono::steady_clock::time_point hardExpire;
  std::chrono::steady_clock::time_point softExpire;

  size_t bytes() const {
    size_t n = statusLine.size() + body.size();
    for (auto &h: headers) n += h.first.size() + h.second.size();
    return n;
  }
};

} // namespace http::cache
