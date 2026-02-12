#pragma once

#include "logstory/core/event_id.hpp"
#include "logstory/core/time.hpp"
#include "logstory/core/severity.hpp"
#include "logstory/core/source_ref.hpp"
#include "logstory/core/tags.hpp"
#include <string>
#include <optional>

namespace logstory::core {

/// Canonical event representation - the core data structure for log analysis
struct Event {
    EventId id;                      // Unique identifier
    std::optional<Timestamp> ts;     // Parsed timestamp (if available)
    Severity sev;                    // Detected severity level
    std::string message;             // Extracted log message
    SourceRef src;                   // Source location (file:line)
    TagMap tags;                     // Extracted metadata fields
    std::string raw;                 // Original raw text (preserved for evidence)
    
    Event()
        : id(0), sev(Severity::UNKNOWN) {}
    
    Event(EventId event_id, SourceRef source)
        : id(event_id), sev(Severity::UNKNOWN), src(std::move(source)) {}
};

} // namespace logstory::core
