#pragma once

#include <string>

namespace logstory::core {

/// Log severity levels
enum class Severity {
    UNKNOWN = 0,
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

/// Convert severity to string
std::string to_string(Severity sev);

/// Convert string to severity (case-insensitive)
Severity severity_from_string(const std::string& str);

} // namespace logstory::core
