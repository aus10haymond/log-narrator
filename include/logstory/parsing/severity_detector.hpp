#pragma once

#include "logstory/core/severity.hpp"
#include <string>

namespace logstory::parsing {

/// Detects severity level from log text using pattern matching and heuristics
class SeverityDetector {
public:
    /// Detect severity from text
    /// Returns UNKNOWN if no clear severity indicators found
    core::Severity detect(const std::string& text);

private:
    /// Try to find explicit severity markers (brackets, JSON fields, etc.)
    core::Severity try_explicit_markers(const std::string& text);
    
    /// Try key=value patterns (level=error, severity=warn, etc.)
    core::Severity try_kv_patterns(const std::string& text);
    
    /// Use keyword heuristics as fallback (careful with false positives)
    core::Severity try_keyword_scoring(const std::string& text);
};

} // namespace logstory::parsing
