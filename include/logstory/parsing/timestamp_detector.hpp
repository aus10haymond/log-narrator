#pragma once

#include "logstory/core/time.hpp"
#include <string>
#include <optional>

namespace logstory::parsing {

/// Detects and parses timestamps from log text
class TimestampDetector {
public:
    /// Try to detect and parse a timestamp from text
    /// Returns timestamp with confidence if found
    std::optional<core::Timestamp> detect(const std::string& text);

private:
    /// Try ISO 8601 format (YYYY-MM-DD, YYYY-MM-DDTHH:MM:SS, etc.)
    std::optional<core::Timestamp> try_iso8601(const std::string& text);
    
    /// Try syslog format (Mon DD HH:MM:SS)
    std::optional<core::Timestamp> try_syslog(const std::string& text);
    
    /// Try epoch seconds/milliseconds
    std::optional<core::Timestamp> try_epoch(const std::string& text);
    
    /// Try common date-time patterns (YYYY/MM/DD HH:MM:SS, etc.)
    std::optional<core::Timestamp> try_common_patterns(const std::string& text);
};

} // namespace logstory::parsing
