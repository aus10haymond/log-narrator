#pragma once

#include "logstory/rules/finding.hpp"
#include "logstory/core/event.hpp"
#include "logstory/analysis/stats.hpp"
#include <string>
#include <vector>
#include <chrono>

namespace logstory::narrative {

// Executive summary bullet point
struct SummaryBullet {
    std::string text;
    
    SummaryBullet() = default;
    explicit SummaryBullet(const std::string& t) : text(t) {}
};

// Timeline highlight entry
struct TimelineHighlight {
    std::chrono::system_clock::time_point timestamp;
    std::string description;
    core::Severity severity;
    core::EventId event_id;
    
    TimelineHighlight() : severity(core::Severity::INFO), event_id(0) {}
};

// Evidence excerpt for appendix
struct EvidenceExcerpt {
    core::EventId event_id;
    std::string source_ref;  // e.g., "app.log:123"
    std::string timestamp_str;
    std::string severity_str;
    std::string text;
    
    EvidenceExcerpt() : event_id(0) {}
};

// Complete report structure
struct Report {
    // Metadata
    std::string title = "Log Analysis Report";
    std::chrono::system_clock::time_point generated_at;
    
    // Analysis period
    std::optional<std::chrono::system_clock::time_point> log_start_time;
    std::optional<std::chrono::system_clock::time_point> log_end_time;
    
    // Executive Summary
    std::vector<SummaryBullet> summary;
    
    // Key statistics
    size_t total_events = 0;
    size_t error_count = 0;
    size_t warning_count = 0;
    
    // Timeline Highlights
    std::vector<TimelineHighlight> timeline;
    
    // Findings
    std::vector<rules::Finding> findings;
    
    // Evidence Appendix
    std::vector<EvidenceExcerpt> evidence;
    
    Report() : generated_at(std::chrono::system_clock::now()) {}
    
    // Helper methods
    bool has_findings() const { return !findings.empty(); }
    bool has_critical_findings() const;
    size_t finding_count_by_severity(rules::FindingSeverity sev) const;
};

} // namespace logstory::narrative
