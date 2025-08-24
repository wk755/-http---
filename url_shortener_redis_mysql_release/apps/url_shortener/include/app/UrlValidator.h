
#pragma once
#include <string>
#include <regex>

namespace app {
struct UrlValidator {
  static bool valid(const std::string& u){
    static const std::regex re(R"(^https?://[^\s]+$)", std::regex::icase);
    return std::regex_match(u, re);
  }
};
} // namespace app
