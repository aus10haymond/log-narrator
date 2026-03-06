#include "logstory/narrative/timeline_writer.hpp"
#include "logstory/core/severity.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace logstory::narrative {

TimelineWriter::TimelineWriter(const TimelineWriterConfig& config)
    : config_(config) {}

void TimelineWriter::write_highlights(const Report& report, std::ostream& out) {
    // Write CSV header
    out << "Timestamp,Severity,Event ID,Description";
    if (config_.include_raw_text) {
        out << ",Raw Text";
    }
    out << "\n";
    
    // Write each highlight
    for (const auto& highlight : report.timeline) {
        out << escape_csv(format_timestamp(highlight.timestamp)) << ",";
        out << escape_csv(core::to_string(highlight.severity)) << ",";
        out << highlight.event_id << ",";
        out << escape_csv(truncate(highlight.description));
        
        if (config_.include_raw_text) {
            // Raw text not available in highlights, leave empty
            out << ",";
        }
        
        out << "\n";
    }
}

void TimelineWriter::write_events(const std::vector<core::Event>& events, 
                                   std::ostream& out) {
    // Write CSV header
    out << "Event ID,Timestamp,Severity,Source,Message";
    if (config_.include_raw_text) {
        out << ",Raw Text";
    }
    out << "\n";
    
    // Write each event
    for (const auto& event : events) {
        out << event.id << ",";
        out << escape_csv(format_optional_timestamp(event.ts)) << ",";
        out << escape_csv(core::to_string(event.sev)) << ",";
        out << escape_csv(event.src.to_string()) << ",";
        out << escape_csv(truncate(event.message));
        
        if (config_.include_raw_text) {
            out << "," << escape_csv(truncate(event.raw));
        }
        
        out << "\n";
    }
}

bool TimelineWriter::write_to_file(const Report& report, 
                                    const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file) {
        return false;
    }
    
    write_highlights(report, file);
    return file.good();
}

bool TimelineWriter::write_events_to_file(const std::vector<core::Event>& events,
                                          const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file) {
        return false;
    }
    
    write_events(events, file);
    return file.good();
}

std::string TimelineWriter::escape_csv(const std::string& field) {
    // If field contains comma, quote, or newline, wrap in quotes and escape quotes
    bool needs_quoting = field.find(',') != std::string::npos ||
                        field.find('"') != std::string::npos ||
                        field.find('\n') != std::string::npos ||
                        field.find('\r') != std::string::npos;
    
    if (!needs_quoting) {
        return field;
    }
    
    std::string result = "\"";
    for (char c : field) {
        if (c == '"') {
            result += "\"\"";  // Escape quote with double quote
        } else if (c == '\n' || c == '\r') {
            result += ' ';  // Replace newlines with space in CSV
        } else {
            result += c;
        }
    }
    result += "\"";
    return result;
}

std::string TimelineWriter::format_timestamp(
    const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_buf;
    
#ifdef _WIN32
    gmtime_s(&tm_buf, &time_t);
#else
    gmtime_r(&time_t, &tm_buf);
#endif
    
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    
    // Add milliseconds
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()) % 1000;
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}

std::string TimelineWriter::format_optional_timestamp(
    const std::optional<core::Timestamp>& ts) {
    if (ts && ts->is_valid()) {
        return format_timestamp(ts->tp);
    }
    return "";
}

std::string TimelineWriter::truncate(const std::string& text) {
    if (text.length() <= config_.max_text_length) {
        return text;
    }
    
    return text.substr(0, config_.max_text_length - 3) + "...";
}

} // namespace logstory::narrative
