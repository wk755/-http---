curl -I "http://127.0.0.1:8080/s/${ID}" -- 80% 命中 + 20% miss 的压测脚本（含按线程播种的随机数）
-- 用法：
--   wrk -t8 -c800 -d120s --latency -s get_s.lua http://127.0.0.1:8080
--   # 可选：传入自定义 ids 文件： wrk ... -s get_s.lua -- /path/to/ids.txt URL

local ids = {}
local thread_seq = 0          -- 主脚本里的全局计数器，用来给每个线程分配一个唯一 tid
local ids_path = "ids.txt"    -- 默认从当前目录读取 ids.txt

-- 为每个线程分配一个唯一 tid（主进程阶段调用，每线程一次）
function setup(thread)
  thread_seq = thread_seq + 1
  thread:set("tid", thread_seq)
end

-- 每个线程启动时初始化：读取 ids，并按 tid 播种随机数
function init(args)
  -- 允许通过 `-- <ids路径>` 覆盖 ids 文件路径
  if args and args[1] then ids_path = args[1] end

  local f = io.open(ids_path, "r")
  if f then
    for line in f:lines() do
      if #line > 0 and line ~= "null" then
        ids[#ids + 1] = line
      end
    end
    f:close()
  end

  local tid = tonumber(wrk.thread:get("tid")) or 0
  -- 用时间 + 线程序号播种，避免多线程同一随机序列
  local seed = os.time() + tid * 1000003
  math.randomseed(seed)
end

-- 统计不同返回码（方便压测结束后核对命中/回源/异常比例）
local ok3xx, miss404, other = 0, 0, 0
function response(status, headers, body)
  if status >= 300 and status < 400 then
    ok3xx = ok3xx + 1
  elseif status == 404 or status == 410 then
    miss404 = miss404 + 1
  else
    other = other + 1
  end
end

-- 每个请求：80% 随机挑一个有效 id，20% 构造一个不存在的 id
request = function()
  local id
  if #ids > 0 and math.random(100) <= 80 then
    id = ids[math.random(#ids)]
  else
    id = "noexist_" .. tostring(math.random(1e9))
  end
  return wrk.format("GET", "/s/" .. id, {["Connection"] = "keep-alive"})
end

-- 压测结束时打印一行统计（命中/未命中/其他）
function done(summary, latency, requests)
  io.write(("\nOK(3xx)=%d  MISS(404/410)=%d  OTHER=%d\n"):format(ok3xx, miss404, other))
end
