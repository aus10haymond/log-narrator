#include "logstory/analysis/episode_builder.hpp"
#include <algorithm>
#include <unordered_map>

namespace logstory::analysis {

std::vector<Episode> EpisodeBuilder::build(const std::vector<core::Event>& events) {
    if (events.empty()) {
        return {};
    }
    
    std::vector<Episode> episodes;
    
    // Phase 1: Group by time gaps
    Episode current_episode(next_episode_id_++);
    
    for (size_t i = 0; i < events.size(); ++i) {
        const auto& event = events[i];
        
        // Check if we should start a new episode
        bool start_new = false;
        if (!current_episode.empty() && i > 0) {
            start_new = has_time_gap(events[i - 1], event);
        }
        
        if (start_new) {
            // Finalize current episode
            update_metadata(current_episode, events);
            identify_highlights(current_episode, events);
            episodes.push_back(current_episode);
            
            // Start new episode
            current_episode = Episode(next_episode_id_++);
        }
        
        // Add event to current episode
        current_episode.event_ids.push_back(event.id);
    }
    
    // Don't forget the last episode
    if (!current_episode.empty()) {
        update_metadata(current_episode, events);
        identify_highlights(current_episode, events);
        episodes.push_back(current_episode);
    }
    
    // Phase 2: Merge adjacent episodes with shared correlation IDs
    if (config_.merge_by_correlation && episodes.size() > 1) {
        std::vector<Episode> merged;
        merged.push_back(episodes[0]);
        
        for (size_t i = 1; i < episodes.size(); ++i) {
            if (share_correlation_ids(merged.back(), episodes[i])) {
                // Merge with previous episode
                merged.back() = merge_episodes(merged.back(), episodes[i]);
            } else {
                merged.push_back(episodes[i]);
            }
        }
        
        return merged;
    }
    
    return episodes;
}

bool EpisodeBuilder::has_time_gap(const core::Event& prev, const core::Event& next) const {
    // If either event doesn't have a valid timestamp, don't break on time gap
    if (!prev.ts.has_value() || !prev.ts->is_valid() ||
        !next.ts.has_value() || !next.ts->is_valid()) {
        return false;
    }
    
    auto gap = next.ts->tp - prev.ts->tp;
    return gap > config_.time_gap_threshold;
}

std::vector<std::string> EpisodeBuilder::get_correlation_ids(const core::Event& event) const {
    std::vector<std::string> ids;
    
    auto req_it = event.tags.find("request_id");
    if (req_it != event.tags.end()) {
        ids.push_back(req_it->second);
    }
    
    auto trace_it = event.tags.find("trace_id");
    if (trace_it != event.tags.end()) {
        ids.push_back(trace_it->second);
    }
    
    return ids;
}

bool EpisodeBuilder::share_correlation_ids(const Episode& ep1, const Episode& ep2) const {
    // Check if any correlation IDs are shared
    for (const auto& id1 : ep1.correlation_ids) {
        for (const auto& id2 : ep2.correlation_ids) {
            if (id1 == id2) {
                return true;
            }
        }
    }
    return false;
}

Episode EpisodeBuilder::merge_episodes(const Episode& ep1, const Episode& ep2) {
    Episode merged(ep1.id); // Keep first episode's ID
    
    // Merge event IDs
    merged.event_ids = ep1.event_ids;
    merged.event_ids.insert(merged.event_ids.end(), 
                           ep2.event_ids.begin(), ep2.event_ids.end());
    
    // Merge times
    merged.start_time = ep1.start_time;
    merged.end_time = ep2.end_time;
    
    // Merge correlation IDs (unique)
    std::unordered_set<std::string> unique_ids;
    for (const auto& id : ep1.correlation_ids) {
        unique_ids.insert(id);
    }
    for (const auto& id : ep2.correlation_ids) {
        unique_ids.insert(id);
    }
    merged.correlation_ids.assign(unique_ids.begin(), unique_ids.end());
    
    // Merge highlights
    merged.highlights = ep1.highlights;
    merged.highlights.insert(merged.highlights.end(),
                            ep2.highlights.begin(), ep2.highlights.end());
    
    // Take max severity
    merged.max_severity = std::max(ep1.max_severity, ep2.max_severity);
    
    return merged;
}

void EpisodeBuilder::identify_highlights(Episode& episode, 
                                        const std::vector<core::Event>& events) {
    if (episode.event_ids.empty()) {
        return;
    }
    
    // Build a map of event ID to event for quick lookup
    std::unordered_map<core::EventId, const core::Event*> event_map;
    for (const auto& event : events) {
        event_map[event.id] = &event;
    }
    
    core::EventId first_error_id = 0;
    core::EventId max_severity_id = 0;
    core::Severity max_sev = core::Severity::UNKNOWN;
    
    for (const auto& event_id : episode.event_ids) {
        auto it = event_map.find(event_id);
        if (it == event_map.end()) {
            continue;
        }
        
        const auto& event = *it->second;
        
        // Track first error
        if (first_error_id == 0 && 
            (event.sev == core::Severity::ERROR || event.sev == core::Severity::FATAL)) {
            first_error_id = event.id;
        }
        
        // Track max severity event
        if (event.sev > max_sev) {
            max_sev = event.sev;
            max_severity_id = event.id;
        }
    }
    
    // Add highlights
    if (first_error_id != 0) {
        episode.highlights.push_back(first_error_id);
    }
    if (max_severity_id != 0 && max_severity_id != first_error_id) {
        episode.highlights.push_back(max_severity_id);
    }
}

void EpisodeBuilder::update_metadata(Episode& episode, 
                                     const std::vector<core::Event>& events) {
    if (episode.event_ids.empty()) {
        return;
    }
    
    // Build event map
    std::unordered_map<core::EventId, const core::Event*> event_map;
    for (const auto& event : events) {
        event_map[event.id] = &event;
    }
    
    // Collect correlation IDs and find time boundaries
    std::unordered_set<std::string> unique_corr_ids;
    core::Severity max_sev = core::Severity::UNKNOWN;
    
    for (const auto& event_id : episode.event_ids) {
        auto it = event_map.find(event_id);
        if (it == event_map.end()) {
            continue;
        }
        
        const auto& event = *it->second;
        
        // Collect correlation IDs
        auto corr_ids = get_correlation_ids(event);
        for (const auto& id : corr_ids) {
            unique_corr_ids.insert(id);
        }
        
        // Track max severity
        max_sev = std::max(max_sev, event.sev);
        
        // Track time boundaries
        if (event.ts.has_value() && event.ts->is_valid()) {
            if (!episode.start_time.has_value()) {
                episode.start_time = event.ts->tp;
                episode.end_time = event.ts->tp;
            } else {
                episode.start_time = std::min(*episode.start_time, event.ts->tp);
                episode.end_time = std::max(*episode.end_time, event.ts->tp);
            }
        }
    }
    
    episode.correlation_ids.assign(unique_corr_ids.begin(), unique_corr_ids.end());
    episode.max_severity = max_sev;
}

} // namespace logstory::analysis
