#include <catch2/catch_test_macros.hpp>
#include "logstory/io/multiline_framer.hpp"
#include "logstory/io/file_reader.hpp"
#include <filesystem>

using namespace logstory::io;
using namespace logstory::core;

namespace fs = std::filesystem;

// Helper to get fixture path
static std::string get_fixture_path(const std::string& filename) {
    fs::path test_dir = fs::current_path();
    fs::path fixtures = test_dir / ".." / "tests" / "fixtures" / "logs" / filename;
    
    if (!fs::exists(fixtures)) {
        fixtures = test_dir / "tests" / "fixtures" / "logs" / filename;
    }
    
    return fixtures.string();
}

TEST_CASE("MultilineFramer handles single-line records", "[multiline_framer]") {
    MultilineFramer framer;
    std::vector<RawLine> lines;
    std::vector<Record> records;
    
    lines.emplace_back("Normal line 1", "test.log", 1);
    lines.emplace_back("Normal line 2", "test.log", 2);
    lines.emplace_back("Normal line 3", "test.log", 3);
    
    framer.frame(lines, records);
    
    REQUIRE(records.size() == 3);
    REQUIRE(records[0].text == "Normal line 1");
    REQUIRE(records[1].text == "Normal line 2");
    REQUIRE(records[2].text == "Normal line 3");
}

TEST_CASE("MultilineFramer merges Java stack trace", "[multiline_framer]") {
    MultilineFramer framer;
    std::vector<RawLine> lines;
    std::vector<Record> records;
    
    lines.emplace_back("ERROR Exception occurred", "test.log", 1);
    lines.emplace_back("java.lang.NullPointerException: null", "test.log", 2);
    lines.emplace_back("\tat com.example.Service.method(Service.java:45)", "test.log", 3);
    lines.emplace_back("\tat com.example.Main.main(Main.java:10)", "test.log", 4);
    lines.emplace_back("INFO Recovery started", "test.log", 5);
    
    framer.frame(lines, records);
    
    // Should be 2 records: the error with stack trace, and the recovery message
    REQUIRE(records.size() == 2);
    
    // First record should contain all stack trace lines
    REQUIRE(records[0].src.start_line == 1);
    REQUIRE(records[0].src.end_line == 4);
    REQUIRE(records[0].text.find("ERROR Exception occurred") != std::string::npos);
    REQUIRE(records[0].text.find("NullPointerException") != std::string::npos);
    REQUIRE(records[0].text.find("at com.example.Service.method") != std::string::npos);
    
    // Second record is the recovery message
    REQUIRE(records[1].text == "INFO Recovery started");
    REQUIRE(records[1].src.start_line == 5);
}

TEST_CASE("MultilineFramer merges Python stack trace", "[multiline_framer]") {
    MultilineFramer framer;
    std::vector<RawLine> lines;
    std::vector<Record> records;
    
    lines.emplace_back("ERROR Unhandled exception", "test.log", 1);
    lines.emplace_back("Traceback (most recent call last):", "test.log", 2);
    lines.emplace_back("  File \"app.py\", line 42, in main", "test.log", 3);
    lines.emplace_back("    result = process()", "test.log", 4);
    lines.emplace_back("ValueError: invalid input", "test.log", 5);
    lines.emplace_back("INFO Continuing", "test.log", 6);
    
    framer.frame(lines, records);
    
    REQUIRE(records.size() == 2);
    
    // First record contains the full traceback
    REQUIRE(records[0].src.start_line == 1);
    REQUIRE(records[0].src.end_line == 5);
    REQUIRE(records[0].text.find("Traceback") != std::string::npos);
    REQUIRE(records[0].text.find("File \"app.py\"") != std::string::npos);
    REQUIRE(records[0].text.find("ValueError") != std::string::npos);
    
    // Second record is separate
    REQUIRE(records[1].text == "INFO Continuing");
}

TEST_CASE("MultilineFramer handles 'Caused by' continuation", "[multiline_framer]") {
    MultilineFramer framer;
    std::vector<RawLine> lines;
    std::vector<Record> records;
    
    lines.emplace_back("ERROR Main exception", "test.log", 1);
    lines.emplace_back("RuntimeException: Failed", "test.log", 2);
    lines.emplace_back("Caused by: SQLException: Connection failed", "test.log", 3);
    lines.emplace_back("\tat db.connect()", "test.log", 4);
    lines.emplace_back("INFO Next operation", "test.log", 5);
    
    framer.frame(lines, records);
    
    REQUIRE(records.size() == 2);
    REQUIRE(records[0].src.end_line == 4);
    REQUIRE(records[0].text.find("Caused by") != std::string::npos);
}

TEST_CASE("MultilineFramer respects max_lines_per_record limit", "[multiline_framer]") {
    MultilineFramerConfig config;
    config.max_lines_per_record = 3;
    MultilineFramer framer(config);
    
    std::vector<RawLine> lines;
    std::vector<Record> records;
    
    lines.emplace_back("ERROR Exception", "test.log", 1);
    lines.emplace_back("\tat line 2", "test.log", 2);
    lines.emplace_back("\tat line 3", "test.log", 3);
    lines.emplace_back("\tat line 4", "test.log", 4);
    lines.emplace_back("\tat line 5", "test.log", 5);
    
    framer.frame(lines, records);
    
    // Should split because max_lines_per_record is 3
    REQUIRE(records.size() > 1);
    
    // First record should have at most 3 lines
    int first_record_lines = records[0].src.end_line - records[0].src.start_line + 1;
    REQUIRE(first_record_lines <= 3);
}

TEST_CASE("MultilineFramer respects max_chars_per_record limit", "[multiline_framer]") {
    MultilineFramerConfig config;
    config.max_chars_per_record = 50;
    MultilineFramer framer(config);
    
    std::vector<RawLine> lines;
    std::vector<Record> records;
    
    lines.emplace_back("ERROR Exception occurred", "test.log", 1);
    lines.emplace_back("\tat very long method name that exceeds limit", "test.log", 2);
    lines.emplace_back("\tat another method", "test.log", 3);
    
    framer.frame(lines, records);
    
    // Should split to respect character limit
    for (const auto& record : records) {
        REQUIRE(record.text.size() <= config.max_chars_per_record);
    }
}

TEST_CASE("MultilineFramer does not merge unrelated lines", "[multiline_framer]") {
    MultilineFramer framer;
    std::vector<RawLine> lines;
    std::vector<Record> records;
    
    lines.emplace_back("INFO Normal log line", "test.log", 1);
    lines.emplace_back("  This is indented but not an error continuation", "test.log", 2);
    lines.emplace_back("DEBUG Another line", "test.log", 3);
    
    framer.frame(lines, records);
    
    // Indented line after non-error should not merge
    REQUIRE(records.size() == 3);
}

TEST_CASE("MultilineFramer handles empty lines", "[multiline_framer]") {
    MultilineFramer framer;
    std::vector<RawLine> lines;
    std::vector<Record> records;
    
    lines.emplace_back("Line 1", "test.log", 1);
    lines.emplace_back("", "test.log", 2);
    lines.emplace_back("Line 3", "test.log", 3);
    
    framer.frame(lines, records);
    
    REQUIRE(records.size() == 3);
    REQUIRE(records[0].text == "Line 1");
    REQUIRE(records[1].text == "");
    REQUIRE(records[2].text == "Line 3");
}

TEST_CASE("MultilineFramer handles empty input", "[multiline_framer]") {
    MultilineFramer framer;
    std::vector<RawLine> lines;
    std::vector<Record> records;
    
    framer.frame(lines, records);
    
    REQUIRE(records.empty());
}

TEST_CASE("MultilineFramer preserves raw text with newlines", "[multiline_framer]") {
    MultilineFramer framer;
    std::vector<RawLine> lines;
    std::vector<Record> records;
    
    lines.emplace_back("ERROR Exception", "test.log", 1);
    lines.emplace_back("\tat method1", "test.log", 2);
    lines.emplace_back("\tat method2", "test.log", 3);
    
    framer.frame(lines, records);
    
    REQUIRE(records.size() == 1);
    
    // Should contain newlines between merged lines
    std::string expected = "ERROR Exception\n\tat method1\n\tat method2";
    REQUIRE(records[0].text == expected);
}

TEST_CASE("MultilineFramer with Python stack trace fixture", "[multiline_framer][integration]") {
    std::string path = get_fixture_path("python_stacktrace.log");
    
    FileReader reader;
    std::vector<RawLine> lines;
    Status status = reader.read(path, lines);
    
    if (!status.ok()) {
        // Skip if fixture not found
        SKIP("Fixture file not found");
    }
    
    MultilineFramer framer;
    std::vector<Record> records;
    framer.frame(lines, records);
    
    // Should have 3 records: INFO, ERROR+Traceback, INFO
    REQUIRE(records.size() == 3);
    
    // Middle record should be the multiline error
    REQUIRE(records[1].text.find("Traceback") != std::string::npos);
    REQUIRE(records[1].text.find("ValueError") != std::string::npos);
    REQUIRE(records[1].src.start_line < records[1].src.end_line);
}

TEST_CASE("MultilineFramer with Java stack trace fixture", "[multiline_framer][integration]") {
    std::string path = get_fixture_path("java_stacktrace.log");
    
    FileReader reader;
    std::vector<RawLine> lines;
    Status status = reader.read(path, lines);
    
    if (!status.ok()) {
        SKIP("Fixture file not found");
    }
    
    MultilineFramer framer;
    std::vector<Record> records;
    framer.frame(lines, records);
    
    // Should have 3 records: INFO, ERROR+stack, WARN
    REQUIRE(records.size() == 3);
    
    // Middle record should contain the full stack trace
    REQUIRE(records[1].text.find("NullPointerException") != std::string::npos);
    REQUIRE(records[1].text.find("Caused by") != std::string::npos);
    REQUIRE(records[1].src.start_line < records[1].src.end_line);
}
