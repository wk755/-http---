#pragma once
#include "middleware/Middleware.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include <vector>
#include <string>
namespace app {
class BlacklistMiddleware : public http::middleware::Middleware {
  std::vector<std::string> patterns_;
public:
  explicit BlacklistMiddleware(std::vector<std::string> patterns) : patterns_(std::move(patterns)) {}
  void before(http::HttpRequest& request) override {
    std::string chk = request.getBody(); chk.push_back(' '); chk += request.path();
    for (const auto& p : patterns_) {
      if (!p.empty() && chk.find(p) != std::string::npos) {
        http::HttpResponse resp(false);
        resp.setStatusCode(http::HttpResponse::k400BadRequest);
        resp.setBody("blocked by blacklist");
        throw resp;
      }
    }
  }
  void after(http::HttpResponse&) override {}
}; } // namespace app
