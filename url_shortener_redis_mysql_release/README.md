
# URL Shortener on Kama-HTTPServer (final)

- 路由：
  - `POST /api/shorten` → `{"id":"..."}`
  - `GET /s/{id}`       → 301/302 跳转
  - `GET /metrics`      → Prometheus 文本指标
  - `GET /healthz`      → 存活检查

- 存储：IStore（内存 / Redis / MySQL 可切换）
- 中间件：CORS / 限流（内存或 Redis）/ 黑名单 / LRU 缓存适配 / Trace 头

## 构建（默认纯内存）
```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release   -DMUDUO_LIBRARIES="-lmuduo_net -lmuduo_base -lpthread"   -DOPENSSL_LIBRARIES="-lssl -lcrypto"
make -j
./url_shortener 8080
```

## 启用 Redis
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DWITH_REDIS=ON   -DMUDUO_LIBRARIES="-lmuduo_net -lmuduo_base -lpthread"   -DOPENSSL_LIBRARIES="-lssl -lcrypto"
make -j
REDIS_HOST=127.0.0.1 REDIS_PORT=6379 ./url_shortener 8080
```

## 启用 MySQL
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DWITH_MYSQL=ON   -DMUDUO_LIBRARIES="-lmuduo_net -lmuduo_base -lpthread"   -DOPENSSL_LIBRARIES="-lssl -lcrypto"
make -j
./url_shortener 8080
```

## 压测脚本
```bash
./scripts/wrk_short.sh http://127.0.0.1:8080
```

## /metrics（含存储指标）
- `url_shortener_store_get_hit` / `url_shortener_store_get_miss`
- `url_shortener_store_setnx_ok` / `url_shortener_store_setnx_conflict`
