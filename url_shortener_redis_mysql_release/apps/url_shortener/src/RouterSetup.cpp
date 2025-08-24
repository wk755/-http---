#include "app/RouterSetup.h"
#include "app/UrlValidator.h"
#include "app/ShortId.h"
#include "app/Metrics.h"
#include <string>
using http::HttpRequest; using http::HttpResponse;
namespace {
bool parseJson(const std::string& body, std::string& url, int& ttl){
  url.clear(); ttl = 0; auto pos = body.find("\"url\""); if (pos==std::string::npos) return false;
  pos = body.find(':', pos); if (pos==std::string::npos) return false; pos = body.find('\"', pos); if (pos==std::string::npos) return false;
  auto end = body.find('\"', pos+1); if (end==std::string::npos) return false; url = body.substr(pos+1, end-pos-1);
  auto tt = body.find("\"ttl\""); if (tt!=std::string::npos){ auto p2 = body.find(':', tt); if (p2!=std::string::npos){ int v=0; size_t k=p2+1;
    while (k<body.size() && isspace((unsigned char)body[k])) ++k; while (k<body.size() && isdigit((unsigned char)body[k])) { v = v*10 + (body[k]-'0'); ++k; } ttl=v; } }
  return true;
}}
namespace app {
void ShortenHandler::handle(const HttpRequest& req, HttpResponse* resp){
  auto body = req.getBody(); std::string url; int ttl=0;
  if (!parseJson(body, url, ttl) || !UrlValidator::valid(url)){
    if (st_.metrics) st_.metrics->shorten_bad.fetch_add(1);
    resp->setStatusCode(HttpResponse::k400BadRequest); resp->setContentType("application/json"); resp->setBody(R"({"error":"invalid url"})"); return;
  }
  std::string id;
  for (int i=0;i<5;++i){
    id = ShortId::random(7);
    if (st_.store->setNX(id, url, ttl)){
      if (st_.bloom) st_.bloom->add(id);
      if (st_.metrics) { st_.metrics->shorten_ok.fetch_add(1); st_.metrics->store_setnx_ok.fetch_add(1); }
      resp->setStatusCode(HttpResponse::k200Ok); resp->setContentType("application/json");
      resp->setBody(std::string("{\"id\":\"") + id + "\"}"); return;
    } else {
      if (st_.metrics) st_.metrics->store_setnx_conflict.fetch_add(1);
    }
  }
  resp->setStatusCode(HttpResponse::k500InternalServerError); resp->setBody("exhausted id retries");
}
void RedirectHandler::handle(const HttpRequest& req, HttpResponse* resp){
  auto id = req.getPathParameters("param1");
  if (id.empty()){ resp->setStatusCode(HttpResponse::k400BadRequest); resp->setBody("bad id"); return; }
  if (st_.bloom && !st_.bloom->mightContain(id)){
    if (st_.metrics) st_.metrics->redirect_miss.fetch_add(1);
    resp->setStatusCode(HttpResponse::k404NotFound); resp->setBody("not found"); return;
  }
  auto v = st_.store->get(id);
  if (!v.has_value()){
    if (st_.metrics){ st_.metrics->redirect_miss.fetch_add(1); st_.metrics->store_get_miss.fetch_add(1); }
    resp->setStatusCode(HttpResponse::k404NotFound); resp->setBody("not found"); return;
  }
  if (st_.metrics){ st_.metrics->redirect_ok.fetch_add(1); st_.metrics->store_get_hit.fetch_add(1); }
  resp->setStatusCode(HttpResponse::k301MovedPermanently);
  resp->addHeader("Location", *v);
  resp->addHeader("X-Cacheable", "1");
}
void MetricsHandler::handle(const HttpRequest& req, HttpResponse* resp){
  (void)req; resp->setStatusCode(HttpResponse::k200Ok); resp->setContentType("text/plain; version=0.0.4");
  resp->setBody(m_->prometheus());
}
void HealthHandler::handle(const HttpRequest& req, HttpResponse* resp){
  (void)req; resp->setStatusCode(HttpResponse::k200Ok); resp->setBody("ok");
}} // namespace app
