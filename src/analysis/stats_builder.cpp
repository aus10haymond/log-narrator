#include "logstory/analysis/stats_builder.hpp"
#include <algorithm>
#include <map>
#include <regex>

namespace logstory::analysis {

StatsBuilder::StatsBuilder(const StatsConfig& config) : config_(config) {}

Stats StatsBuilder::build(const std::vector<core::Event>& events) {
    Stats stats;
    
    if (events.empty()) {
        return stats;
    }
    
    stats.total_events = events.size();
    
    // Process each event
    for (const auto& event : events) {
        process_event(event, stats);
    }
    
    // Compute frequent patterns
    compute_frequent_patterns(events, stats);
    
    return stats;
}

void StatsBuilder::process_event(const core::Event& event, Stats& stats) {
    // Update severity counts
    stats.severity_counts[event.sev]++;
    
    // Update source counts
    if (!event.src.source_path.empty()) {
        stats.source_counts[event.src.source_path]++;
    }
    
    // Update time boundaries
    if (event.ts.has_value()) {
        auto tp = event.ts->tp;
        
        if (!stats.start_time.has_value() || tp < *stats.start_time) {
            stats.start_time = tp;
        }
        
        if (!stats.end_time.has_value() || tp > *stats.end_time) {
            stats.end_time = tp;
        }
        
        // Update time series
        if (stats.severity_time_series.find(event.sev) == stats.severity_time_series.end()) {
            stats.severity_time_series[event.sev] = TimeSeries(config_.time_bucket_size);
        }
        stats.severity_time_series[event.sev].add_event(tp);
    }
}

void StatsBuilder::compute_frequent_patterns(const std::vector<core::Event>& events, Stats& stats) {
    // Count patterns
    std::map<std::string, std::pair<size_t, core::Severity>> pattern_counts;
    
    for (const auto& event : events) {
        if (event.message.length() >= config_.min_pattern_length) {
            std::string pattern = extract_pattern(event.message);
            if (!pattern.empty()) {
                auto& entry = pattern_counts[pattern];
                entry.first++;
                // Track max severity
                if (static_cast<int>(event.sev) > static_cast<int>(entry.second)) {
                    entry.second = event.sev;
                }
            }
        }
    }
    
    // Convert to vector and sort by count
    std::vector<FrequentPattern> patterns;
    for (const auto& [pattern, data] : pattern_counts) {
        patterns.emplace_back(pattern, data.first, data.second);
    }
    
    std::sort(patterns.begin(), patterns.end(),
        [](const FrequentPattern& a, const FrequentPattern& b) {
            return a.count > b.count;
        });
    
    // Keep top N
    size_t n = std::min(config_.top_n_patterns, patterns.size());
    stats.frequent_patterns.assign(patterns.begin(), patterns.begin() + n);
}

std::string StatsBuilder::extract_pattern(const std::string& message) const {
    // Simple pattern extraction: replace numbers and IDs with placeholders
    std::string pattern = message;
    
    // Replace UUIDs
    std::regex uuid_regex("[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}");
    pattern = std::regex_replace(pattern, uuid_regex, "<UUID>");
    
    // Replace hex numbers
    std::regex hex_regex("0x[0-9a-fA-F]+");
    pattern = std::regex_replace(pattern, hex_regex, "<HEX>");
    
    // Replace numbers (including decimals and negatives)
    std::regex num_regex("-?\\d+\\.?\\d*");
    pattern = std::regex_replace(pattern, num_regex, "<NUM>");
    
    // Replace quoted strings
    std::regex quoted_regex("\"[^\"]+\"");
    pattern = std::regex_replace(pattern, quoted_regex, "<STR>");
    
    return pattern;
}

} // namespace logstory::analysis
