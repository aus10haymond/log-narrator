#include <catch2/catch_test_macros.hpp>
#include "logstory/parsing/timestamp_detector.hpp"

using namespace logstory::parsing;
using namespace logstory::core;

TEST_CASE("TimestampDetector detects ISO 8601 date", "[timestamp_detector]") {
    TimestampDetector detector;
    
    auto ts = detector.detect("2024-01-15 Log message");
    
    REQUIRE(ts.has_value());
    REQUIRE(ts->confidence >= 90);
}

TEST_CASE("TimestampDetector detects ISO 8601 with time", "[timestamp_detector]") {
    TimestampDetector detector;
    
    auto ts = detector.detect("2024-01-15 10:30:45 ERROR Something failed");
    
    REQUIRE(ts.has_value());
    REQUIRE(ts->confidence >= 90);
}

TEST_CASE("TimestampDetector detects ISO 8601 with T separator", "[timestamp_detector]") {
    TimestampDetector detector;
    
    auto ts = detector.detect("2024-01-15T10:30:45Z Message");
    
    REQUIRE(ts.has_value());
    REQUIRE(ts->confidence >= 90);
    REQUIRE(ts->tz_known);
}

TEST_CASE("TimestampDetector detects common YYYY-MM-DD HH:MM:SS pattern", "[timestamp_detector]") {
    TimestampDetector detector;
    
    auto ts = detector.detect("2024-01-15 10:30:45 Message");
    
    REQUIRE(ts.has_value());
    REQUIRE(ts->confidence >= 85);
}

TEST_CASE("TimestampDetector detects syslog format", "[timestamp_detector]") {
    TimestampDetector detector;
    
    auto ts = detector.detect("Jan 15 10:30:45 hostname service: message");
    
    REQUIRE(ts.has_value());
    REQUIRE(ts->confidence >= 60); // Lower confidence due to missing year
}

TEST_CASE("TimestampDetector detects epoch seconds", "[timestamp_detector]") {
    TimestampDetector detector;
    
    // Unix timestamp for Jan 1, 2024
    auto ts = detector.detect("1704067200 Log message");
    
    REQUIRE(ts.has_value());
    REQUIRE(ts->confidence >= 50);
}

TEST_CASE("TimestampDetector detects epoch milliseconds", "[timestamp_detector]") {
    TimestampDetector detector;
    
    auto ts = detector.detect("1704067200000 Log message");
    
    REQUIRE(ts.has_value());
    REQUIRE(ts->confidence >= 50);
}

TEST_CASE("TimestampDetector returns nullopt for no timestamp", "[timestamp_detector]") {
    TimestampDetector detector;
    
    auto ts = detector.detect("Just a log message with no timestamp");
    
    REQUIRE_FALSE(ts.has_value());
}

TEST_CASE("TimestampDetector handles timestamp in middle of text", "[timestamp_detector]") {
    TimestampDetector detector;
    
    auto ts = detector.detect("[ERROR] 2024-01-15 10:30:45 Something failed");
    
    REQUIRE(ts.has_value());
}

TEST_CASE("TimestampDetector rejects invalid dates", "[timestamp_detector]") {
    TimestampDetector detector;
    
    // Invalid month
    auto ts1 = detector.detect("2024-13-01 Invalid month");
    REQUIRE_FALSE(ts1.has_value());
    
    // Invalid day
    auto ts2 = detector.detect("2024-01-32 Invalid day");
    REQUIRE_FALSE(ts2.has_value());
}
