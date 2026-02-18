#include "logstory/rules/builtin/error_burst_after_change_rule.hpp"
#include <algorithm>
#include <cctype>

namespace logstory::rules::builtin {

std::vector<Finding> ErrorBurstAfterChangeRule::evaluate(const RuleContext& context) {
    std::vector<Finding> findings;
    
    if (!context.events || !context.anomalies) {
        return findings;
    }
    
    const auto& events = *context.events;
    
    // Find all change events
    std::vector<size_t> change_indices = find_change_events(events);
    
    if (change_indices.empty()) {
        return findings;
    }
    
    // Look for error bursts that occur shortly after change events
    for (const auto& anomaly : *context.anomalies) {
        if (anomaly.type != analysis::Anomaly::Type::ERROR_BURST) {
            continue;
        }
        
        if (!anomaly.start_time.has_value()) {
            continue;
        }
        
        auto burst_time = *anomaly.start_time;
        
        // Find nearest preceding change event
        std::optional<size_t> nearest_change_idx;
        std::chrono::seconds min_gap = std::chrono::hours(24);
        
        for (size_t change_idx : change_indices) {
            if (!events[change_idx].ts.has_value()) {
                continue;
            }
            
            auto change_time = events[change_idx].ts->tp;
            
            // Change must be before burst
            if (change_time >= burst_time) {
                continue;
            }
            
            auto gap = std::chrono::duration_cast<std::chrono::seconds>(burst_time - change_time);
            
            // Look for changes within 30 minutes before burst
            if (gap <= std::chrono::minutes(30) && gap < min_gap) {
                min_gap = gap;
                nearest_change_idx = change_idx;
            }
        }
        
        // If we found a change event close to the burst, create a finding
        if (nearest_change_idx.has_value()) {
            Finding finding;
            finding.id = "error-burst-after-change-" + std::to_string(findings.size() + 1);
            finding.title = "Error Burst Following Deployment/Config Change";
            finding.summary = "Error spike detected " + std::to_string(min_gap.count()) + 
                             " seconds after a deployment or configuration change";
            finding.severity = FindingSeverity::HIGH;
            
            // Higher confidence for shorter gaps
            double gap_minutes = min_gap.count() / 60.0;
            finding.confidence = std::max(0.5, 1.0 - (gap_minutes / 30.0));
            
            finding.start_time = events[*nearest_change_idx].ts->tp;
            finding.end_time = anomaly.end_time;
            
            // Add evidence
            finding.add_evidence(events[*nearest_change_idx].id, "Deployment or config change");
            
            // Add some error events from the burst (limit to first 5)
            size_t error_count = 0;
            for (const auto& event : events) {
                if (event.sev == core::Severity::ERROR && 
                    event.ts.has_value() &&
                    event.ts->tp >= burst_time &&
                    event.ts->tp <= *anomaly.end_time) {
                    
                    finding.add_evidence(event.id, "Error during burst");
                    error_count++;
                    if (error_count >= 5) break;
                }
            }
            
            findings.push_back(finding);
        }
    }
    
    return findings;
}

bool ErrorBurstAfterChangeRule::is_change_event(const core::Event& event) const {
    std::string lower_msg = event.message;
    std::transform(lower_msg.begin(), lower_msg.end(), lower_msg.begin(),
        [](unsigned char c) { return std::tolower(c); });
    
    return lower_msg.find("deploy") != std::string::npos ||
           lower_msg.find("deployment") != std::string::npos ||
           lower_msg.find("config") != std::string::npos ||
           lower_msg.find("configuration") != std::string::npos ||
           lower_msg.find("release") != std::string::npos ||
           lower_msg.find("rollout") != std::string::npos ||
           lower_msg.find("upgrade") != std::string::npos ||
           lower_msg.find("migration") != std::string::npos;
}

std::vector<size_t> ErrorBurstAfterChangeRule::find_change_events(
    const std::vector<core::Event>& events) const {
    
    std::vector<size_t> indices;
    
    for (size_t i = 0; i < events.size(); i++) {
        if (is_change_event(events[i])) {
            indices.push_back(i);
        }
    }
    
    return indices;
}

} // namespace logstory::rules::builtin
