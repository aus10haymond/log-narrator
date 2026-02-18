#include "logstory/analysis/stats.hpp"
#include <algorithm>

namespace logstory::analysis {

void TimeSeries::add_event(std::chrono::system_clock::time_point tp) {
    // Round down to bucket boundary
    auto epoch = tp.time_since_epoch();
    auto bucket_duration = std::chrono::duration_cast<std::chrono::minutes>(bucket_size);
    auto bucket_count = epoch / bucket_duration;
    auto bucket_start = std::chrono::system_clock::time_point(bucket_count * bucket_duration);
    
    // Find or create bucket
    auto it = std::find_if(points.begin(), points.end(), 
        [&](const TimeSeriesPoint& p) { return p.timestamp == bucket_start; });
    
    if (it != points.end()) {
        it->count++;
    } else {
        points.push_back(TimeSeriesPoint(bucket_start, 1));
    }
}

size_t TimeSeries::total_count() const {
    size_t total = 0;
    for (const auto& point : points) {
        total += point.count;
    }
    return total;
}

std::optional<TimeSeriesPoint> TimeSeries::max_point() const {
    if (points.empty()) {
        return std::nullopt;
    }
    
    auto max_it = std::max_element(points.begin(), points.end(),
        [](const TimeSeriesPoint& a, const TimeSeriesPoint& b) {
            return a.count < b.count;
        });
    
    return *max_it;
}

size_t Stats::error_count() const {
    auto it = severity_counts.find(core::Severity::ERROR);
    return (it != severity_counts.end()) ? it->second : 0;
}

size_t Stats::warn_count() const {
    auto it = severity_counts.find(core::Severity::WARN);
    return (it != severity_counts.end()) ? it->second : 0;
}

double Stats::error_rate() const {
    if (total_events == 0) {
        return 0.0;
    }
    return static_cast<double>(error_count()) / static_cast<double>(total_events);
}

} // namespace logstory::analysis
