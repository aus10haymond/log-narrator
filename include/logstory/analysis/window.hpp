#pragma once

#include "logstory/core/event.hpp"
#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace logstory::analysis {

/// Time window for filtering events
struct TimeWindow {
    std::optional<std::chrono::system_clock::time_point> start;
    std::optional<std::chrono::system_clock::time_point> end;
    
    TimeWindow() = default;
    TimeWindow(std::optional<std::chrono::system_clock::time_point> s,
               std::optional<std::chrono::system_clock::time_point> e)
        : start(s), end(e) {}
    
    /// Check if an event falls within this window
    bool contains(const core::Event& event) const;
    
    /// Check if a timestamp falls within this window
    bool contains(const std::chrono::system_clock::time_point& tp) const;
    
    /// Check if window has any constraints
    bool is_constrained() const { return start.has_value() || end.has_value(); }
};

/// Parse ISO8601 timestamp string
std::optional<std::chrono::system_clock::time_point> parse_iso8601(const std::string& str);

/// Parse relative time like "1h", "30m", "2d" from now
std::optional<std::chrono::system_clock::time_point> parse_relative_time(const std::string& str);

/// Parse time string (tries ISO8601 first, then relative)
std::optional<std::chrono::system_clock::time_point> parse_time(const std::string& str);

/// Filter events by time window
std::vector<core::Event> filter_by_window(const std::vector<core::Event>& events,
                                           const TimeWindow& window);

} // namespace logstory::analysis
