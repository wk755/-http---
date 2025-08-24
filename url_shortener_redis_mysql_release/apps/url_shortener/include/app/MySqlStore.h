#pragma once
#ifdef WITH_MYSQL
#include "app/IStore.h"
#include <memory>
#include <string>
#include <optional>
#include <stdexcept>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
namespace app {
class MySqlStore : public IStore {
  std::unique_ptr<sql::Connection> conn_;
public:
  MySqlStore(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema) {
    sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
    conn_.reset(driver->connect(url, user, pass)); conn_->setSchema(schema); init();
  }
  void init(){
    std::unique_ptr<sql::PreparedStatement> stmt(
      conn_->prepareStatement("CREATE TABLE IF NOT EXISTS short_url("
                              "id VARCHAR(32) PRIMARY KEY,"
                              "long_url TEXT NOT NULL,"
                              "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                              "ttl_sec INT NULL)"));
    stmt->execute();
  }
  std::optional<std::string> get(const std::string& key) override {
    std::unique_ptr<sql::PreparedStatement> stmt(
      conn_->prepareStatement("SELECT long_url, ttl_sec, UNIX_TIMESTAMP(created_at) FROM short_url WHERE id=?"));
    stmt->setString(1, key);
    std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery());
    if (rs->next()){
      auto url = rs->getString(1); int ttl = rs->isNull(2)? 0 : rs->getInt(2); long created = rs->getInt64(3);
      if (ttl>0){ long now = time(nullptr); if (now > created + ttl) return std::nullopt; }
      return std::make_optional(url);
    }
    return std::nullopt;
  }
  bool setNX(const std::string& key, const std::string& value, int ttlSec=0) override {
    try{
      std::unique_ptr<sql::PreparedStatement> stmt(
        conn_->prepareStatement("INSERT INTO short_url(id,long_url,ttl_sec) VALUES(?,?,?)"));
      stmt->setString(1, key); stmt->setString(2, value);
      if (ttlSec>0) stmt->setInt(3, ttlSec); else stmt->setNull(3, 0);
      stmt->execute(); return true;
    } catch (sql::SQLException&){ return false; }
  }
  void set(const std::string& key, const std::string& value, int ttlSec=0) override {
    std::unique_ptr<sql::PreparedStatement> stmt(
      conn_->prepareStatement("REPLACE INTO short_url(id,long_url,ttl_sec) VALUES(?,?,?)"));
    stmt->setString(1, key); stmt->setString(2, value);
    if (ttlSec>0) stmt->setInt(3, ttlSec); else stmt->setNull(3, 0); stmt->execute();
  }
  void del(const std::string& key) override {
    std::unique_ptr<sql::PreparedStatement> stmt(conn_->prepareStatement("DELETE FROM short_url WHERE id=?")); stmt->setString(1, key); stmt->execute();
  }
}; } // namespace app
#endif
