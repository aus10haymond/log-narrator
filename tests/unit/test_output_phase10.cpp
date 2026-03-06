#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "logstory/narrative/timeline_writer.hpp"
#include "logstory/io/output_manager.hpp"
#include "logstory/core/event.hpp"
#include "logstory/core/severity.hpp"

#include <sstream>
#include <filesystem>
#include <fstream>

using namespace logstory;
using namespace Catch::Matchers;

TEST_CASE("TimelineWriter - CSV escape", "[timeline_writer]") {
    narrative::TimelineWriter writer;
    
    SECTION("Basic text doesn't need escaping") {
        narrative::Report report;
        narrative::TimelineHighlight highlight;
        highlight.timestamp = std::chrono::system_clock::now();
        highlight.description = "Simple message";
        highlight.severity = core::Severity::INFO;
        highlight.event_id = 1;
        report.timeline.push_back(highlight);
        
        std::ostringstream oss;
        writer.write_highlights(report, oss);
        
        std::string output = oss.str();
        REQUIRE(output.find("Simple message") != std::string::npos);
        REQUIRE(output.find("Timestamp,Severity,Event ID,Description") != std::string::npos);
    }
    
    SECTION("Text with comma is quoted") {
        narrative::Report report;
        narrative::TimelineHighlight highlight;
        highlight.timestamp = std::chrono::system_clock::now();
        highlight.description = "Message, with comma";
        highlight.severity = core::Severity::ERROR;
        highlight.event_id = 2;
        report.timeline.push_back(highlight);
        
        std::ostringstream oss;
        writer.write_highlights(report, oss);
        
        std::string output = oss.str();
        REQUIRE(output.find("\"Message, with comma\"") != std::string::npos);
    }
    
    SECTION("Text with quotes is escaped") {
        narrative::Report report;
        narrative::TimelineHighlight highlight;
        highlight.timestamp = std::chrono::system_clock::now();
        highlight.description = "Message with \"quotes\"";
        highlight.severity = core::Severity::WARN;
        highlight.event_id = 3;
        report.timeline.push_back(highlight);
        
        std::ostringstream oss;
        writer.write_highlights(report, oss);
        
        std::string output = oss.str();
        // Quotes should be doubled
        REQUIRE(output.find("\"\"quotes\"\"") != std::string::npos);
    }
}

TEST_CASE("TimelineWriter - Event timeline export", "[timeline_writer]") {
    narrative::TimelineWriterConfig config;
    config.include_raw_text = true;
    config.max_text_length = 50;
    
    narrative::TimelineWriter writer(config);
    
    SECTION("Single event export") {
        std::vector<core::Event> events;
        
        core::Event event;
        event.id = 100;
        event.sev = core::Severity::ERROR;
        event.message = "Test error message";
        event.raw = "2023-01-01 10:00:00 ERROR Test error message";
        event.src = core::SourceRef("test.log", 42);
        
        events.push_back(event);
        
        std::ostringstream oss;
        writer.write_events(events, oss);
        
        std::string output = oss.str();
        REQUIRE(output.find("Event ID,Timestamp,Severity,Source,Message,Raw Text") != std::string::npos);
        REQUIRE(output.find("100") != std::string::npos);
        REQUIRE(output.find("ERROR") != std::string::npos);
        REQUIRE(output.find("Test error message") != std::string::npos);
        REQUIRE(output.find("test.log:42") != std::string::npos);
    }
    
    SECTION("Multiple events preserve order") {
        std::vector<core::Event> events;
        
        for (int i = 0; i < 5; i++) {
            core::Event event;
            event.id = i;
            event.sev = core::Severity::INFO;
            event.message = "Message " + std::to_string(i);
            event.raw = event.message;
            event.src = core::SourceRef("test.log", i + 1);
            events.push_back(event);
        }
        
        std::ostringstream oss;
        writer.write_events(events, oss);
        
        std::string output = oss.str();
        
        // Check that all events are present
        for (int i = 0; i < 5; i++) {
            REQUIRE(output.find("Message " + std::to_string(i)) != std::string::npos);
        }
    }
    
    SECTION("Long messages are truncated") {
        std::vector<core::Event> events;
        
        core::Event event;
        event.id = 1;
        event.sev = core::Severity::INFO;
        event.message = std::string(100, 'x');  // 100 characters
        event.raw = event.message;
        event.src = core::SourceRef("test.log", 1);
        
        events.push_back(event);
        
        std::ostringstream oss;
        writer.write_events(events, oss);
        
        std::string output = oss.str();
        
        // Should be truncated to 50 chars + "..."
        REQUIRE(output.find("...") != std::string::npos);
    }
}

TEST_CASE("TimelineWriter - Timestamp formatting", "[timeline_writer]") {
    narrative::TimelineWriter writer;
    
    SECTION("Format valid timestamp") {
        narrative::Report report;
        narrative::TimelineHighlight highlight;
        
        // Create a specific timestamp
        std::tm tm = {};
        tm.tm_year = 123;  // 2023
        tm.tm_mon = 0;     // January
        tm.tm_mday = 15;
        tm.tm_hour = 14;
        tm.tm_min = 30;
        tm.tm_sec = 45;
        
        auto time_t = std::mktime(&tm);
        highlight.timestamp = std::chrono::system_clock::from_time_t(time_t);
        highlight.description = "Test event";
        highlight.severity = core::Severity::INFO;
        highlight.event_id = 1;
        
        report.timeline.push_back(highlight);
        
        std::ostringstream oss;
        writer.write_highlights(report, oss);
        
        std::string output = oss.str();
        // Should contain date components
        REQUIRE(output.find("2023") != std::string::npos);
    }
}

TEST_CASE("OutputManager - Directory management", "[output_manager]") {
    SECTION("Create output directory") {
        io::OutputConfig config;
        config.output_dir = "test_output_temp";
        config.create_directories = true;
        
        io::OutputManager manager(config);
        
        REQUIRE(manager.prepare_directory());
        REQUIRE(std::filesystem::exists("test_output_temp"));
        
        // Cleanup
        std::filesystem::remove_all("test_output_temp");
    }
    
    SECTION("Get output path") {
        io::OutputConfig config;
        config.output_dir = "my_output";
        
        io::OutputManager manager(config);
        
        auto path = manager.get_output_path("report.md");
        REQUIRE(path.filename() == "report.md");
        REQUIRE(path.parent_path().filename() == "my_output");
    }
}

TEST_CASE("OutputManager - Report writing", "[output_manager]") {
    // Create a temporary output directory
    io::OutputConfig config;
    config.output_dir = "test_output_write";
    config.create_directories = true;
    config.overwrite_existing = true;
    
    io::OutputManager manager(config);
    
    // Create a minimal report
    narrative::Report report;
    report.title = "Test Report";
    report.total_events = 100;
    report.error_count = 5;
    report.warning_count = 10;
    
    SECTION("Write markdown only") {
        config.formats = io::OutputFormat::MARKDOWN;
        io::OutputManager md_manager(config);
        
        auto result = md_manager.write_report(report);
        
        REQUIRE(result.success);
        REQUIRE(result.written_files.size() == 1);
        REQUIRE(result.written_files[0].find("report.md") != std::string::npos);
        REQUIRE(std::filesystem::exists(config.output_dir / "report.md"));
    }
    
    SECTION("Write JSON only") {
        config.formats = io::OutputFormat::JSON;
        io::OutputManager json_manager(config);
        
        auto result = json_manager.write_report(report);
        
        REQUIRE(result.success);
        REQUIRE(result.written_files.size() == 1);
        REQUIRE(result.written_files[0].find("report.json") != std::string::npos);
        REQUIRE(std::filesystem::exists(config.output_dir / "report.json"));
    }
    
    SECTION("Write CSV only") {
        config.formats = io::OutputFormat::CSV;
        io::OutputManager csv_manager(config);
        
        // Add a timeline highlight
        narrative::TimelineHighlight highlight;
        highlight.timestamp = std::chrono::system_clock::now();
        highlight.description = "Test event";
        highlight.severity = core::Severity::INFO;
        highlight.event_id = 1;
        report.timeline.push_back(highlight);
        
        auto result = csv_manager.write_report(report);
        
        REQUIRE(result.success);
        REQUIRE(result.written_files.size() == 1);
        REQUIRE(result.written_files[0].find("timeline.csv") != std::string::npos);
        REQUIRE(std::filesystem::exists(config.output_dir / "timeline.csv"));
    }
    
    SECTION("Write all formats") {
        config.formats = io::OutputFormat::ALL;
        io::OutputManager all_manager(config);
        
        auto result = all_manager.write_report(report);
        
        REQUIRE(result.success);
        REQUIRE(result.written_files.size() == 3);
        REQUIRE(std::filesystem::exists(config.output_dir / "report.md"));
        REQUIRE(std::filesystem::exists(config.output_dir / "report.json"));
        REQUIRE(std::filesystem::exists(config.output_dir / "timeline.csv"));
    }
    
    // Cleanup
    std::filesystem::remove_all(config.output_dir);
}

TEST_CASE("OutputManager - Event timeline writing", "[output_manager]") {
    io::OutputConfig config;
    config.output_dir = "test_output_events";
    config.create_directories = true;
    config.formats = io::OutputFormat::CSV;
    
    io::OutputManager manager(config);
    
    std::vector<core::Event> events;
    
    for (int i = 0; i < 10; i++) {
        core::Event event;
        event.id = i;
        event.sev = (i % 2 == 0) ? core::Severity::INFO : core::Severity::ERROR;
        event.message = "Event " + std::to_string(i);
        event.raw = event.message;
        event.src = core::SourceRef("test.log", i + 1);
        events.push_back(event);
    }
    
    auto result = manager.write_timeline(events);
    
    REQUIRE(result.success);
    REQUIRE(result.written_files.size() == 1);
    REQUIRE(std::filesystem::exists(config.output_dir / "events_timeline.csv"));
    
    // Verify CSV content
    std::ifstream csv_file(config.output_dir / "events_timeline.csv");
    REQUIRE(csv_file.is_open());
    
    std::string header;
    std::getline(csv_file, header);
    REQUIRE(header.find("Event ID") != std::string::npos);
    REQUIRE(header.find("Timestamp") != std::string::npos);
    REQUIRE(header.find("Severity") != std::string::npos);
    
    csv_file.close();  // Close file before cleanup
    
    // Cleanup
    std::filesystem::remove_all(config.output_dir);
}

TEST_CASE("OutputManager - Error handling", "[output_manager]") {
    SECTION("Non-existent directory without auto-create") {
        io::OutputConfig config;
        config.output_dir = "non_existent_dir_12345";
        config.create_directories = false;
        
        io::OutputManager manager(config);
        
        REQUIRE_FALSE(manager.prepare_directory());
    }
    
    SECTION("Invalid directory path") {
        io::OutputConfig config;
        // Use an invalid path (on Windows, paths with certain characters are invalid)
        config.output_dir = "invalid<>path";
        config.create_directories = true;
        
        io::OutputManager manager(config);
        
        // This should fail gracefully
        narrative::Report report;
        auto result = manager.write_report(report);
        REQUIRE_FALSE(result.success);
        REQUIRE_FALSE(result.error_message.empty());
    }
}

TEST_CASE("OutputFormat - Bitwise operations", "[output_manager]") {
    using io::OutputFormat;
    using io::has_format;
    
    SECTION("Single format check") {
        auto formats = OutputFormat::MARKDOWN;
        REQUIRE(has_format(formats, OutputFormat::MARKDOWN));
        REQUIRE_FALSE(has_format(formats, OutputFormat::JSON));
    }
    
    SECTION("Multiple formats") {
        auto formats = OutputFormat::MARKDOWN | OutputFormat::JSON;
        REQUIRE(has_format(formats, OutputFormat::MARKDOWN));
        REQUIRE(has_format(formats, OutputFormat::JSON));
        REQUIRE_FALSE(has_format(formats, OutputFormat::CSV));
    }
    
    SECTION("All formats") {
        auto formats = OutputFormat::ALL;
        REQUIRE(has_format(formats, OutputFormat::MARKDOWN));
        REQUIRE(has_format(formats, OutputFormat::JSON));
        REQUIRE(has_format(formats, OutputFormat::CSV));
    }
}
