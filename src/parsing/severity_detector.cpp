#include "logstory/parsing/severity_detector.hpp"
#include <regex>
#include <algorithm>

namespace logstory::parsing {

core::Severity SeverityDetector::detect(const std::string& text) {
    // Try methods in order of reliability
    
    // 1. Explicit markers (most reliable)
    if (auto sev = try_explicit_markers(text); sev != core::Severity::UNKNOWN) {
        return sev;
    }
    
    // 2. Key-value patterns
    if (auto sev = try_kv_patterns(text); sev != core::Severity::UNKNOWN) {
        return sev;
    }
    
    // 3. Keyword scoring (fallback, lower confidence)
    return try_keyword_scoring(text);
}

core::Severity SeverityDetector::try_explicit_markers(const std::string& text) {
    // Try common bracket formats: [ERROR], [WARN], etc.
    std::regex bracket_regex(R"(\[(TRACE|DEBUG|INFO|WARN|WARNING|ERROR|ERR|FATAL|CRITICAL|SEVERE)\])",
                            std::regex_constants::icase);
    std::smatch match;
    if (std::regex_search(text, match, bracket_regex)) {
        return core::severity_from_string(match[1].str());
    }
    
    // Try space-separated at start: "ERROR: message" or "ERROR message"
    std::regex start_regex(R"(^\s*(TRACE|DEBUG|INFO|WARN|WARNING|ERROR|ERR|FATAL|CRITICAL|SEVERE)[\s:])",
                          std::regex_constants::icase);
    if (std::regex_search(text, match, start_regex)) {
        return core::severity_from_string(match[1].str());
    }
    
    // Try JSON-like field: "level":"error", "severity":"warn"
    std::regex json_regex(R"(["'](?:level|severity)["']\s*:\s*["'](TRACE|DEBUG|INFO|WARN|WARNING|ERROR|ERR|FATAL|CRITICAL)["'])",
                         std::regex_constants::icase);
    if (std::regex_search(text, match, json_regex)) {
        return core::severity_from_string(match[1].str());
    }
    
    return core::Severity::UNKNOWN;
}

core::Severity SeverityDetector::try_kv_patterns(const std::string& text) {
    // Match key=value patterns: level=error, severity=warn, etc.
    std::regex kv_regex(R"(\b(level|severity|log_level|loglevel)\s*=\s*(\w+)\b)",
                       std::regex_constants::icase);
    
    std::smatch match;
    if (std::regex_search(text, match, kv_regex)) {
        std::string value = match[2].str();
        auto sev = core::severity_from_string(value);
        if (sev != core::Severity::UNKNOWN) {
            return sev;
        }
    }
    
    return core::Severity::UNKNOWN;
}

core::Severity SeverityDetector::try_keyword_scoring(const std::string& text) {
    // Convert to lowercase for case-insensitive matching
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    // Check for strong error indicators
    static const std::vector<std::string> fatal_keywords = {
        "fatal", "critical", "panic", "abort"
    };
    for (const auto& keyword : fatal_keywords) {
        if (lower_text.find(keyword) != std::string::npos) {
            return core::Severity::FATAL;
        }
    }
    
    // Check for error keywords
    static const std::vector<std::string> error_keywords = {
        "error", "exception", "failed", "failure", "err:"
    };
    for (const auto& keyword : error_keywords) {
        if (lower_text.find(keyword) != std::string::npos) {
            return core::Severity::ERROR;
        }
    }
    
    // Check for warning keywords
    static const std::vector<std::string> warn_keywords = {
        "warn", "warning", "deprecated"
    };
    for (const auto& keyword : warn_keywords) {
        if (lower_text.find(keyword) != std::string::npos) {
            return core::Severity::WARN;
        }
    }
    
    // Check for debug keywords  
    if (lower_text.find("debug") != std::string::npos ||
        lower_text.find("trace") != std::string::npos) {
        return core::Severity::DEBUG;
    }
    
    // Default to INFO if we see common info indicators
    if (lower_text.find("info") != std::string::npos ||
        lower_text.find("start") != std::string::npos ||
        lower_text.find("complete") != std::string::npos) {
        return core::Severity::INFO;
    }
    
    return core::Severity::UNKNOWN;
}

} // namespace logstory::parsing
