#include <catch2/catch_test_macros.hpp>
#include "logstory/parsing/severity_detector.hpp"
#include "logstory/parsing/kv_extractor.hpp"

using namespace logstory::parsing;
using namespace logstory::core;

// Severity Detector Tests

TEST_CASE("SeverityDetector detects bracketed ERROR", "[severity_detector]") {
    SeverityDetector detector;
    REQUIRE(detector.detect("[ERROR] Something failed") == Severity::ERROR);
}

TEST_CASE("SeverityDetector detects bracketed WARN", "[severity_detector]") {
    SeverityDetector detector;
    REQUIRE(detector.detect("[WARN] Warning message") == Severity::WARN);
}

TEST_CASE("SeverityDetector detects bracketed INFO", "[severity_detector]") {
    SeverityDetector detector;
    REQUIRE(detector.detect("[INFO] Information") == Severity::INFO);
}

TEST_CASE("SeverityDetector detects start-of-line ERROR", "[severity_detector]") {
    SeverityDetector detector;
    REQUIRE(detector.detect("ERROR: Connection failed") == Severity::ERROR);
}

TEST_CASE("SeverityDetector detects key=value pattern", "[severity_detector]") {
    SeverityDetector detector;
    REQUIRE(detector.detect("level=error msg=failed") == Severity::ERROR);
    REQUIRE(detector.detect("severity=warn something") == Severity::WARN);
}

TEST_CASE("SeverityDetector detects JSON-like pattern", "[severity_detector]") {
    SeverityDetector detector;
    auto result = detector.detect(R"({"level":"error","msg":"failed"})");
    REQUIRE(result == Severity::ERROR);
}

TEST_CASE("SeverityDetector uses keyword heuristics", "[severity_detector]") {
    SeverityDetector detector;
    REQUIRE(detector.detect("exception occurred") == Severity::ERROR);
    REQUIRE(detector.detect("warning: deprecated") == Severity::WARN);
}

TEST_CASE("SeverityDetector is case-insensitive", "[severity_detector]") {
    SeverityDetector detector;
    REQUIRE(detector.detect("[error] Failed") == Severity::ERROR);
    REQUIRE(detector.detect("[WARN] Warning") == Severity::WARN);
    REQUIRE(detector.detect("[Info] Message") == Severity::INFO);
}

TEST_CASE("SeverityDetector detects FATAL", "[severity_detector]") {
    SeverityDetector detector;
    REQUIRE(detector.detect("[FATAL] Critical error") == Severity::FATAL);
    REQUIRE(detector.detect("CRITICAL failure") == Severity::FATAL);
}

TEST_CASE("SeverityDetector returns UNKNOWN for ambiguous text", "[severity_detector]") {
    SeverityDetector detector;
    REQUIRE(detector.detect("Just a normal message") == Severity::UNKNOWN);
}

// KV Extractor Tests

TEST_CASE("KVExtractor extracts simple key=value", "[kv_extractor]") {
    KVExtractor extractor;
    TagMap tags;
    
    extractor.extract("user_id=12345 request_id=abc123", tags);
    
    REQUIRE(tags.size() == 2);
    REQUIRE(tags["user_id"] == "12345");
    REQUIRE(tags["request_id"] == "abc123");
}

TEST_CASE("KVExtractor extracts quoted values", "[kv_extractor]") {
    KVExtractor extractor;
    TagMap tags;
    
    extractor.extract("name=\"John Doe\" message=\"Hello World\"", tags);
    
    REQUIRE(tags["name"] == "John Doe");
    REQUIRE(tags["message"] == "Hello World");
}

TEST_CASE("KVExtractor extracts single-quoted values", "[kv_extractor]") {
    KVExtractor extractor;
    TagMap tags;
    
    extractor.extract("name='Jane' city='New York'", tags);
    
    REQUIRE(tags["name"] == "Jane");
    REQUIRE(tags["city"] == "New York");
}

TEST_CASE("KVExtractor handles mixed formats", "[kv_extractor]") {
    KVExtractor extractor;
    TagMap tags;
    
    extractor.extract("id=123 name=\"Test\" status=active", tags);
    
    REQUIRE(tags.size() == 3);
    REQUIRE(tags["id"] == "123");
    REQUIRE(tags["name"] == "Test");
    REQUIRE(tags["status"] == "active");
}

TEST_CASE("KVExtractor strips trailing punctuation", "[kv_extractor]") {
    KVExtractor extractor;
    TagMap tags;
    
    extractor.extract("status=success, user=admin;", tags);
    
    REQUIRE(tags["status"] == "success");
    REQUIRE(tags["user"] == "admin");
}

TEST_CASE("KVExtractor handles empty input", "[kv_extractor]") {
    KVExtractor extractor;
    TagMap tags;
    
    extractor.extract("", tags);
    
    REQUIRE(tags.empty());
}

TEST_CASE("KVExtractor handles no key-value pairs", "[kv_extractor]") {
    KVExtractor extractor;
    TagMap tags;
    
    extractor.extract("Just a log message with no pairs", tags);
    
    REQUIRE(tags.empty());
}

TEST_CASE("KVExtractor skips generic keys", "[kv_extractor]") {
    KVExtractor extractor;
    TagMap tags;
    
    extractor.extract("at=info to=server", tags);
    
    // 'at' and 'to' should be skipped as too generic
    REQUIRE(tags.empty());
}

TEST_CASE("KVExtractor handles real log example", "[kv_extractor]") {
    KVExtractor extractor;
    TagMap tags;
    
    extractor.extract("2024-01-15 user_id=555 action=login status=success duration_ms=123", tags);
    
    REQUIRE(tags["user_id"] == "555");
    REQUIRE(tags["action"] == "login");
    REQUIRE(tags["status"] == "success");
    REQUIRE(tags["duration_ms"] == "123");
}
