#pragma once

#include "logstory/core/event.hpp"
#include "logstory/core/severity.hpp"
#include <vector>
#include <unordered_map>
#include <map>
#include <chrono>

namespace logstory::analysis {

/// Index for fast event queries by time, severity, and correlation IDs
class EventIndex {
public:
    using TimePoint = std::chrono::system_clock::time_point;
    using TimeBucket = std::chrono::minutes;
    
    /// Build index from a list of events
    void build(const std::vector<core::Event>& events);
    
    /// Get all events in the index
    const std::vector<core::Event>& get_all_events() const { return events_; }
    
    /// Get events by severity
    std::vector<core::EventId> get_by_severity(core::Severity sev) const;
    
    /// Get events by correlation ID (request_id or trace_id)
    std::vector<core::EventId> get_by_correlation_id(const std::string& corr_id) const;
    
    /// Get events in a time range (if timestamps available)
    std::vector<core::EventId> get_by_time_range(TimePoint start, TimePoint end) const;
    
    /// Get count by severity
    size_t count_by_severity(core::Severity sev) const;
    
private:
    std::vector<core::Event> events_;
    
    // Severity index: Severity -> list of event IDs
    std::unordered_map<core::Severity, std::vector<core::EventId>> severity_index_;
    
    // Correlation index: correlation_id -> list of event IDs
    std::unordered_map<std::string, std::vector<core::EventId>> correlation_index_;
    
    // Time index: time bucket -> list of event IDs
    std::map<int64_t, std::vector<core::EventId>> time_index_;
    
    /// Convert time_point to bucket key (minutes since epoch)
    int64_t time_to_bucket(TimePoint tp) const;
};

} // namespace logstory::analysis
