#pragma once

#include "logstory/narrative/report.hpp"
#include "logstory/core/event.hpp"
#include "logstory/analysis/stats.hpp"
#include "logstory/analysis/episode.hpp"
#include "logstory/rules/finding.hpp"
#include <vector>
#include <map>

namespace logstory::narrative {

// Configuration for report generation
struct NarratorConfig {
    size_t max_timeline_highlights = 20;
    size_t max_evidence_excerpts = 50;
    size_t max_excerpt_length = 500;  // chars
    
    NarratorConfig() = default;
};

// Generates narrative reports from analysis results
class Narrator {
public:
    explicit Narrator(const NarratorConfig& config = NarratorConfig());
    
    // Generate a complete report
    Report generate(
        const std::vector<core::Event>& events,
        const analysis::Stats& stats,
        const std::vector<analysis::Episode>& episodes,
        const std::vector<rules::Finding>& findings
    );
    
private:
    NarratorConfig config_;
    
    // Section generators
    void generate_summary(Report& report, const analysis::Stats& stats,
                         const std::vector<rules::Finding>& findings);
    
    void generate_timeline(Report& report, const std::vector<core::Event>& events,
                          const std::vector<analysis::Episode>& episodes,
                          const std::vector<rules::Finding>& findings);
    
    void generate_evidence(Report& report, const std::vector<core::Event>& events,
                          const std::vector<rules::Finding>& findings);
    
    // Helper methods
    std::string format_timestamp(const std::chrono::system_clock::time_point& tp) const;
    std::string format_duration(std::chrono::seconds duration) const;
    std::string truncate_text(const std::string& text, size_t max_len) const;
    
    // Event lookup
    const core::Event* find_event_by_id(const std::vector<core::Event>& events,
                                        core::EventId id) const;
};

} // namespace logstory::narrative
