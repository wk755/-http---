
#include "http_cache/CacheMiddleware.h"
#include "http_cache/CacheKey.h"
#include "http_cache/CacheEntry.h"

// Adjust include paths to your project:
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"

using namespace http::cache;
using http::HttpRequest;
using http::HttpResponse;

std::string CacheMiddleware::getMethod(const HttpRequest& req) {
  switch (req.method()) {
    case HttpRequest::kGet: return "GET";
    case HttpRequest::kHead: return "HEAD";
    case HttpRequest::kPost: return "POST";
    case HttpRequest::kPut: return "PUT";
    case HttpRequest::kDelete: return "DELETE";
    case HttpRequest::kOptions: return "OPTIONS";
    default: return "UNKNOWN";
  }
}

std::string CacheMiddleware::getPathWithQuery(const HttpRequest& req) { return req.path(); }

std::string CacheMiddleware::getHeader(const HttpRequest& req, const std::string& k) {
  return req.getHeader(k);
}

int CacheMiddleware::getStatus(const HttpResponse& resp) { return static_cast<int>(resp.getStatusCode()); }

std::string CacheMiddleware::getBody(const HttpResponse& resp) { return resp.body(); }

void CacheMiddleware::addHeader(HttpResponse* resp, const std::string& k, const std::string& v) {
  resp->addHeader(k, v);
}

bool CacheMiddleware::hasNoStore(const HttpResponse& resp) {
  auto cc = resp.getHeader("Cache-Control");
  if (cc.find("no-store") != std::string::npos) return true;
  return false;
}

std::vector<std::pair<std::string,std::string>> CacheMiddleware::dumpHeaders(const HttpResponse& resp) { return resp.headersVector(); }

std::string CacheMiddleware::dumpStatusLine(const HttpResponse& resp) { return resp.statusLine(); }

void CacheMiddleware::setFromEntry(const CachedEntry& e, HttpResponse* out) {
  std::string ver="HTTP/1.1"; int code=200; std::string reason="OK";
  size_t sp1 = e.statusLine.find(' ');
  if (sp1!=std::string::npos) {
    size_t sp2 = e.statusLine.find(' ', sp1+1);
    ver = e.statusLine.substr(0, sp1);
    if (sp2!=std::string::npos){
      code = std::atoi(e.statusLine.substr(sp1+1, sp2-(sp1+1)).c_str());
      reason = e.statusLine.substr(sp2+1);
    }
  }
  out->setStatusLine(ver, static_cast<HttpResponse::HttpStatusCode>(code), reason);
  out->clearHeaders();
  for (auto &h: e.headers) out->addHeader(h.first, h.second);
  out->setBody(e.body);
}

bool CacheMiddleware::isCacheableRequest(const HttpRequest& req, const CachePolicy& p) {
  if (p.respectAuthorization && !getHeader(req, "authorization").empty()) return false;
  auto m = getMethod(req);
  if (m == "GET")  return p.cacheGET;
  if (m == "HEAD") return p.cacheHEAD;
  return false;
}

bool CacheMiddleware::isCacheableResponse(const HttpResponse& resp, const CachePolicy& p) {
  if (p.respectNoStore && hasNoStore(resp)) return false;
  int sc = getStatus(resp);
  return (sc==200 && p.cache200) || (sc==301 && p.cache301) || (sc==404 && p.cache404);
}

CacheKey CacheMiddleware::makeKey(const HttpRequest& req, const CachePolicy& p) {
  CacheKey k;
  k.method = getMethod(req);
  k.pathAndQuery = getPathWithQuery(req);
  if (p.varyAcceptEncoding) k.acceptEncoding = getHeader(req, "accept-encoding");
  return k;
}

CachedEntry CacheMiddleware::pack(const HttpResponse& resp,
                                  std::chrono::steady_clock::time_point now,
                                  const CachePolicy& p) {
  (void)p;
  CachedEntry e;
  e.statusLine = dumpStatusLine(resp);
  e.headers    = dumpHeaders(resp);
  e.body       = getBody(resp);
  e.hardExpire = now + p.ttl;
  e.softExpire = e.hardExpire + p.staleWhileRevalidate;
  return e;
}

bool CacheMiddleware::before(const HttpRequest& req, HttpResponse* resp) {
  if (!isCacheableRequest(req, policy_)) return false;
  auto key = makeKey(req, policy_);
  auto now = std::chrono::steady_clock::now();

  auto hit = store_->get(key);
  if (!hit) return false;

  if (now < hit->hardExpire) {
    setFromEntry(*hit, resp);
    return true;
  }
  if (now < hit->softExpire) {
    setFromEntry(*hit, resp);
    addHeader(resp, "Warning", "110 - Response is Stale");
    return true;
  }
  return false;
}

void CacheMiddleware::after(const HttpRequest& req, const HttpResponse& resp) {
  if (!isCacheableRequest(req, policy_))  return;
  if (!isCacheableResponse(resp, policy_)) return;
  auto e = pack(resp, std::chrono::steady_clock::now(), policy_);
  if (e.bytes() > policy_.maxObjectBytes) return;
  store_->set(makeKey(req, policy_), e);
}
