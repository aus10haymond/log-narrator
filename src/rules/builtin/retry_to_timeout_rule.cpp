#include "logstory/rules/builtin/retry_to_timeout_rule.hpp"
#include <algorithm>
#include <cctype>

namespace logstory::rules::builtin {

std::vector<Finding> RetryToTimeoutRule::evaluate(const RuleContext& context) {
    std::vector<Finding> findings;
    
    if (!context.events || context.events->empty()) {
        return findings;
    }
    
    const auto& events = *context.events;
    
    // Look for retry -> timeout patterns
    for (size_t i = 0; i < events.size(); i++) {
        if (!is_retry_event(events[i])) {
            continue;
        }
        
        // Count consecutive retries
        std::vector<core::EventId> retry_ids;
        retry_ids.push_back(events[i].id);
        
        size_t j = i + 1;
        while (j < events.size() && is_retry_event(events[j])) {
            retry_ids.push_back(events[j].id);
            j++;
        }
        
        // Check if followed by timeout
        if (j < events.size() && is_timeout_event(events[j])) {
            Finding finding;
            finding.id = "retry-timeout-" + std::to_string(findings.size() + 1);
            finding.title = "Retries Leading to Timeout";
            finding.summary = "Detected " + std::to_string(retry_ids.size()) + 
                             " retry attempts followed by a timeout";
            finding.severity = FindingSeverity::HIGH;
            finding.confidence = std::min(1.0, 0.5 + (retry_ids.size() * 0.1));
            
            if (events[i].ts.has_value()) {
                finding.start_time = events[i].ts->tp;
            }
            if (events[j].ts.has_value()) {
                finding.end_time = events[j].ts->tp;
            }
            
            // Add evidence
            for (const auto& retry_id : retry_ids) {
                finding.add_evidence(retry_id, "Retry attempt");
            }
            finding.add_evidence(events[j].id, "Final timeout");
            
            findings.push_back(finding);
            
            // Skip past this pattern
            i = j;
        }
    }
    
    return findings;
}

bool RetryToTimeoutRule::is_retry_event(const core::Event& event) const {
    std::string lower_msg = event.message;
    std::transform(lower_msg.begin(), lower_msg.end(), lower_msg.begin(),
        [](unsigned char c) { return std::tolower(c); });
    
    return lower_msg.find("retry") != std::string::npos ||
           lower_msg.find("retrying") != std::string::npos ||
           lower_msg.find("attempt") != std::string::npos;
}

bool RetryToTimeoutRule::is_timeout_event(const core::Event& event) const {
    std::string lower_msg = event.message;
    std::transform(lower_msg.begin(), lower_msg.end(), lower_msg.begin(),
        [](unsigned char c) { return std::tolower(c); });
    
    return lower_msg.find("timeout") != std::string::npos ||
           lower_msg.find("timed out") != std::string::npos;
}

} // namespace logstory::rules::builtin
