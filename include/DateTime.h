#pragma once

#include <cstdint>
#include <string>

namespace timetracker {

using Timestamp = std::int64_t;

Timestamp now();
bool parseDateTime(const std::string& text, Timestamp& result, std::string& error);
std::string formatDateTime(Timestamp value);
std::string formatUtcIso(Timestamp value);
bool parseUtcIso(const std::string& text, Timestamp& result);
std::string formatDuration(Timestamp seconds);

}
