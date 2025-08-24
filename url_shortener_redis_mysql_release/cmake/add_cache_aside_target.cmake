# Adds a new executable target "url_shortener_cache_aside" without touching the original one.
# This target reuses HTTP_SERVER_SRC / HTTP_CACHE_SRC collected by the top-level CMakeLists.

add_executable(url_shortener_cache_aside
  ${HTTP_SERVER_SRC}
  ${HTTP_CACHE_SRC}
  ${CMAKE_SOURCE_DIR}/apps/url_shortener/src/RouterSetup.cpp
  ${CMAKE_SOURCE_DIR}/apps/url_shortener/src/main_cache_aside.cpp
)

target_include_directories(url_shortener_cache_aside PRIVATE
  ${CMAKE_SOURCE_DIR}/apps/url_shortener/include
  ${CMAKE_SOURCE_DIR}/Kama-HTTPServer-main/HttpServer/include
  ${CMAKE_SOURCE_DIR}/Kama-HTTPServer-main/HttpCache/include
)

target_link_libraries(url_shortener_cache_aside
  ${MUDUO_LIBRARIES}
  ${OPENSSL_LIBRARIES}
)

if (WITH_REDIS)
  find_library(HIREDIS_LIB hiredis)
  if (NOT HIREDIS_LIB)
    message(FATAL_ERROR "WITH_REDIS=ON but hiredis not found")
  endif()
  target_compile_definitions(url_shortener_cache_aside PRIVATE WITH_REDIS)
  target_link_libraries(url_shortener_cache_aside ${HIREDIS_LIB})
endif()

if (WITH_MYSQL)
  find_library(MYSQLCPP_LIB NAMES mysqlcppconn mysqlcppconn8)
  if (NOT MYSQLCPP_LIB)
    message(FATAL_ERROR "WITH_MYSQL=ON but MySQL C++ connector not found (libmysqlcppconn or libmysqlcppconn8)")
  endif()
  target_compile_definitions(url_shortener_cache_aside PRIVATE WITH_MYSQL)
  target_link_libraries(url_shortener_cache_aside ${MYSQLCPP_LIB})
endif()

set_target_properties(url_shortener_cache_aside PROPERTIES BUILD_RPATH_USE_ORIGIN ON)
