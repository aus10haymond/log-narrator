#pragma once

#include "logstory/analysis/stats.hpp"
#include "logstory/core/event.hpp"
#include <chrono>
#include <vector>

namespace logstory::analysis {

// Anomaly detection result
struct Anomaly {
    enum class Type {
        ERROR_BURST,
        RESTART_LOOP,
        MISSING_HEARTBEAT
    };
    
    Type type;
    std::string description;
    std::vector<core::EventId> evidence_ids;
    double confidence; // 0.0 to 1.0
    std::optional<std::chrono::system_clock::time_point> start_time;
    std::optional<std::chrono::system_clock::time_point> end_time;
    
    Anomaly() : type(Type::ERROR_BURST), confidence(0.0) {}
};

// Error burst detector configuration
struct ErrorBurstConfig {
    std::chrono::minutes window_size = std::chrono::minutes(5);
    double threshold_multiplier = 3.0; // Burst = N * baseline rate
    size_t min_errors_for_burst = 10;
    
    ErrorBurstConfig() = default;
};

// Error burst detector
class ErrorBurstDetector {
public:
    explicit ErrorBurstDetector(const ErrorBurstConfig& config = ErrorBurstConfig());
    
    // Detect error bursts in event stream
    std::vector<Anomaly> detect(const std::vector<core::Event>& events, const Stats& stats);
    
private:
    ErrorBurstConfig config_;
    
    double compute_baseline_rate(const Stats& stats) const;
    std::vector<Anomaly> find_bursts(const TimeSeries& error_series, double baseline) const;
};

// Restart loop detector configuration
struct RestartLoopConfig {
    size_t min_restart_count = 3;
    std::chrono::minutes time_window = std::chrono::minutes(10);
    std::vector<std::string> restart_keywords = {
        "starting", "started", "shutdown", "stopping", "stopped",
        "restarting", "restart", "initializing", "initialized"
    };
    
    RestartLoopConfig() = default;
};

// Restart loop detector
class RestartLoopDetector {
public:
    explicit RestartLoopDetector(const RestartLoopConfig& config = RestartLoopConfig());
    
    // Detect restart loops in event stream
    std::vector<Anomaly> detect(const std::vector<core::Event>& events);
    
private:
    RestartLoopConfig config_;
    
    bool is_restart_event(const core::Event& event) const;
    std::vector<Anomaly> find_loops(const std::vector<size_t>& restart_indices, 
                                     const std::vector<core::Event>& events) const;
};

} // namespace logstory::analysis
