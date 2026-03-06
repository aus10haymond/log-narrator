#pragma once

#include "logstory/narrative/report.hpp"
#include "logstory/core/event.hpp"
#include <string>
#include <vector>
#include <filesystem>

namespace logstory::io {

/// Output format selection
enum class OutputFormat {
    MARKDOWN  = 0x01,
    JSON      = 0x02,
    CSV       = 0x04,
    ALL       = MARKDOWN | JSON | CSV
};

// Bitwise operators for OutputFormat
inline OutputFormat operator|(OutputFormat a, OutputFormat b) {
    return static_cast<OutputFormat>(
        static_cast<int>(a) | static_cast<int>(b)
    );
}

inline OutputFormat operator&(OutputFormat a, OutputFormat b) {
    return static_cast<OutputFormat>(
        static_cast<int>(a) & static_cast<int>(b)
    );
}

inline bool has_format(OutputFormat flags, OutputFormat check) {
    return (flags & check) == check;
}

/// Configuration for output management
struct OutputConfig {
    std::filesystem::path output_dir = "output";
    OutputFormat formats = OutputFormat::ALL;
    bool overwrite_existing = true;
    bool create_directories = true;
    
    OutputConfig() = default;
};

/// Result of write operations
struct WriteResult {
    bool success = true;
    std::string error_message;
    std::vector<std::string> written_files;
    
    WriteResult() = default;
    explicit WriteResult(const std::string& error) 
        : success(false), error_message(error) {}
};

/// Manages output directory creation and file writing
class OutputManager {
public:
    explicit OutputManager(const OutputConfig& config = OutputConfig());
    
    /// Prepare output directory (create if needed, clean if requested)
    bool prepare_directory();
    
    /// Write report in all configured formats
    WriteResult write_report(const narrative::Report& report);
    
    /// Write event timeline (CSV only)
    WriteResult write_timeline(const std::vector<core::Event>& events);
    
    /// Write all outputs (report + timeline)
    WriteResult write_all(const narrative::Report& report,
                         const std::vector<core::Event>& events);
    
    /// Get output directory path
    const std::filesystem::path& output_dir() const { return config_.output_dir; }
    
    /// Get full path for output file
    std::filesystem::path get_output_path(const std::string& filename) const;
    
private:
    OutputConfig config_;
    
    /// Ensure directory exists
    bool ensure_directory_exists();
    
    /// Check if directory is writable
    bool is_directory_writable() const;
    
    /// Write report in markdown format
    bool write_markdown(const narrative::Report& report, std::string& filepath);
    
    /// Write report in JSON format
    bool write_json(const narrative::Report& report, std::string& filepath);
    
    /// Write timeline in CSV format
    bool write_csv(const narrative::Report& report, std::string& filepath);
    
    /// Write full event timeline in CSV format
    bool write_events_csv(const std::vector<core::Event>& events, 
                         std::string& filepath);
};

} // namespace logstory::io
