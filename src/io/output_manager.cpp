#include "logstory/io/output_manager.hpp"
#include "logstory/narrative/markdown_writer.hpp"
#include "logstory/narrative/json_writer.hpp"
#include "logstory/narrative/timeline_writer.hpp"
#include <fstream>
#include <iostream>

namespace logstory::io {

OutputManager::OutputManager(const OutputConfig& config)
    : config_(config) {}

bool OutputManager::prepare_directory() {
    if (!ensure_directory_exists()) {
        return false;
    }
    
    if (!is_directory_writable()) {
        return false;
    }
    
    return true;
}

WriteResult OutputManager::write_report(const narrative::Report& report) {
    WriteResult result;
    
    if (!prepare_directory()) {
        return WriteResult("Failed to prepare output directory: " + 
                          config_.output_dir.string());
    }
    
    // Write Markdown
    if (has_format(config_.formats, OutputFormat::MARKDOWN)) {
        std::string filepath;
        if (write_markdown(report, filepath)) {
            result.written_files.push_back(filepath);
        } else {
            result.success = false;
            result.error_message += "Failed to write Markdown report. ";
        }
    }
    
    // Write JSON
    if (has_format(config_.formats, OutputFormat::JSON)) {
        std::string filepath;
        if (write_json(report, filepath)) {
            result.written_files.push_back(filepath);
        } else {
            result.success = false;
            result.error_message += "Failed to write JSON report. ";
        }
    }
    
    // Write CSV timeline highlights
    if (has_format(config_.formats, OutputFormat::CSV)) {
        std::string filepath;
        if (write_csv(report, filepath)) {
            result.written_files.push_back(filepath);
        } else {
            result.success = false;
            result.error_message += "Failed to write CSV timeline. ";
        }
    }
    
    return result;
}

WriteResult OutputManager::write_timeline(const std::vector<core::Event>& events) {
    WriteResult result;
    
    if (!prepare_directory()) {
        return WriteResult("Failed to prepare output directory: " + 
                          config_.output_dir.string());
    }
    
    std::string filepath;
    if (write_events_csv(events, filepath)) {
        result.written_files.push_back(filepath);
    } else {
        result.success = false;
        result.error_message = "Failed to write event timeline CSV";
    }
    
    return result;
}

WriteResult OutputManager::write_all(const narrative::Report& report,
                                     const std::vector<core::Event>& events) {
    WriteResult result = write_report(report);
    
    // Also write full event timeline if CSV format is enabled
    if (has_format(config_.formats, OutputFormat::CSV)) {
        WriteResult timeline_result = write_timeline(events);
        
        if (!timeline_result.success) {
            result.success = false;
            result.error_message += timeline_result.error_message;
        } else {
            result.written_files.insert(result.written_files.end(),
                                       timeline_result.written_files.begin(),
                                       timeline_result.written_files.end());
        }
    }
    
    return result;
}

std::filesystem::path OutputManager::get_output_path(const std::string& filename) const {
    return config_.output_dir / filename;
}

bool OutputManager::ensure_directory_exists() {
    if (!config_.create_directories) {
        return std::filesystem::exists(config_.output_dir);
    }
    
    std::error_code ec;
    std::filesystem::create_directories(config_.output_dir, ec);
    
    if (ec) {
        std::cerr << "Failed to create output directory: " << ec.message() << "\n";
        return false;
    }
    
    return true;
}

bool OutputManager::is_directory_writable() const {
    // Test write by creating a temp file
    auto test_path = config_.output_dir / ".write_test";
    
    std::ofstream test_file(test_path);
    if (!test_file) {
        return false;
    }
    test_file.close();
    
    std::error_code ec;
    std::filesystem::remove(test_path, ec);
    
    return true;
}

bool OutputManager::write_markdown(const narrative::Report& report, 
                                   std::string& filepath) {
    narrative::MarkdownWriter writer;
    auto path = get_output_path("report.md");
    filepath = path.string();
    return writer.write_to_file(report, filepath);
}

bool OutputManager::write_json(const narrative::Report& report, 
                               std::string& filepath) {
    narrative::JSONWriter writer;
    auto path = get_output_path("report.json");
    filepath = path.string();
    return writer.write_to_file(report, filepath);
}

bool OutputManager::write_csv(const narrative::Report& report, 
                              std::string& filepath) {
    narrative::TimelineWriter writer;
    auto path = get_output_path("timeline.csv");
    filepath = path.string();
    return writer.write_to_file(report, filepath);
}

bool OutputManager::write_events_csv(const std::vector<core::Event>& events,
                                     std::string& filepath) {
    narrative::TimelineWriterConfig config;
    config.include_all_events = true;
    config.include_raw_text = true;
    
    narrative::TimelineWriter writer(config);
    auto path = get_output_path("events_timeline.csv");
    filepath = path.string();
    return writer.write_events_to_file(events, filepath);
}

} // namespace logstory::io
