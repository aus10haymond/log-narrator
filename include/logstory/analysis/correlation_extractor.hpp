#pragma once

#include "logstory/core/event.hpp"
#include <string>
#include <vector>

namespace logstory::analysis {

/// Extracts correlation IDs from events (request_id, trace_id, UUIDs)
class CorrelationExtractor {
public:
    /// Extract correlation IDs from an event and normalize them into tags
    /// Updates event.tags with normalized correlation IDs
    void extract(core::Event& event);

private:
    /// Extract request ID variants and normalize to "request_id"
    void extract_request_id(core::Event& event);
    
    /// Extract trace ID variants and normalize to "trace_id"
    void extract_trace_id(core::Event& event);
    
    /// Extract UUIDs from text
    std::vector<std::string> extract_uuids(const std::string& text);
    
    /// Check if a string looks like a valid UUID
    bool is_uuid(const std::string& str);
};

} // namespace logstory::analysis
