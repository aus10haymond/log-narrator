#include <catch2/catch_test_macros.hpp>
#include "logstory/core/source_ref.hpp"

using namespace logstory::core;

TEST_CASE("SourceRef formats single-line reference correctly", "[source_ref]") {
    SourceRef ref("test.log", 42);
    REQUIRE(ref.to_string() == "test.log:42");
}

TEST_CASE("SourceRef formats multi-line reference correctly", "[source_ref]") {
    SourceRef ref("test.log", 10, 15);
    REQUIRE(ref.to_string() == "test.log:10-15");
}

TEST_CASE("SourceRef with same start and end line formats as single-line", "[source_ref]") {
    SourceRef ref("test.log", 5, 5);
    REQUIRE(ref.to_string() == "test.log:5");
}

TEST_CASE("SourceRef default constructor initializes correctly", "[source_ref]") {
    SourceRef ref;
    REQUIRE(ref.source_path.empty());
    REQUIRE(ref.start_line == 0);
    REQUIRE(ref.end_line == 0);
    REQUIRE(ref.to_string() == ":0");
}

TEST_CASE("SourceRef with path containing spaces", "[source_ref]") {
    SourceRef ref("path with spaces.log", 100);
    REQUIRE(ref.to_string() == "path with spaces.log:100");
}

TEST_CASE("SourceRef with path containing special characters", "[source_ref]") {
    SourceRef ref("C:\\Users\\test\\logs\\app.log", 25, 30);
    REQUIRE(ref.to_string() == "C:\\Users\\test\\logs\\app.log:25-30");
}

TEST_CASE("SourceRef with stdin as source path", "[source_ref]") {
    SourceRef ref("stdin", 1, 5);
    REQUIRE(ref.to_string() == "stdin:1-5");
}

TEST_CASE("SourceRef with large line numbers", "[source_ref]") {
    SourceRef ref("large.log", 999999, 1000000);
    REQUIRE(ref.to_string() == "large.log:999999-1000000");
}
