#include "log.hpp"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace crossroads::log {
namespace {

std::mutex g_mu;
std::ofstream g_file;
Sink g_sink = nullptr;

const char* level_name(Level level) {
  switch (level) {
    case Level::Info:
      return "INFO";
    case Level::Warn:
      return "WARN";
    case Level::Error:
      return "ERROR";
  }
  return "INFO";
}

std::string timestamp() {
  using clock = std::chrono::system_clock;
  const auto now = clock::now();
  const auto t = clock::to_time_t(now);
  std::tm tm{};
  localtime_s(&tm, &t);
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

}  // namespace

void init(const std::wstring& log_file_path) {
  std::lock_guard lock(g_mu);
  g_file.open(log_file_path, std::ios::out | std::ios::app);
}

void shutdown() {
  std::lock_guard lock(g_mu);
  if (g_file.is_open()) {
    g_file.flush();
    g_file.close();
  }
}

void set_sink(Sink sink) {
  std::lock_guard lock(g_mu);
  g_sink = sink;
}

void write(Level level, std::string_view message) {
  const std::string line = timestamp() + " [" + level_name(level) + "] " + std::string(message);
  Sink sink = nullptr;
  {
    std::lock_guard lock(g_mu);
    if (g_file.is_open()) {
      g_file << line << '\n';
      g_file.flush();
    }
    sink = g_sink;
  }
  if (sink) {
    sink(level, message);
  }
}

void info(std::string_view message) { write(Level::Info, message); }
void warn(std::string_view message) { write(Level::Warn, message); }
void error(std::string_view message) { write(Level::Error, message); }

}  // namespace crossroads::log
