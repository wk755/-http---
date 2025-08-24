#pragma once
#include "app/IStore.h"
#include "app/ShortId.h"
#include "app/Bloom.h"
#include "app/UrlValidator.h"
#include "app/Metrics.h"
#include "router/RouterHandler.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include <memory>
namespace app {
struct State {
  IStore* store{nullptr};
  Bloom* bloom{nullptr};
  Metrics* metrics{nullptr};
  int shards{1};
};
class ShortenHandler : public http::router::RouterHandler {
  State st_;
public:
  explicit ShortenHandler(State s): st_(s) {}
  void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
};
class RedirectHandler : public http::router::RouterHandler {
  State st_;
public:
  explicit RedirectHandler(State s): st_(s) {}
  void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
};
class MetricsHandler : public http::router::RouterHandler {
  Metrics* m_;
public:
  explicit MetricsHandler(Metrics* m): m_(m) {}
  void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
};
class HealthHandler : public http::router::RouterHandler {
public:
  void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
}; } // namespace app
