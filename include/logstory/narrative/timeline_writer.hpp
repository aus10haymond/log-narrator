#pragma once

#include "logstory/narrative/report.hpp"
#include "logstory/core/event.hpp"
#include <string>
#include <ostream>
#include <vector>

namespace logstory::narrative {

/// Configuration for CSV timeline export
struct TimelineWriterConfig {
    bool include_all_events = false;  // If false, only timeline highlights
    bool include_raw_text = true;     // Include raw log text column
    size_t max_text_length = 200;     // Truncate long messages
    
    TimelineWriterConfig() = default;
};

/// Writes timeline data in CSV format for spreadsheet analysis
class TimelineWriter {
public:
    explicit TimelineWriter(const TimelineWriterConfig& config = TimelineWriterConfig());
    
    /// Write timeline highlights from report to CSV
    void write_highlights(const Report& report, std::ostream& out);
    
    /// Write all events to CSV timeline
    void write_events(const std::vector<core::Event>& events, std::ostream& out);
    
    /// Write timeline to file
    bool write_to_file(const Report& report, const std::string& filepath);
    
    /// Write full event timeline to file
    bool write_events_to_file(const std::vector<core::Event>& events, 
                              const std::string& filepath);
    
private:
    TimelineWriterConfig config_;
    
    /// Escape CSV field (handle quotes and commas)
    std::string escape_csv(const std::string& field);
    
    /// Format timestamp for CSV
    std::string format_timestamp(const std::chrono::system_clock::time_point& tp);
    
    /// Format optional timestamp
    std::string format_optional_timestamp(const std::optional<core::Timestamp>& ts);
    
    /// Truncate text to configured length
    std::string truncate(const std::string& text);
};

} // namespace logstory::narrative
