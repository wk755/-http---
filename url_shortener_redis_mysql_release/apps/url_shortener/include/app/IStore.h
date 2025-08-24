#pragma once
#include <string>
#include <optional>
namespace app {
class IStore {
public:
  virtual ~IStore() = default;
  virtual std::optional<std::string> get(const std::string& key) = 0;
  virtual bool setNX(const std::string& key, const std::string& value, int ttlSec = 0) = 0;
  virtual void set(const std::string& key, const std::string& value, int ttlSec = 0) = 0;
  virtual void del(const std::string& key) = 0;
}; } // namespace app
