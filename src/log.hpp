#pragma once

#include <string>
#include <string_view>

namespace crossroads::log {

enum class Level { Info, Warn, Error };

void init(const std::wstring& log_file_path);
void shutdown();
void write(Level level, std::string_view message);
void info(std::string_view message);
void warn(std::string_view message);
void error(std::string_view message);

// Optional UI sink: called on the thread that logs (UI should PostMessage if needed).
using Sink = void (*)(Level, std::string_view);
void set_sink(Sink sink);

}  // namespace crossroads::log
