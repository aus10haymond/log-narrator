#include <catch2/catch_test_macros.hpp>
#include "logstory/io/file_reader.hpp"
#include <filesystem>
#include <fstream>

using namespace logstory::io;
using namespace logstory::core;

namespace fs = std::filesystem;

// Helper to get absolute path to test fixtures
static std::string get_fixture_path(const std::string& filename) {
    // Get the path relative to the tests directory
    fs::path test_dir = fs::current_path();
    
    // Navigate to find the fixtures directory
    // This assumes tests run from build directory
    fs::path fixtures = test_dir / ".." / "tests" / "fixtures" / "logs" / filename;
    
    // If not found, try alternate path (running from project root)
    if (!fs::exists(fixtures)) {
        fixtures = test_dir / "tests" / "fixtures" / "logs" / filename;
    }
    
    return fixtures.string();
}

TEST_CASE("FileReader reads file content correctly", "[file_reader]") {
    FileReader reader;
    std::vector<RawLine> lines;
    
    std::string path = get_fixture_path("simple.log");
    Status status = reader.read(path, lines);
    
    REQUIRE(status.ok());
    REQUIRE(lines.size() == 3);
    
    REQUIRE(lines[0].text == "Line 1");
    REQUIRE(lines[0].line_no == 1);
    REQUIRE(lines[0].source_path == path);
    
    REQUIRE(lines[1].text == "Line 2");
    REQUIRE(lines[1].line_no == 2);
    REQUIRE(lines[1].source_path == path);
    
    REQUIRE(lines[2].text == "Line 3");
    REQUIRE(lines[2].line_no == 3);
    REQUIRE(lines[2].source_path == path);
}

TEST_CASE("FileReader normalizes Windows line endings", "[file_reader]") {
    FileReader reader;
    std::vector<RawLine> lines;
    
    std::string path = get_fixture_path("windows_endings.txt");
    Status status = reader.read(path, lines);
    
    REQUIRE(status.ok());
    REQUIRE(lines.size() == 3);
    
    // Check that \r has been stripped
    REQUIRE(lines[0].text == "Windows line 1");
    REQUIRE(lines[1].text == "Windows line 2");
    REQUIRE(lines[2].text == "Windows line 3");
    
    // Verify no trailing \r
    for (const auto& line : lines) {
        bool has_trailing_cr = (!line.text.empty() && line.text.back() == '\r');
        REQUIRE_FALSE(has_trailing_cr);
    }
}

TEST_CASE("FileReader reports correct line numbers", "[file_reader]") {
    FileReader reader;
    std::vector<RawLine> lines;
    
    std::string path = get_fixture_path("simple.log");
    Status status = reader.read(path, lines);
    
    REQUIRE(status.ok());
    
    for (size_t i = 0; i < lines.size(); ++i) {
        REQUIRE(lines[i].line_no == static_cast<uint32_t>(i + 1));
    }
}

TEST_CASE("FileReader assigns correct source path", "[file_reader]") {
    FileReader reader;
    std::vector<RawLine> lines;
    
    std::string path = get_fixture_path("simple.log");
    Status status = reader.read(path, lines);
    
    REQUIRE(status.ok());
    
    for (const auto& line : lines) {
        REQUIRE(line.source_path == path);
    }
}

TEST_CASE("FileReader handles non-existent file", "[file_reader]") {
    FileReader reader;
    std::vector<RawLine> lines;
    
    Status status = reader.read("nonexistent_file.log", lines);
    
    REQUIRE_FALSE(status.ok());
    REQUIRE(status.code == ErrorCode::FILE_NOT_FOUND);
    REQUIRE_FALSE(status.message.empty());
    REQUIRE(lines.empty());
}

TEST_CASE("FileReader handles directory path", "[file_reader]") {
    FileReader reader;
    std::vector<RawLine> lines;
    
    // Try to read a directory instead of a file
    std::string dir_path = get_fixture_path("..");
    Status status = reader.read(dir_path, lines);
    
    REQUIRE_FALSE(status.ok());
    REQUIRE(status.code == ErrorCode::INVALID_INPUT);
    REQUIRE(lines.empty());
}

TEST_CASE("FileReader handles empty file", "[file_reader]") {
    FileReader reader;
    std::vector<RawLine> lines;
    
    // Create a temporary empty file
    std::string temp_path = "temp_empty_file.log";
    {
        std::ofstream temp_file(temp_path);
        // Create empty file
    }
    
    Status status = reader.read(temp_path, lines);
    
    REQUIRE(status.ok());
    REQUIRE(lines.empty());
    
    // Cleanup
    fs::remove(temp_path);
}

TEST_CASE("FileReader handles file with blank lines", "[file_reader]") {
    FileReader reader;
    std::vector<RawLine> lines;
    
    // Create a temporary file with blank lines
    std::string temp_path = "temp_blank_lines.log";
    {
        std::ofstream temp_file(temp_path);
        temp_file << "Line 1\n";
        temp_file << "\n";
        temp_file << "Line 3\n";
    }
    
    Status status = reader.read(temp_path, lines);
    
    REQUIRE(status.ok());
    REQUIRE(lines.size() == 3);
    REQUIRE(lines[0].text == "Line 1");
    REQUIRE(lines[1].text == "");
    REQUIRE(lines[2].text == "Line 3");
    
    // Cleanup
    fs::remove(temp_path);
}
