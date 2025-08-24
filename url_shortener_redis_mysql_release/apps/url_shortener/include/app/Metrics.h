#pragma once
#include <atomic>
#include <string>
#include <sstream>
namespace app {
struct Metrics {
  std::atomic<uint64_t> shorten_ok{0}, shorten_bad{0};
  std::atomic<uint64_t> redirect_ok{0}, redirect_miss{0};
  std::atomic<uint64_t> ratelimit_drops{0};
  std::atomic<uint64_t> cache_hits{0}, cache_sets{0};
  // store metrics
  std::atomic<uint64_t> store_get_hit{0}, store_get_miss{0};
  std::atomic<uint64_t> store_setnx_ok{0}, store_setnx_conflict{0};

  std::string prometheus() const {
    std::ostringstream os;
    os << "# HELP url_shortener_store_get_hit Store GET hits\n"
       << "# TYPE url_shortener_store_get_hit counter\n"
       << "url_shortener_store_get_hit " << store_get_hit.load() << "\n"
       << "# HELP url_shortener_store_get_miss Store GET misses\n"
       << "# TYPE url_shortener_store_get_miss counter\n"
       << "url_shortener_store_get_miss " << store_get_miss.load() << "\n"
       << "# HELP url_shortener_store_setnx_ok Store SETNX successes\n"
       << "# TYPE url_shortener_store_setnx_ok counter\n"
       << "url_shortener_store_setnx_ok " << store_setnx_ok.load() << "\n"
       << "# HELP url_shortener_store_setnx_conflict Store SETNX conflicts\n"
       << "# TYPE url_shortener_store_setnx_conflict counter\n"
       << "url_shortener_store_setnx_conflict " << store_setnx_conflict.load() << "\n"
       << "# HELP url_shortener_shorten_ok Total shorten success\n"
       << "# TYPE url_shortener_shorten_ok counter\n"
       << "url_shortener_shorten_ok " << shorten_ok.load() << "\n"
       << "# HELP url_shortener_redirect_ok Total redirects\n"
       << "# TYPE url_shortener_redirect_ok counter\n"
       << "url_shortener_redirect_ok " << redirect_ok.load() << "\n"
       << "url_shortener_redirect_miss " << redirect_miss.load() << "\n"
       << "url_shortener_shorten_bad " << shorten_bad.load() << "\n"
       << "url_shortener_ratelimit_drops " << ratelimit_drops.load() << "\n"
       << "url_shortener_cache_hits " << cache_hits.load() << "\n"
       << "url_shortener_cache_sets " << cache_sets.load() << "\n";
    return os.str();
  }
}; } // namespace app
