#pragma once

#include <muduo/net/TcpServer.h>

namespace http
{

class HttpResponse 
{
public:
    enum HttpStatusCode
    {
        kUnknown,
        k200Ok = 200,
        k204NoContent = 204,
        k301MovedPermanently = 301,
        k400BadRequest = 400,
        k401Unauthorized = 401,
        k403Forbidden = 403,
        k404NotFound = 404,
        k409Conflict = 409,
        k500InternalServerError = 500,
    };

    HttpResponse(bool close = true)
        : statusCode_(kUnknown)
        , closeConnection_(close)
    {}

    void setVersion(std::string version)
    { httpVersion_ = version; }
    void setStatusCode(HttpStatusCode code)
    { statusCode_ = code; }

    HttpStatusCode getStatusCode() const
    { return statusCode_; }

    void setStatusMessage(const std::string message)
    { statusMessage_ = message; }

    void setCloseConnection(bool on)
    { closeConnection_ = on; }

    bool closeConnection() const
    { return closeConnection_; }
    
    void setContentType(const std::string& contentType)
    { addHeader("Content-Type", contentType); }

    void setContentLength(uint64_t length)
    { addHeader("Content-Length", std::to_string(length)); }

    void addHeader(const std::string& key, const std::string& value)
    { headers_[key] = value; }
    
    void setBody(const std::string& body)
    { 
        body_ = body;
        // body_ += "\0";
    }

    void setStatusLine(const std::string& version,
                         HttpStatusCode statusCode,
                         const std::string& statusMessage);

    void setErrorHeader(){}

    // --- helpers for caching/metrics ---
    const std::string& body() const { return body_; }
    std::string getHeader(const std::string& field) const {
        auto it = headers_.find(field); if (it==headers_.end()) return std::string(); return it->second;
    }
    const std::map<std::string,std::string>& headers() const { return headers_; }
    std::vector<std::pair<std::string,std::string>> headersVector() const {
        std::vector<std::pair<std::string,std::string>> v; v.reserve(headers_.size());
        for (const auto &p: headers_) v.emplace_back(p.first, p.second);
        return v;
    }
    std::string statusLine() const {
        char buf[64]; snprintf(buf, sizeof buf, "%s %d ", httpVersion_.c_str(), statusCode_);
        return std::string(buf) + statusMessage_;
    }
    void clearHeaders() { headers_.clear(); }
    // --- end helpers ---


    void appendToBuffer(muduo::net::Buffer* outputBuf) const;
private:
    std::string                        httpVersion_; 
    HttpStatusCode                     statusCode_;
    std::string                        statusMessage_;
    bool                               closeConnection_;
    std::map<std::string, std::string> headers_;
    std::string                        body_;
    bool                               isFile_;
};

} // namespace http