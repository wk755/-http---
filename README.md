# 基于Muduo的HTTP高性能通信框架 -- 实现高性能短链接服务

一个面向生产可扩展的**高性能短链接服务**，核心体现为 **高性能 HTTP 通信框架**：  
基于 Muduo 的 Reactor/epoll 非阻塞 I/O、多事件循环线程、可组合中间件链（CORS / 限流 / LRU 响应缓存）、正则/前缀路由、可观测指标与可插拔存储（内存 / Redis / 可选 MySQL）。提供 `POST /api/shorten` 生成短链与 `GET /s/{id}` 重定向的最小闭环。

设计目标：**极简接口 + 高 QPS 读写 + 易扩展 + 易观测**，业务逻辑解耦于框架，便于在此基础上替换存储、升级限流、扩展鉴权等。

---

## 目录

- [特性 Highlights](#特性-highlights)
- [总体架构 Architecture](#总体架构-architecture)
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

## 构建与运行 Build & Run
### 环境与依赖（Environment & Dependencies）

- **操作系统（任一）**：
Linux（Ubuntu 20.04+/Debian 11+/CentOS/RHEL 8+/Alma/ Rocky），
macOS 12+（Apple Silicon/Intel）
- **编译器**：GCC 9+ 或 Clang 12+（支持 C++17）
- **构建工具**：CMake 3.16+
- **网络库**：Muduo（已随工程封装/链接）
- **可选依赖**：
  - OpenSSL（HTTPS）
  - hiredis（Redis）
  - mysqlclient / mysql-connector-cpp（MySQL）

### 运行：
### 1) 仅本地内存模式（零依赖）

```bash
# 构建（Release）
cmake -DCMAKE_BUILD_TYPE=Release  ..
make

# 运行（默认 8080）
./build/url_shortener 8080
```

### 2)  Redis 模式

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DWITH_REDIS=ON ..
make

# 运行
./build/url_shortener 8080
```
### 3) Redis + MySQL 模式（同时启用）
```bash
cmake -DCMAKE_BUILD_TYPE=Release -DWITH_REDIS=ON -DWITH_MYSQL=ON ..
make

# 运行
./build/url_shortener 8080
```
### 链接过程：
启动服务器后，运行
```bash
ID=$(curl -s -X POST 'http://127.0.0.1:8080/api/shorten' \
  -H 'Content-Type: application/json' \
  -d '{"url":"https://example.com/demo?ts='"$(date +%s)"'","ttl":600}' | jq -r '.id')
echo "$ID"
```
创建短链接，得到ID，然后连接ID跳转服务
```bash
curl -i "http://127.0.0.1:8080/s/$ID" 
```
<img width="2070" height="1062" alt="image" src="https://github.com/user-attachments/assets/04624fca-0ea8-405f-9e38-1483a8c10e54" />

## 性能与压测 
### 1. 读测试
用最轻的接口 /healthz 做 30 秒压测，几乎不做业务，只测HTTP框架自身的开销（路由、解析、写回等）。
```bash
# 生成 2000 条短链，取出返回的 id 存到 ids.txt
for i in $(seq 1 2000); do
  curl -s -X POST http://127.0.0.1:8080/api/shorten \
    -H 'Content-Type: application/json' \
    -d '{"long_url":"https://example.com/page/'"$i"'","ttl":3600}' \
  | grep -o '"id":"[^"]*"' | head -n1 | cut -d\" -f4
done > ids.txt

# 验证随机一条可跳转（应返回 301/302）
HOT=$(shuf -n1 ids.txt | tr -d '\r\n')
curl -g -i "http://127.0.0.1:8080/s/${HOT}" | sed -n '1,10p'


#用 wrk 压这个 HOT
wrk -t8 -c800 -d60s --timeout 10s --latency "http://127.0.0.1:8080/s/${HOT}"

```

# 测试结果如下：
结果表明：
-t8：8 个压测线程

-c800：总共 800 个并发连接（平均每线程 ~100）

-d60s：持续 60 秒

--timeout 10s：单请求/连接超时 10 秒

--latency：打印延迟分位（P50/P75/P90/P99）

URL：固定读 GET /s/{id}（这里的 ${HOT} 是某个短链 id）

输出关键读数（限流开启）

Requests/sec: 4603.91（统计了全部响应：含 2xx/3xx 也含 4xx/5xx）

Total: 276,524 次请求 / 60s

Non-2xx or 3xx responses: 154,803（≈ 56% 被限流/出错，典型是 403/429）

成功请求数: 276,524 − 154,803 = 121,721

⇒ 成功吞吐（允许的 RPS）≈ 121,721 / 60 = 2,028.7 /s

被限流吞吐 ≈ 154,803 / 60 = 2,580.1 /s

Latency 分布（含被拒请求的总体分布）

P50 ~ 150 ms，P90 ~ 234 ms，P99 ~ 1.08 s
在限流/背压下，队列与连接抖动 + 日志 IO 会把尾延迟推高，这是预期现象；这些分位数不代表系统在“被接受的请求”上的真实能力。在配置的限流下，服务整体被“卡”在 ~2.0k 成功 RPS，其余 ~2.6k RPS 被按策略拒绝（403/429）。这说明限流策略生效；但该场景不适合评估框架极限吞吐/延迟。

<img width="1977" height="441" alt="image" src="https://github.com/user-attachments/assets/c2264151-47ba-4b4d-b735-15478e29bde8" />


### 2. 写测试
```bash
 -- post_shorten.lua
 构建一个脚本
wrk.method = "POST"
wrk.headers["Content-Type"] = "application/json"
local counter = 0

-- 每个请求构造一个唯一 long_url，避免重复键冲突
request = function()
  counter = counter + 1
  local long = string.format("https://example.com/page/%d?utm=%d", counter, math.random(1e9))
  local body = string.format('{"url":"%s","ttl":%d}', long, 3600)
  return wrk.format(nil, "/api/shorten", nil, body)
end

压测
wrk -t8 -c800 -d60s --timeout 10s --latency "http://127.0.0.1:8080/s/${HOT}"
```
结果说明：
压测配置（限流开启）

工具：wrk -t8 -c400 -d120s --latency -s post_shorten.lua http://127.0.0.1:8080

说明：8 线程、400 并发、持续 120s，脚本每次请求 POST /api/shorten 创建短链（写压）。

总吞吐（含被拒）： Requests/sec ≈ 4,910.82

总请求数： 589,722

非 2xx/3xx（被限/错误）： 347,682 → 拒绝率 ≈ 58.95%

成功请求数： 589,722 − 347,682 = 242,040

成功吞吐（≈允许的 RPS）： 242,040 / 120 ≈ 2,017 /s

延迟分布（总体，含被拒）： P50 ≈ 76.5 ms，P90 ≈ 95.8 ms，P99 ≈ 190 ms

在限流与背压下，整体分位数被抬高；评估真实性能应以“仅成功请求”分布为准。在启用令牌桶限流策略的情况下，服务被稳定“卡”在 ~2.0k 成功 RPS；其余 ~2.9k RPS 按策略返回 403/429（Retry-After），系统在高压+背压场景下保持稳定无崩溃。
<img width="2046" height="510" alt="image" src="https://github.com/user-attachments/assets/08fb88a2-3f66-49c3-95a2-11d8e9afc428" />

## 可观测性
现有观测点

/healthz：存活探针
GET 返回 200 ok。用来给 wrk 预热、K8s liveness/readiness 等。

/metrics：Prometheus 文本格式指标
由 app::Metrics::prometheus() 输出，直接可被 Prometheus 抓取。已有指标（部分）：

url_shortener_shorten_ok / url_shortener_shorten_bad：短链创建 成功/失败总数

url_shortener_redirect_ok / url_shortener_redirect_miss：跳转 成功/未命中（404）总数

url_shortener_ratelimit_drops：被限流丢弃的请求数

url_shortener_cache_hits / url_shortener_cache_sets：HTTP 层 LRU 缓存 命中/写入

store_get_hit / store_get_miss / store_setnx_ok / store_setnx_conflict：底层存储命中/冲突等

这些都在 apps/url_shortener/include/app/Metrics.h 里定义并在 /metrics 暴露。

日志：Muduo 风格的 INFO/ERROR（连接建立/移除、EPIPE、限流返回等）。压测时建议把连接级别日志降噪或重定向到文件，避免干扰延迟。


