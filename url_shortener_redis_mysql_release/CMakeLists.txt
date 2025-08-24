cmake_minimum_required(VERSION 3.12)
project(url_shortener_all CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ===== Feature toggles =====
option(WITH_REDIS  "Enable Redis-based store & rate limiter" OFF)
option(WITH_MYSQL  "Enable MySQL-based store (SoR)"         OFF)

# 可选：用户也可以手动覆盖这些变量（例如交叉编译或自定义安装路径）
set(MUDUO_LIBRARIES    "" CACHE STRING "Override: e.g. -lmuduo_net;-lmuduo_base;-lpthread")
set(OPENSSL_LIBRARIES  "" CACHE STRING "Override: e.g. -lssl;-lcrypto")

# ===== Include paths =====
include_directories(
  ${CMAKE_SOURCE_DIR}/Kama-HTTPServer-main/HttpServer/include
  ${CMAKE_SOURCE_DIR}/Kama-HTTPServer-main/HttpCache/include
  ${CMAKE_SOURCE_DIR}/apps/url_shortener/include
)

# ===== Collect sources =====
# HttpServer 源码，排除 utils/db（避免未装 mysqlcppconn 时强行链接）
file(GLOB_RECURSE HTTP_SERVER_DB_SRC
  ${CMAKE_SOURCE_DIR}/Kama-HTTPServer-main/HttpServer/src/utils/db/*.cpp
)
file(GLOB_RECURSE HTTP_SERVER_SRC
  ${CMAKE_SOURCE_DIR}/Kama-HTTPServer-main/HttpServer/src/*.cpp
)
if (HTTP_SERVER_DB_SRC)
  list(REMOVE_ITEM HTTP_SERVER_SRC ${HTTP_SERVER_DB_SRC})
endif()

file(GLOB_RECURSE HTTP_CACHE_SRC
  ${CMAKE_SOURCE_DIR}/Kama-HTTPServer-main/HttpCache/src/*.cpp
)

add_executable(url_shortener
  ${HTTP_SERVER_SRC}
  ${HTTP_CACHE_SRC}
)

# apps/url_shortener 会通过 target_sources() 给 url_shortener 加上 main/handlers
add_subdirectory(apps/url_shortener)

# ===== Core deps: Threads + OpenSSL + muduo =====
find_package(Threads REQUIRED)

# OpenSSL：若未覆盖 OPENSSL_LIBRARIES，则用 CMake 包
if (OPENSSL_LIBRARIES STREQUAL "")
  find_package(OpenSSL REQUIRED)  # 提供 OpenSSL::SSL OpenSSL::Crypto
  set(OPENSSL_LIBRARIES OpenSSL::SSL OpenSSL::Crypto)
endif()

# muduo：若未覆盖 MUDUO_LIBRARIES，则自动查找
if (MUDUO_LIBRARIES STREQUAL "")
  find_library(MUDUO_NET  muduo_net)
  find_library(MUDUO_BASE muduo_base)
  if (NOT MUDUO_NET OR NOT MUDUO_BASE)
    message(FATAL_ERROR
      "muduo not found. Install libmuduo-dev, or pass "
      "-DMUDUO_LIBRARIES='-lmuduo_net;-lmuduo_base;-lpthread'")
  endif()
  set(MUDUO_LIBRARIES ${MUDUO_NET} ${MUDUO_BASE} Threads::Threads)
endif()

target_link_libraries(url_shortener
  PRIVATE
    ${MUDUO_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    Threads::Threads
)

# ===== Optional: Redis (hiredis) =====
if (WITH_REDIS)
  find_library(HIREDIS_LIB hiredis)
  if (NOT HIREDIS_LIB)
    message(FATAL_ERROR "WITH_REDIS=ON but hiredis not found (apt: libhiredis-dev)")
  endif()
  target_compile_definitions(url_shortener PRIVATE WITH_REDIS)
  target_link_libraries(url_shortener PRIVATE ${HIREDIS_LIB})
endif()

# ===== Optional: MySQL (Connector/C++) =====
if (WITH_MYSQL)
  # 一些发行版叫 mysqlcppconn8
  find_library(MYSQLCPP_LIB NAMES mysqlcppconn mysqlcppconn8)
  if (NOT MYSQLCPP_LIB)
    message(FATAL_ERROR "WITH_MYSQL=ON but mysqlcppconn not found (apt: libmysqlcppconn-dev)")
  endif()
  target_compile_definitions(url_shortener PRIVATE WITH_MYSQL)
  target_link_libraries(url_shortener PRIVATE ${MYSQLCPP_LIB})
endif()

# 运行时 rpath 指向相对路径，便于本地运行
set_target_properties(url_shortener PROPERTIES BUILD_RPATH_USE_ORIGIN ON)
include(${CMAKE_SOURCE_DIR}/cmake/add_cache_aside_target.cmake)