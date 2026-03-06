#include "logstory/analysis/window.hpp"
#include <iomanip>
#include <sstream>
#include <regex>
#include <cctype>

namespace logstory::analysis {

bool TimeWindow::contains(const core::Event& event) const {
    if (!event.ts.has_value()) {
        // Events without timestamps are included if window is unconstrained
        return !is_constrained();
    }
    
    return contains(event.ts->tp);
}

bool TimeWindow::contains(const std::chrono::system_clock::time_point& tp) const {
    if (start.has_value() && tp < *start) {
        return false;
    }
    
    if (end.has_value() && tp > *end) {
        return false;
    }
    
    return true;
}

std::optional<std::chrono::system_clock::time_point> parse_iso8601(const std::string& str) {
    // Try to parse ISO8601 format: 2024-03-06T10:30:45Z or similar
    std::istringstream ss(str);
    std::tm tm = {};
    
    // Try different formats
    std::vector<std::string> formats = {
        "%Y-%m-%dT%H:%M:%SZ",      // 2024-03-06T10:30:45Z
        "%Y-%m-%d %H:%M:%S",        // 2024-03-06 10:30:45
        "%Y-%m-%dT%H:%M:%S",        // 2024-03-06T10:30:45
        "%Y-%m-%d",                 // 2024-03-06
    };
    
    for (const auto& fmt : formats) {
        ss.clear();
        ss.str(str);
        ss >> std::get_time(&tm, fmt.c_str());
        if (!ss.fail()) {
            auto time = std::mktime(&tm);
            return std::chrono::system_clock::from_time_t(time);
        }
    }
    
    return std::nullopt;
}

std::optional<std::chrono::system_clock::time_point> parse_relative_time(const std::string& str) {
    // Parse relative time like "1h", "30m", "2d", "1w"
    std::regex rel_regex(R"((\d+)([smhdw]))");
    std::smatch match;
    
    if (!std::regex_match(str, match, rel_regex)) {
        return std::nullopt;
    }
    
    int value = std::stoi(match[1].str());
    char unit = match[2].str()[0];
    
    auto now = std::chrono::system_clock::now();
    
    switch (unit) {
        case 's': // seconds
            return now - std::chrono::seconds(value);
        case 'm': // minutes
            return now - std::chrono::minutes(value);
        case 'h': // hours
            return now - std::chrono::hours(value);
        case 'd': // days
            return now - std::chrono::hours(value * 24);
        case 'w': // weeks
            return now - std::chrono::hours(value * 24 * 7);
        default:
            return std::nullopt;
    }
}

std::optional<std::chrono::system_clock::time_point> parse_time(const std::string& str) {
    // Try ISO8601 first
    auto result = parse_iso8601(str);
    if (result.has_value()) {
        return result;
    }
    
    // Try relative time
    return parse_relative_time(str);
}

std::vector<core::Event> filter_by_window(const std::vector<core::Event>& events,
                                           const TimeWindow& window) {
    if (!window.is_constrained()) {
        return events;  // No filtering needed
    }
    
    std::vector<core::Event> filtered;
    filtered.reserve(events.size());
    
    for (const auto& event : events) {
        if (window.contains(event)) {
            filtered.push_back(event);
        }
    }
    
    return filtered;
}

} // namespace logstory::analysis
