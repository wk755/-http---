# 基于Muduo的HTTP高性能通信框架 -- 实现高性能短链接服务

一个面向生产可扩展的**高性能短链接服务**，核心体现为 **高性能 HTTP 通信框架**：  
基于 Muduo 的 Reactor/epoll 非阻塞 I/O、多事件循环线程、可组合中间件链（CORS / 限流 / LRU 响应缓存）、正则/前缀路由、可观测指标与可插拔存储（内存 / Redis / 可选 MySQL）。提供 `POST /api/shorten` 生成短链与 `GET /s/{id}` 重定向的最小闭环。

设计目标：**极简接口 + 高 QPS 读写 + 易扩展 + 易观测**，业务逻辑解耦于框架，便于在此基础上替换存储、升级限流、扩展鉴权等。

---

## 目录

- [特性 Highlights](#特性-highlights)
- [总体架构 Architecture](#总体架构-architecture)
- [快速开始 Quick Start](#快速开始-quick-start)
- [构建与运行 Build & Run](#构建与运行-build--run)
- [性能与压测 Benchmarking](#性能与压测-benchmarking)
- [可观测性 Observability](#可观测性-observability)

---

## 特性 Highlights

- **高性能 HTTP 框架**
  - Reactor + **epoll** 非阻塞 I/O
  - **多事件循环线程**（主循环接入；N 个 Worker 事件循环并发处理）
  - 静态/正则 **路由**，零热路径开销优化（预编译正则、近路由匹配）
- **中间件链（Before/After 双相）**
  - **CORS**：零侵入跨域支持
  - **限流**：内存令牌桶或 Redis 固定窗口，前置快速拒绝
  - **LRU 响应缓存**（HttpCache）：缓存 `200/301/404`，**命中直出**
- **可插拔存储 IStore**：Memory / Redis /（可选）MySQL
  - Redis 写入采用 `SETNX/EX` 保障幂等与 TTL
  - 本地 **Bloom** 预筛，减少不存在键的存储访问
- **可观测性**
  - **/metrics** 暴露 Prometheus 指标（QPS、命中率、限流丢弃、存储命中等）
  - **/healthz** 存活检查
- **易用接口**
  - `POST /api/shorten` 生成短链
  - `GET /s/{id}` 301 跳转

---

## 总体架构 Architecture

```text
Client
│
▼
[Acceptor (non-blocking, epoll)] --> BaseLoop
│ accepts
├───────────────────────────────► Worker EventLoops (N threads)
│
▼
[Middleware Chain]
CORS → RateLimit → HttpCache
│
▼
Router
static path + regex param extraction
│
▼
Handler
(CPU-light, async I/O friendly)
│
▼
[Store Abstraction]
Memory / Redis / (MySQL optional dual-write)
│
▼
Persistence
```

**热点路径优化：**
- 限流前置（最便宜的判断尽早拒绝）
- Bloom 预筛减少回源
- HttpCache 命中 **before 阶段短路返回**，after 阶段写入缓存
- 路由正则预编译 + 轻量 Buffer，减少 alloc/拷贝

## 快速开始 Quick Start

### 1) 仅本地内存模式（零依赖）

```bash
# 构建（Release）
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# 运行（默认 8080）
./build/url_shortener 8080
```

### 2)  Redis 模式

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DWITH_REDIS=ON
cmake --build build -j

# 运行
./build/url_shortener 8080
```

## 构建与运行 Build & Run

### 依赖建议
- **编译器**：GCC 9+ / Clang 12+（C++17）
- **构建工具**：CMake 3.16+
- **网络库**：Muduo（已随工程封装/链接）
- **可选依赖**：
  - OpenSSL（HTTPS）
  - hiredis（Redis）
  - mysqlclient / mysql-connector-cpp（MySQL）

