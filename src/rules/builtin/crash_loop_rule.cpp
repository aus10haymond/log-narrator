#include "logstory/rules/builtin/crash_loop_rule.hpp"

namespace logstory::rules::builtin {

std::vector<Finding> CrashLoopRule::evaluate(const RuleContext& context) {
    std::vector<Finding> findings;
    
    if (!context.anomalies) {
        return findings;
    }
    
    // Look for restart loop anomalies
    for (const auto& anomaly : *context.anomalies) {
        if (anomaly.type == analysis::Anomaly::Type::RESTART_LOOP) {
            Finding finding;
            finding.id = "crash-loop-" + std::to_string(findings.size() + 1);
            finding.title = "Crash Loop Detected";
            finding.summary = anomaly.description;
            finding.severity = FindingSeverity::CRITICAL;
            finding.confidence = anomaly.confidence;
            finding.start_time = anomaly.start_time;
            finding.end_time = anomaly.end_time;
            
            // Add evidence from anomaly
            for (const auto& event_id : anomaly.evidence_ids) {
                finding.add_evidence(event_id, "Restart event");
            }
            
            findings.push_back(finding);
        }
    }
    
    return findings;
}

} // namespace logstory::rules::builtin
