#include "logstory/analysis/event_index.hpp"
#include <algorithm>

namespace logstory::analysis {

void EventIndex::build(const std::vector<core::Event>& events) {
    events_ = events;
    
    // Clear existing indices
    severity_index_.clear();
    correlation_index_.clear();
    time_index_.clear();
    
    // Build indices
    for (const auto& event : events_) {
        // Index by severity
        severity_index_[event.sev].push_back(event.id);
        
        // Index by correlation IDs
        auto req_it = event.tags.find("request_id");
        if (req_it != event.tags.end()) {
            correlation_index_[req_it->second].push_back(event.id);
        }
        
        auto trace_it = event.tags.find("trace_id");
        if (trace_it != event.tags.end()) {
            correlation_index_[trace_it->second].push_back(event.id);
        }
        
        auto uuid_it = event.tags.find("uuid");
        if (uuid_it != event.tags.end()) {
            correlation_index_[uuid_it->second].push_back(event.id);
        }
        
        // Index by time (if timestamp available)
        if (event.ts.has_value() && event.ts->is_valid()) {
            int64_t bucket = time_to_bucket(event.ts->tp);
            time_index_[bucket].push_back(event.id);
        }
    }
}

std::vector<core::EventId> EventIndex::get_by_severity(core::Severity sev) const {
    auto it = severity_index_.find(sev);
    if (it != severity_index_.end()) {
        return it->second;
    }
    return {};
}

std::vector<core::EventId> EventIndex::get_by_correlation_id(const std::string& corr_id) const {
    auto it = correlation_index_.find(corr_id);
    if (it != correlation_index_.end()) {
        return it->second;
    }
    return {};
}

std::vector<core::EventId> EventIndex::get_by_time_range(TimePoint start, TimePoint end) const {
    std::vector<core::EventId> result;
    
    int64_t start_bucket = time_to_bucket(start);
    int64_t end_bucket = time_to_bucket(end);
    
    // Iterate through buckets in range
    auto it = time_index_.lower_bound(start_bucket);
    auto end_it = time_index_.upper_bound(end_bucket);
    
    for (; it != end_it; ++it) {
        // Add all events from this bucket
        result.insert(result.end(), it->second.begin(), it->second.end());
    }
    
    return result;
}

size_t EventIndex::count_by_severity(core::Severity sev) const {
    auto it = severity_index_.find(sev);
    if (it != severity_index_.end()) {
        return it->second.size();
    }
    return 0;
}

int64_t EventIndex::time_to_bucket(TimePoint tp) const {
    // Convert to minutes since epoch
    auto duration = tp.time_since_epoch();
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
    return minutes.count();
}

} // namespace logstory::analysis
