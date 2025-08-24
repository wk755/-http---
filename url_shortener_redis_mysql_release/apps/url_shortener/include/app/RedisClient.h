// RedisClient.h
#pragma once
#ifdef WITH_REDIS
#include <hiredis/hiredis.h>
#include <string>
#include <stdexcept>
#include <cstdarg>

// 需要 C++17（inline 变量）
namespace app {

class RedisClient {
public:
  // 构造时可不传；保持与旧代码兼容：后面可再调用 connect()
  RedisClient(std::string host = "127.0.0.1", int port = 6379, int timeout_ms = 200)
  : host_(std::move(host)), port_(port) {
    tv_.tv_sec  = timeout_ms/1000;
    tv_.tv_usec = (timeout_ms%1000)*1000;
  }

  ~RedisClient() { close(); }                // 仅关闭“当前线程”的连接

  // 兼容旧代码：设置参数并清掉本线程旧连接；真正连在第一次 cmd() 时做
  void connect(const std::string& host, int port, int timeout_ms = 200) {
    host_ = host; port_ = port;
    tv_.tv_sec  = timeout_ms/1000;
    tv_.tv_usec = (timeout_ms%1000)*1000;
    close();                                  // 清除当前线程的 tls 连接
  }

  void close() {
    if (tls_ctx_) { redisFree(tls_ctx_); tls_ctx_ = nullptr; }
  }

  // 仅表示“当前线程是否已有连接”
  bool ok() const { return tls_ctx_ != nullptr; }

  // 线程安全：每个线程使用自己的 redisContext
  redisReply* cmd(const char* fmt, ...) {
    redisContext* c = get();                  // 取本线程连接（懒连接）
    va_list ap; va_start(ap, fmt);
    auto* r = (redisReply*)redisvCommand(c, fmt, ap);
    va_end(ap);

    // 断链时重连一次
    if (!r) {
      if (tls_ctx_) { redisFree(tls_ctx_); tls_ctx_ = nullptr; }
      c = get();
      va_list ap2; va_start(ap2, fmt);
      r = (redisReply*)redisvCommand(c, fmt, ap2);
      va_end(ap2);
    }
    return r;                                  // 调用者记得 freeReplyObject(r)
  }

  static void free(redisReply* r){ if (r) freeReplyObject(r); }

private:
  redisContext* get() {
    if (!tls_ctx_) {
      tls_ctx_ = redisConnectWithTimeout(host_.c_str(), port_, tv_);
      if (!tls_ctx_ || tls_ctx_->err) {
        throw std::runtime_error(
          std::string("redis connect failed: ") + (tls_ctx_ ? tls_ctx_->errstr : "no ctx"));
      }
    }
    return tls_ctx_;
  }

  std::string host_;
  int         port_;
  timeval     tv_{};

  // ★ 每线程一条连接；header 中用 inline thread_local 避免多重定义
  inline static thread_local redisContext* tls_ctx_ = nullptr;
};

} // namespace app
#endif


