#include <catch2/catch_test_macros.hpp>
#include "logstory/io/record_framer.hpp"

using namespace logstory::io;
using namespace logstory::core;

TEST_CASE("RecordFramer creates one record per line", "[record_framer]") {
    RecordFramer framer;
    std::vector<RawLine> lines;
    std::vector<Record> records;
    
    lines.emplace_back("Line 1", "test.log", 1);
    lines.emplace_back("Line 2", "test.log", 2);
    lines.emplace_back("Line 3", "test.log", 3);
    
    framer.frame(lines, records);
    
    REQUIRE(records.size() == 3);
}

TEST_CASE("RecordFramer preserves line text", "[record_framer]") {
    RecordFramer framer;
    std::vector<RawLine> lines;
    std::vector<Record> records;
    
    lines.emplace_back("First line", "test.log", 1);
    lines.emplace_back("Second line", "test.log", 2);
    
    framer.frame(lines, records);
    
    REQUIRE(records[0].text == "First line");
    REQUIRE(records[1].text == "Second line");
}

TEST_CASE("RecordFramer sets correct source references", "[record_framer]") {
    RecordFramer framer;
    std::vector<RawLine> lines;
    std::vector<Record> records;
    
    lines.emplace_back("Line 1", "app.log", 10);
    lines.emplace_back("Line 2", "app.log", 11);
    
    framer.frame(lines, records);
    
    REQUIRE(records[0].src.source_path == "app.log");
    REQUIRE(records[0].src.start_line == 10);
    REQUIRE(records[0].src.end_line == 10);
    
    REQUIRE(records[1].src.source_path == "app.log");
    REQUIRE(records[1].src.start_line == 11);
    REQUIRE(records[1].src.end_line == 11);
}

TEST_CASE("RecordFramer handles empty input", "[record_framer]") {
    RecordFramer framer;
    std::vector<RawLine> lines;
    std::vector<Record> records;
    
    framer.frame(lines, records);
    
    REQUIRE(records.empty());
}

TEST_CASE("RecordFramer handles single line", "[record_framer]") {
    RecordFramer framer;
    std::vector<RawLine> lines;
    std::vector<Record> records;
    
    lines.emplace_back("Only line", "single.log", 1);
    
    framer.frame(lines, records);
    
    REQUIRE(records.size() == 1);
    REQUIRE(records[0].text == "Only line");
    REQUIRE(records[0].src.start_line == 1);
    REQUIRE(records[0].src.end_line == 1);
}

TEST_CASE("RecordFramer handles multiple source files", "[record_framer]") {
    RecordFramer framer;
    std::vector<RawLine> lines;
    std::vector<Record> records;
    
    lines.emplace_back("From file 1", "file1.log", 1);
    lines.emplace_back("From file 2", "file2.log", 1);
    lines.emplace_back("From file 1 again", "file1.log", 2);
    
    framer.frame(lines, records);
    
    REQUIRE(records.size() == 3);
    REQUIRE(records[0].src.source_path == "file1.log");
    REQUIRE(records[1].src.source_path == "file2.log");
    REQUIRE(records[2].src.source_path == "file1.log");
}

TEST_CASE("RecordFramer preserves empty lines", "[record_framer]") {
    RecordFramer framer;
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

TEST_CASE("RecordFramer clears previous results", "[record_framer]") {
    RecordFramer framer;
    std::vector<RawLine> lines;
    std::vector<Record> records;
    
    // First framing
    lines.emplace_back("Line 1", "test.log", 1);
    framer.frame(lines, records);
    REQUIRE(records.size() == 1);
    
    // Second framing with different input
    lines.clear();
    lines.emplace_back("Line A", "test.log", 1);
    lines.emplace_back("Line B", "test.log", 2);
    
    framer.frame(lines, records);
    
    REQUIRE(records.size() == 2);
    REQUIRE(records[0].text == "Line A");
    REQUIRE(records[1].text == "Line B");
}
