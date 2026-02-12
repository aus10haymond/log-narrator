#include <catch2/catch_test_macros.hpp>
#include "logstory/core/event.hpp"

using namespace logstory::core;

TEST_CASE("Event default construction", "[event]") {
    Event e;
    
    REQUIRE(e.id == 0);
    REQUIRE(e.sev == Severity::UNKNOWN);
    REQUIRE(e.message.empty());
    REQUIRE(e.raw.empty());
    REQUIRE(e.tags.empty());
    REQUIRE_FALSE(e.ts.has_value());
}

TEST_CASE("Event construction with ID and source", "[event]") {
    SourceRef src("test.log", 42);
    Event e(123, src);
    
    REQUIRE(e.id == 123);
    REQUIRE(e.src.source_path == "test.log");
    REQUIRE(e.src.start_line == 42);
    REQUIRE(e.sev == Severity::UNKNOWN);
}

TEST_CASE("Event can store timestamp", "[event]") {
    Event e;
    
    auto now = std::chrono::system_clock::now();
    e.ts = Timestamp(now, 95, true);
    
    REQUIRE(e.ts.has_value());
    REQUIRE(e.ts->confidence == 95);
    REQUIRE(e.ts->tz_known == true);
}

TEST_CASE("Event can store severity", "[event]") {
    Event e;
    e.sev = Severity::ERROR;
    
    REQUIRE(e.sev == Severity::ERROR);
}

TEST_CASE("Event can store message and raw text", "[event]") {
    Event e;
    e.message = "An error occurred";
    e.raw = "2024-01-15 10:30:00 ERROR An error occurred";
    
    REQUIRE(e.message == "An error occurred");
    REQUIRE(e.raw == "2024-01-15 10:30:00 ERROR An error occurred");
}

TEST_CASE("Event can store tags", "[event]") {
    Event e;
    e.tags["user_id"] = "12345";
    e.tags["request_id"] = "abc-def-123";
    
    REQUIRE(e.tags.size() == 2);
    REQUIRE(e.tags["user_id"] == "12345");
    REQUIRE(e.tags["request_id"] == "abc-def-123");
}

TEST_CASE("Timestamp default construction", "[timestamp]") {
    Timestamp ts;
    
    REQUIRE(ts.confidence == 0);
    REQUIRE(ts.tz_known == false);
    REQUIRE_FALSE(ts.is_valid());
}

TEST_CASE("Timestamp with time_point", "[timestamp]") {
    auto now = std::chrono::system_clock::now();
    Timestamp ts(now);
    
    REQUIRE(ts.tp == now);
    REQUIRE(ts.confidence == 100);
    REQUIRE(ts.tz_known == false);
    REQUIRE(ts.is_valid());
}

TEST_CASE("Timestamp with confidence and timezone", "[timestamp]") {
    auto now = std::chrono::system_clock::now();
    Timestamp ts(now, 75, true);
    
    REQUIRE(ts.confidence == 75);
    REQUIRE(ts.tz_known == true);
    REQUIRE(ts.is_valid());
}

TEST_CASE("Timestamp is_valid based on confidence", "[timestamp]") {
    auto now = std::chrono::system_clock::now();
    
    Timestamp ts1(now, 0, false);
    REQUIRE_FALSE(ts1.is_valid());
    
    Timestamp ts2(now, 1, false);
    REQUIRE(ts2.is_valid());
    
    Timestamp ts3(now, 100, true);
    REQUIRE(ts3.is_valid());
}

TEST_CASE("EventId is uint64_t", "[event_id]") {
    EventId id = 12345678901234;
    REQUIRE(id == 12345678901234);
    
    // Verify it can hold large values
    EventId large = UINT64_MAX;
    REQUIRE(large == UINT64_MAX);
}

TEST_CASE("TagMap is unordered_map", "[tags]") {
    TagMap tags;
    tags["key1"] = "value1";
    tags["key2"] = "value2";
    
    REQUIRE(tags.size() == 2);
    REQUIRE(tags["key1"] == "value1");
    REQUIRE(tags["key2"] == "value2");
    
    // Can iterate
    int count = 0;
    for (const auto& [key, value] : tags) {
        count++;
    }
    REQUIRE(count == 2);
}

TEST_CASE("Event with all fields populated", "[event]") {
    auto now = std::chrono::system_clock::now();
    
    Event e;
    e.id = 999;
    e.ts = Timestamp(now, 90, true);
    e.sev = Severity::WARN;
    e.message = "Warning message";
    e.src = SourceRef("app.log", 10, 15);
    e.tags["thread"] = "worker-1";
    e.tags["host"] = "server-01";
    e.raw = "2024-01-15 10:30:00 WARN [worker-1] Warning message";
    
    REQUIRE(e.id == 999);
    REQUIRE(e.ts.has_value());
    REQUIRE(e.ts->confidence == 90);
    REQUIRE(e.sev == Severity::WARN);
    REQUIRE(e.message == "Warning message");
    REQUIRE(e.src.source_path == "app.log");
    REQUIRE(e.src.start_line == 10);
    REQUIRE(e.src.end_line == 15);
    REQUIRE(e.tags.size() == 2);
    REQUIRE(e.raw == "2024-01-15 10:30:00 WARN [worker-1] Warning message");
}
