#pragma once

#include "logstory/core/event.hpp"
#include "logstory/core/severity.hpp"
#include <string>
#include <vector>

namespace logstory::rules {

// Severity/priority of a finding
enum class FindingSeverity {
    LOW,      // Minor issues or observations
    MEDIUM,   // Notable problems that may need attention
    HIGH,     // Serious issues that likely caused failures
    CRITICAL  // Definite root causes or critical failures
};

// Evidence reference to specific events
struct Evidence {
    core::EventId event_id;
    std::string description;  // Why this event is relevant
    
    Evidence() : event_id(0) {}
    Evidence(core::EventId id, const std::string& desc)
        : event_id(id), description(desc) {}
};

// A finding represents a detected pattern or issue
struct Finding {
    std::string id;                    // Unique identifier (e.g., "crash-loop-001")
    std::string title;                 // Short title (e.g., "Crash Loop Detected")
    std::string summary;               // Human-readable summary
    FindingSeverity severity;
    double confidence;                 // 0.0 to 1.0
    std::vector<Evidence> evidence;    // Supporting events
    
    // Optional metadata
    std::optional<std::chrono::system_clock::time_point> start_time;
    std::optional<std::chrono::system_clock::time_point> end_time;
    
    Finding() : severity(FindingSeverity::MEDIUM), confidence(0.0) {}
    
    // Helper to add evidence
    void add_evidence(core::EventId event_id, const std::string& description) {
        evidence.emplace_back(event_id, description);
    }
};

// Convert severity to string
inline std::string to_string(FindingSeverity sev) {
    switch (sev) {
        case FindingSeverity::LOW: return "LOW";
        case FindingSeverity::MEDIUM: return "MEDIUM";
        case FindingSeverity::HIGH: return "HIGH";
        case FindingSeverity::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

} // namespace logstory::rules
