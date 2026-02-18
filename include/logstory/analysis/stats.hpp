#pragma once

#include "logstory/core/event.hpp"
#include "logstory/core/severity.hpp"
#include <chrono>
#include <map>
#include <string>
#include <vector>

namespace logstory::analysis {

// Time-series data point
struct TimeSeriesPoint {
    std::chrono::system_clock::time_point timestamp;
    size_t count;
    
    TimeSeriesPoint() : count(0) {}
    TimeSeriesPoint(std::chrono::system_clock::time_point tp, size_t c) 
        : timestamp(tp), count(c) {}
};

// Time-series data structure
struct TimeSeries {
    std::vector<TimeSeriesPoint> points;
    std::chrono::minutes bucket_size;
    
    TimeSeries() : bucket_size(1) {}
    explicit TimeSeries(std::chrono::minutes bs) : bucket_size(bs) {}
    
    void add_event(std::chrono::system_clock::time_point tp);
    size_t total_count() const;
    std::optional<TimeSeriesPoint> max_point() const;
};

// Frequent message pattern
struct FrequentPattern {
    std::string pattern;
    size_t count;
    core::Severity max_severity;
    
    FrequentPattern() : count(0), max_severity(core::Severity::UNKNOWN) {}
    FrequentPattern(const std::string& p, size_t c, core::Severity s)
        : pattern(p), count(c), max_severity(s) {}
};

// Overall statistics
struct Stats {
    // Severity counts
    std::map<core::Severity, size_t> severity_counts;
    
    // Time series by severity
    std::map<core::Severity, TimeSeries> severity_time_series;
    
    // Source file counts
    std::map<std::string, size_t> source_counts;
    
    // Frequent patterns (top N)
    std::vector<FrequentPattern> frequent_patterns;
    
    // Overall metrics
    size_t total_events = 0;
    std::optional<std::chrono::system_clock::time_point> start_time;
    std::optional<std::chrono::system_clock::time_point> end_time;
    
    Stats() = default;
    
    // Helper methods
    size_t error_count() const;
    size_t warn_count() const;
    double error_rate() const; // Errors per total events
};

} // namespace logstory::analysis
