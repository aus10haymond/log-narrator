#include "logstory/narrative/narrator.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <set>

namespace logstory::narrative {

Narrator::Narrator(const NarratorConfig& config) : config_(config) {}

Report Narrator::generate(
    const std::vector<core::Event>& events,
    const analysis::Stats& stats,
    const std::vector<analysis::Episode>& episodes,
    const std::vector<rules::Finding>& findings) {
    
    Report report;
    
    // Set metadata
    report.log_start_time = stats.start_time;
    report.log_end_time = stats.end_time;
    report.total_events = stats.total_events;
    report.error_count = stats.error_count();
    report.warning_count = stats.warn_count();
    
    // Copy findings (sorted by severity)
    report.findings = findings;
    std::stable_sort(report.findings.begin(), report.findings.end(),
        [](const rules::Finding& a, const rules::Finding& b) {
            return static_cast<int>(a.severity) > static_cast<int>(b.severity);
        });
    
    // Generate sections
    generate_summary(report, stats, findings);
    generate_timeline(report, events, episodes, findings);
    generate_evidence(report, events, findings);
    
    return report;
}

void Narrator::generate_summary(Report& report, const analysis::Stats& stats,
                                const std::vector<rules::Finding>& findings) {
    
    // Overall stats
    std::ostringstream oss;
    oss << "Analyzed " << stats.total_events << " log events";
    if (stats.start_time.has_value() && stats.end_time.has_value()) {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            *stats.end_time - *stats.start_time);
        oss << " spanning " << format_duration(duration);
    }
    report.summary.emplace_back(oss.str());
    
    // Error/warning counts
    if (stats.error_count() > 0) {
        oss.str("");
        oss << "Found " << stats.error_count() << " errors";
        if (stats.warn_count() > 0) {
            oss << " and " << stats.warn_count() << " warnings";
        }
        report.summary.emplace_back(oss.str());
    } else if (stats.warn_count() > 0) {
        report.summary.emplace_back("Found " + std::to_string(stats.warn_count()) + " warnings");
    } else {
        report.summary.emplace_back("No errors or warnings detected");
    }
    
    // Findings summary
    if (!findings.empty()) {
        size_t critical = 0, high = 0, medium = 0, low = 0;
        for (const auto& f : findings) {
            switch (f.severity) {
                case rules::FindingSeverity::CRITICAL: critical++; break;
                case rules::FindingSeverity::HIGH: high++; break;
                case rules::FindingSeverity::MEDIUM: medium++; break;
                case rules::FindingSeverity::LOW: low++; break;
            }
        }
        
        oss.str("");
        oss << "Identified " << findings.size() << " finding(s)";
        if (critical > 0) oss << " (" << critical << " critical";
        if (high > 0) {
            if (critical > 0) oss << ", ";
            else oss << " (";
            oss << high << " high";
        }
        if (critical > 0 || high > 0) oss << ")";
        report.summary.emplace_back(oss.str());
    } else {
        report.summary.emplace_back("No significant patterns or anomalies detected");
    }
    
    // Source files
    if (!stats.source_counts.empty()) {
        oss.str("");
        oss << "Processed " << stats.source_counts.size() << " source file(s)";
        report.summary.emplace_back(oss.str());
    }
}

void Narrator::generate_timeline(Report& report, const std::vector<core::Event>& events,
                                 const std::vector<analysis::Episode>& episodes,
                                 const std::vector<rules::Finding>& findings) {
    
    std::vector<TimelineHighlight> highlights;
    
    // Add highlights from findings
    for (const auto& finding : findings) {
        // Add first evidence event as highlight
        if (!finding.evidence.empty() && finding.start_time.has_value()) {
            TimelineHighlight hl;
            hl.timestamp = *finding.start_time;
            hl.description = finding.title;
            hl.severity = core::Severity::ERROR;
            hl.event_id = finding.evidence[0].event_id;
            highlights.push_back(hl);
        }
    }
    
    // Add error events
    for (const auto& event : events) {
        if (event.sev == core::Severity::ERROR && event.ts.has_value()) {
            TimelineHighlight hl;
            hl.timestamp = event.ts->tp;
            hl.description = truncate_text(event.message, 100);
            hl.severity = event.sev;
            hl.event_id = event.id;
            highlights.push_back(hl);
        }
    }
    
    // Sort by time
    std::stable_sort(highlights.begin(), highlights.end(),
        [](const TimelineHighlight& a, const TimelineHighlight& b) {
            return a.timestamp < b.timestamp;
        });
    
    // Limit to max highlights
    if (highlights.size() > config_.max_timeline_highlights) {
        highlights.resize(config_.max_timeline_highlights);
    }
    
    report.timeline = highlights;
}

void Narrator::generate_evidence(Report& report, const std::vector<core::Event>& events,
                                 const std::vector<rules::Finding>& findings) {
    
    std::set<core::EventId> added_ids;
    
    // Collect evidence from findings
    for (const auto& finding : findings) {
        for (const auto& ev : finding.evidence) {
            if (added_ids.count(ev.event_id) > 0) {
                continue;
            }
            
            const core::Event* event = find_event_by_id(events, ev.event_id);
            if (!event) continue;
            
            EvidenceExcerpt excerpt;
            excerpt.event_id = ev.event_id;
            
            // Format source reference
            std::ostringstream oss;
            oss << event->src.source_path << ":" << event->src.start_line;
            if (event->src.end_line != event->src.start_line) {
                oss << "-" << event->src.end_line;
            }
            excerpt.source_ref = oss.str();
            
            // Format timestamp
            if (event->ts.has_value()) {
                excerpt.timestamp_str = format_timestamp(event->ts->tp);
            } else {
                excerpt.timestamp_str = "N/A";
            }
            
            // Severity
            excerpt.severity_str = core::to_string(event->sev);
            
            // Truncate text
            excerpt.text = truncate_text(event->raw, config_.max_excerpt_length);
            
            report.evidence.push_back(excerpt);
            added_ids.insert(ev.event_id);
            
            if (report.evidence.size() >= config_.max_evidence_excerpts) {
                return;
            }
        }
    }
}

std::string Narrator::format_timestamp(const std::chrono::system_clock::time_point& tp) const {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_buf;
    
#ifdef _WIN32
    localtime_s(&tm_buf, &time_t);
#else
    localtime_r(&time_t, &tm_buf);
#endif
    
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string Narrator::format_duration(std::chrono::seconds duration) const {
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
    duration -= hours;
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
    duration -= minutes;
    auto seconds = duration;
    
    std::ostringstream oss;
    if (hours.count() > 0) {
        oss << hours.count() << "h ";
    }
    if (minutes.count() > 0 || hours.count() > 0) {
        oss << minutes.count() << "m ";
    }
    oss << seconds.count() << "s";
    
    return oss.str();
}

std::string Narrator::truncate_text(const std::string& text, size_t max_len) const {
    if (text.length() <= max_len) {
        return text;
    }
    return text.substr(0, max_len - 3) + "...";
}

const core::Event* Narrator::find_event_by_id(const std::vector<core::Event>& events,
                                               core::EventId id) const {
    for (const auto& event : events) {
        if (event.id == id) {
            return &event;
        }
    }
    return nullptr;
}

bool Report::has_critical_findings() const {
    for (const auto& finding : findings) {
        if (finding.severity == rules::FindingSeverity::CRITICAL) {
            return true;
        }
    }
    return false;
}

size_t Report::finding_count_by_severity(rules::FindingSeverity sev) const {
    size_t count = 0;
    for (const auto& finding : findings) {
        if (finding.severity == sev) {
            count++;
        }
    }
    return count;
}

} // namespace logstory::narrative
