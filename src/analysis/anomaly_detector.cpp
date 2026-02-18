#include "logstory/analysis/anomaly_detector.hpp"
#include <algorithm>
#include <cctype>

namespace logstory::analysis {

// ============================================================================
// ErrorBurstDetector
// ============================================================================

ErrorBurstDetector::ErrorBurstDetector(const ErrorBurstConfig& config)
    : config_(config) {}

std::vector<Anomaly> ErrorBurstDetector::detect(
    const std::vector<core::Event>& events, 
    const Stats& stats) {
    
    std::vector<Anomaly> anomalies;
    
    // Get error time series
    auto it = stats.severity_time_series.find(core::Severity::ERROR);
    if (it == stats.severity_time_series.end()) {
        return anomalies; // No errors
    }
    
    const TimeSeries& error_series = it->second;
    if (error_series.points.empty()) {
        return anomalies;
    }
    
    // Compute baseline error rate
    double baseline = compute_baseline_rate(stats);
    
    // Find bursts
    return find_bursts(error_series, baseline);
}

double ErrorBurstDetector::compute_baseline_rate(const Stats& stats) const {
    // Simple baseline: average errors per minute bucket
    auto it = stats.severity_time_series.find(core::Severity::ERROR);
    if (it == stats.severity_time_series.end() || it->second.points.empty()) {
        return 0.0;
    }
    
    size_t total_errors = it->second.total_count();
    size_t num_buckets = it->second.points.size();
    
    if (num_buckets == 0) {
        return 0.0;
    }
    
    return static_cast<double>(total_errors) / static_cast<double>(num_buckets);
}

std::vector<Anomaly> ErrorBurstDetector::find_bursts(
    const TimeSeries& error_series, 
    double baseline) const {
    
    std::vector<Anomaly> bursts;
    
    double threshold = baseline * config_.threshold_multiplier;
    
    for (const auto& point : error_series.points) {
        if (point.count >= config_.min_errors_for_burst && 
            static_cast<double>(point.count) >= threshold) {
            
            Anomaly anomaly;
            anomaly.type = Anomaly::Type::ERROR_BURST;
            anomaly.start_time = point.timestamp;
            anomaly.end_time = point.timestamp + error_series.bucket_size;
            anomaly.description = "Error burst detected: " + 
                std::to_string(point.count) + " errors in " +
                std::to_string(error_series.bucket_size.count()) + " minutes";
            
            // Confidence based on how much it exceeds threshold
            double ratio = static_cast<double>(point.count) / threshold;
            anomaly.confidence = std::min(1.0, ratio / 2.0);
            
            bursts.push_back(anomaly);
        }
    }
    
    return bursts;
}

// ============================================================================
// RestartLoopDetector
// ============================================================================

RestartLoopDetector::RestartLoopDetector(const RestartLoopConfig& config)
    : config_(config) {}

std::vector<Anomaly> RestartLoopDetector::detect(const std::vector<core::Event>& events) {
    std::vector<size_t> restart_indices;
    
    // Find all restart events
    for (size_t i = 0; i < events.size(); i++) {
        if (is_restart_event(events[i])) {
            restart_indices.push_back(i);
        }
    }
    
    if (restart_indices.size() < config_.min_restart_count) {
        return {}; // Not enough restarts
    }
    
    return find_loops(restart_indices, events);
}

bool RestartLoopDetector::is_restart_event(const core::Event& event) const {
    // Convert message to lowercase for case-insensitive matching
    std::string lower_msg = event.message;
    std::transform(lower_msg.begin(), lower_msg.end(), lower_msg.begin(),
        [](unsigned char c) { return std::tolower(c); });
    
    // Check for restart keywords
    for (const auto& keyword : config_.restart_keywords) {
        if (lower_msg.find(keyword) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

std::vector<Anomaly> RestartLoopDetector::find_loops(
    const std::vector<size_t>& restart_indices,
    const std::vector<core::Event>& events) const {
    
    std::vector<Anomaly> loops;
    
    // Look for clusters of restarts within time window
    for (size_t i = 0; i < restart_indices.size(); i++) {
        size_t start_idx = restart_indices[i];
        const auto& start_event = events[start_idx];
        
        if (!start_event.ts.has_value()) {
            continue;
        }
        
        auto start_time = start_event.ts->tp;
        auto window_end = start_time + config_.time_window;
        
        // Count restarts in window
        std::vector<core::EventId> evidence;
        evidence.push_back(start_event.id);
        
        for (size_t j = i + 1; j < restart_indices.size(); j++) {
            size_t idx = restart_indices[j];
            const auto& evt = events[idx];
            
            if (!evt.ts.has_value()) {
                continue;
            }
            
            if (evt.ts->tp <= window_end) {
                evidence.push_back(evt.id);
            } else {
                break; // Past window
            }
        }
        
        // Check if we found a loop
        if (evidence.size() >= config_.min_restart_count) {
            Anomaly anomaly;
            anomaly.type = Anomaly::Type::RESTART_LOOP;
            anomaly.start_time = start_time;
            anomaly.end_time = events[restart_indices[i + evidence.size() - 1]].ts->tp;
            anomaly.description = "Restart loop detected: " + 
                std::to_string(evidence.size()) + " restarts in " +
                std::to_string(config_.time_window.count()) + " minutes";
            anomaly.evidence_ids = evidence;
            
            // Higher confidence for more restarts
            anomaly.confidence = std::min(1.0, 
                static_cast<double>(evidence.size()) / (config_.min_restart_count * 2.0));
            
            loops.push_back(anomaly);
            
            // Skip ahead to avoid overlapping detections
            i += evidence.size() - 1;
        }
    }
    
    return loops;
}

} // namespace logstory::analysis
