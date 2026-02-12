#include "logstory/core/severity.hpp"
#include <algorithm>
#include <cctype>

namespace logstory::core {

std::string to_string(Severity sev) {
    switch (sev) {
        case Severity::TRACE:   return "TRACE";
        case Severity::DEBUG:   return "DEBUG";
        case Severity::INFO:    return "INFO";
        case Severity::WARN:    return "WARN";
        case Severity::ERROR:   return "ERROR";
        case Severity::FATAL:   return "FATAL";
        case Severity::UNKNOWN:
        default:                return "UNKNOWN";
    }
}

Severity severity_from_string(const std::string& str) {
    // Convert to uppercase for comparison
    std::string upper = str;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    
    if (upper == "TRACE" || upper == "VERBOSE")
        return Severity::TRACE;
    if (upper == "DEBUG" || upper == "DBG")
        return Severity::DEBUG;
    if (upper == "INFO" || upper == "INFORMATION")
        return Severity::INFO;
    if (upper == "WARN" || upper == "WARNING")
        return Severity::WARN;
    if (upper == "ERROR" || upper == "ERR")
        return Severity::ERROR;
    if (upper == "FATAL" || upper == "CRITICAL" || upper == "SEVERE")
        return Severity::FATAL;
    
    return Severity::UNKNOWN;
}

} // namespace logstory::core
