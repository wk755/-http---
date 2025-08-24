-- 写多：持续创建短链的 wrk 脚本
-- 用法：
--   wrk -t8 -c400 -d120s --latency -s post_shorten.lua http://127.0.0.1:8080
-- 如果你的接口是 /shorten 且字段是 url/ttl，自行把路径和字段名改掉

wrk.method = "POST"
wrk.headers["Content-Type"] = "application/json"

local thread_seq = 0  -- 主控制，给每个线程一个唯一 tid

-- 给每个线程分配唯一 tid
function setup(thread)
  thread_seq = thread_seq + 1
  thread:set("tid", thread_seq)
end

-- 每个线程的本地计数器（Lua 全局在 wrk 里是“每线程一份”）
local counter = 0

-- 构造请求体：保证 long_url 全局唯一，避免唯一约束冲突
request = function()
  counter = counter + 1
  local tid = tonumber(wrk.thread:get("tid")) or 0
  -- 组合：线程号 + 计数器 + 随机数，确保不同线程不会撞车
  local uniq = string.format("%d-%d-%d", tid, counter, math.random(1e9))
  local long = string.format("https://example.com/page/%s", uniq)

  -- 如果你的服务字段是 ttl_sec，请把 ttl 改成 ttl_sec
  local body = string.format('{"long_url":"%s","ttl":%d}', long, 3600)

  -- wrk.method 已设为 POST，这里传 nil 即可复用
  return wrk.format(nil, "/api/shorten", nil, body)
end

-- 统计响应码分布，压测结束时打印（便于排错）
local ok2xx, conflict409, other = 0, 0, 0
function response(status, headers, body)
  if status >= 200 and status < 300 then
    ok2xx = ok2xx + 1
  elseif status == 409 then
    conflict409 = conflict409 + 1
  else
    other = other + 1
  end
end

function done(summary, latency, requests)
  io.write(("\n2xx=%d  409(conflict)=%d  OTHER=%d\n"):format(ok2xx, conflict409, other))
end
