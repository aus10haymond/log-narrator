#pragma once

#include "logstory/analysis/episode.hpp"
#include "logstory/core/event.hpp"
#include <vector>
#include <chrono>
#include <unordered_set>

namespace logstory::analysis {

/// Configuration for episode building
struct EpisodeConfig {
    /// Maximum time gap between events in an episode (default: 5 minutes)
    std::chrono::seconds time_gap_threshold = std::chrono::minutes(5);
    
    /// Whether to merge episodes with shared correlation IDs
    bool merge_by_correlation = true;
};

/// Builds episodes from a sequence of events
class EpisodeBuilder {
public:
    explicit EpisodeBuilder(EpisodeConfig config = EpisodeConfig())
        : config_(config), next_episode_id_(1) {}
    
    /// Build episodes from events
    std::vector<Episode> build(const std::vector<core::Event>& events);

private:
    EpisodeConfig config_;
    uint64_t next_episode_id_;
    
    /// Check if there's a significant time gap between events
    bool has_time_gap(const core::Event& prev, const core::Event& next) const;
    
    /// Extract correlation IDs from an event
    std::vector<std::string> get_correlation_ids(const core::Event& event) const;
    
    /// Check if two episodes share correlation IDs
    bool share_correlation_ids(const Episode& ep1, const Episode& ep2) const;
    
    /// Merge two episodes
    Episode merge_episodes(const Episode& ep1, const Episode& ep2);
    
    /// Identify key events in an episode
    void identify_highlights(Episode& episode, const std::vector<core::Event>& events);
    
    /// Update episode metadata (times, severity, correlation IDs)
    void update_metadata(Episode& episode, const std::vector<core::Event>& events);
};

} // namespace logstory::analysis
