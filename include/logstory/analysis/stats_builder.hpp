#pragma once

#include "logstory/analysis/stats.hpp"
#include "logstory/core/event.hpp"
#include <chrono>
#include <vector>

namespace logstory::analysis {

struct StatsConfig {
    std::chrono::minutes time_bucket_size = std::chrono::minutes(1);
    size_t top_n_patterns = 10;
    size_t min_pattern_length = 10; // Min chars for pattern matching
    
    StatsConfig() = default;
};

class StatsBuilder {
public:
    explicit StatsBuilder(const StatsConfig& config = StatsConfig());
    
    // Build statistics from a list of events
    Stats build(const std::vector<core::Event>& events);
    
private:
    StatsConfig config_;
    
    // Helper methods
    void process_event(const core::Event& event, Stats& stats);
    void compute_frequent_patterns(const std::vector<core::Event>& events, Stats& stats);
    std::string extract_pattern(const std::string& message) const;
};

} // namespace logstory::analysis
