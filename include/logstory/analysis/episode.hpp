#pragma once

#include "logstory/core/event.hpp"
#include "logstory/core/time.hpp"
#include <vector>
#include <optional>
#include <string>

namespace logstory::analysis {

/// Represents a coherent sequence of related log events (a "story chunk")
struct Episode {
    using TimePoint = std::chrono::system_clock::time_point;
    
    /// Unique episode ID
    uint64_t id;
    
    /// IDs of events in this episode (in order)
    std::vector<core::EventId> event_ids;
    
    /// Start and end timestamps (if available)
    std::optional<TimePoint> start_time;
    std::optional<TimePoint> end_time;
    
    /// Shared correlation IDs across events in this episode
    std::vector<std::string> correlation_ids;
    
    /// Key event IDs that are highlights of this episode
    /// (first error, max severity event, etc.)
    std::vector<core::EventId> highlights;
    
    /// Maximum severity level in this episode
    core::Severity max_severity;
    
    Episode() : id(0), max_severity(core::Severity::UNKNOWN) {}
    
    explicit Episode(uint64_t episode_id)
        : id(episode_id), max_severity(core::Severity::UNKNOWN) {}
    
    /// Get the number of events in this episode
    size_t size() const { return event_ids.size(); }
    
    /// Check if episode is empty
    bool empty() const { return event_ids.empty(); }
};

} // namespace logstory::analysis
