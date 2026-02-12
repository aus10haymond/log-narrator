#include <catch2/catch_test_macros.hpp>
#include "logstory/io/dir_scanner.hpp"
#include <filesystem>
#include <fstream>
#include <algorithm>

using namespace logstory::io;
using namespace logstory::core;

namespace fs = std::filesystem;

// Helper to get absolute path to test fixtures directory
static std::string get_fixtures_dir() {
    fs::path test_dir = fs::current_path();
    
    // Try build directory structure
    fs::path fixtures = test_dir / ".." / "tests" / "fixtures" / "logs";
    
    // If not found, try alternate path (running from project root)
    if (!fs::exists(fixtures)) {
        fixtures = test_dir / "tests" / "fixtures" / "logs";
    }
    
    return fixtures.string();
}

TEST_CASE("DirScanner identifies files with correct extensions", "[dir_scanner]") {
    DirScanner scanner;
    std::vector<std::string> files;
    
    std::string fixtures_dir = get_fixtures_dir();
    Status status = scanner.scan(fixtures_dir, files);
    
    REQUIRE(status.ok());
    REQUIRE_FALSE(files.empty());
    
    // Should find .log, .txt, and .jsonl files
    bool found_log = false;
    bool found_txt = false;
    bool found_jsonl = false;
    
    for (const auto& file : files) {
        fs::path p(file);
        std::string ext = p.extension().string();
        
        if (ext == ".log") found_log = true;
        else if (ext == ".txt") found_txt = true;
        else if (ext == ".jsonl") found_jsonl = true;
    }
    
    REQUIRE(found_log);
    REQUIRE(found_txt);
    REQUIRE(found_jsonl);
}

TEST_CASE("DirScanner sorts files lexicographically", "[dir_scanner]") {
    DirScanner scanner;
    std::vector<std::string> files;
    
    std::string fixtures_dir = get_fixtures_dir();
    Status status = scanner.scan(fixtures_dir, files);
    
    REQUIRE(status.ok());
    REQUIRE(files.size() >= 2);
    
    // Verify files are sorted
    std::vector<std::string> sorted_files = files;
    std::sort(sorted_files.begin(), sorted_files.end());
    
    REQUIRE(files == sorted_files);
}

TEST_CASE("DirScanner handles non-existent directory", "[dir_scanner]") {
    DirScanner scanner;
    std::vector<std::string> files;
    
    Status status = scanner.scan("nonexistent_directory", files);
    
    REQUIRE_FALSE(status.ok());
    REQUIRE(status.code == ErrorCode::DIRECTORY_NOT_FOUND);
    REQUIRE_FALSE(status.message.empty());
    REQUIRE(files.empty());
}

TEST_CASE("DirScanner handles file path instead of directory", "[dir_scanner]") {
    DirScanner scanner;
    std::vector<std::string> files;
    
    // Try to scan a file instead of a directory
    std::string fixtures_dir = get_fixtures_dir();
    std::string file_path = fixtures_dir + "/simple.log";
    
    Status status = scanner.scan(file_path, files);
    
    REQUIRE_FALSE(status.ok());
    REQUIRE(status.code == ErrorCode::INVALID_INPUT);
    REQUIRE(files.empty());
}

TEST_CASE("DirScanner handles empty directory", "[dir_scanner]") {
    DirScanner scanner;
    std::vector<std::string> files;
    
    // Create a temporary empty directory
    std::string temp_dir = "temp_empty_dir";
    fs::create_directory(temp_dir);
    
    Status status = scanner.scan(temp_dir, files);
    
    REQUIRE_FALSE(status.ok());
    REQUIRE(status.code == ErrorCode::DIRECTORY_EMPTY);
    REQUIRE(files.empty());
    
    // Cleanup
    fs::remove(temp_dir);
}

TEST_CASE("DirScanner handles custom extensions", "[dir_scanner]") {
    DirScanner scanner;
    std::vector<std::string> files;
    
    std::string fixtures_dir = get_fixtures_dir();
    std::vector<std::string> custom_exts = {".log"};
    
    Status status = scanner.scan(fixtures_dir, custom_exts, files);
    
    REQUIRE(status.ok());
    REQUIRE_FALSE(files.empty());
    
    // All files should have .log extension
    for (const auto& file : files) {
        fs::path p(file);
        std::string ext = p.extension().string();
        
        // Convert to lowercase for comparison
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        REQUIRE(ext == ".log");
    }
}

TEST_CASE("DirScanner with custom extensions excludes other files", "[dir_scanner]") {
    DirScanner scanner;
    std::vector<std::string> files;
    
    std::string fixtures_dir = get_fixtures_dir();
    std::vector<std::string> custom_exts = {".txt"};
    
    Status status = scanner.scan(fixtures_dir, custom_exts, files);
    
    REQUIRE(status.ok());
    
    // No .log or .jsonl files should be present
    for (const auto& file : files) {
        fs::path p(file);
        std::string ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        REQUIRE(ext == ".txt");
    }
}

TEST_CASE("DirScanner is case-insensitive for extensions", "[dir_scanner]") {
    DirScanner scanner;
    std::vector<std::string> files;
    
    // Create a temporary directory with files having different case extensions
    std::string temp_dir = "temp_case_test";
    fs::create_directory(temp_dir);
    
    // Create files with different case extensions
    std::ofstream(temp_dir + "/file1.LOG");
    std::ofstream(temp_dir + "/file2.Log");
    std::ofstream(temp_dir + "/file3.log");
    
    std::vector<std::string> exts = {".log"};
    Status status = scanner.scan(temp_dir, exts, files);
    
    REQUIRE(status.ok());
    REQUIRE(files.size() == 3);
    
    // Cleanup
    fs::remove_all(temp_dir);
}

TEST_CASE("DirScanner scans recursively", "[dir_scanner]") {
    DirScanner scanner;
    std::vector<std::string> files;
    
    // Create a temporary directory structure
    std::string temp_dir = "temp_recursive_test";
    std::string subdir = temp_dir + "/subdir";
    fs::create_directories(subdir);
    
    std::ofstream(temp_dir + "/root.log");
    std::ofstream(subdir + "/nested.log");
    
    Status status = scanner.scan(temp_dir, files);
    
    REQUIRE(status.ok());
    REQUIRE(files.size() == 2);
    
    // Check that both files are found
    bool found_root = false;
    bool found_nested = false;
    
    for (const auto& file : files) {
        if (file.find("root.log") != std::string::npos) found_root = true;
        if (file.find("nested.log") != std::string::npos) found_nested = true;
    }
    
    REQUIRE(found_root);
    REQUIRE(found_nested);
    
    // Cleanup
    fs::remove_all(temp_dir);
}

TEST_CASE("DirScanner default extensions include .log, .txt, .jsonl", "[dir_scanner]") {
    const auto& defaults = DirScanner::DEFAULT_EXTENSIONS;
    
    REQUIRE(defaults.size() == 3);
    REQUIRE(std::find(defaults.begin(), defaults.end(), ".log") != defaults.end());
    REQUIRE(std::find(defaults.begin(), defaults.end(), ".txt") != defaults.end());
    REQUIRE(std::find(defaults.begin(), defaults.end(), ".jsonl") != defaults.end());
}
