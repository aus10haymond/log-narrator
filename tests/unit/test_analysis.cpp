#include <catch2/catch_test_macros.hpp>
#include "logstory/analysis/correlation_extractor.hpp"
#include "logstory/analysis/event_index.hpp"

using namespace logstory::analysis;
using namespace logstory::core;

// Correlation Extractor Tests

TEST_CASE("CorrelationExtractor extracts request_id from tags", "[correlation_extractor]") {
    CorrelationExtractor extractor;
    Event event;
    event.raw = "Log message";
    event.tags["requestId"] = "req-12345";
    
    extractor.extract(event);
    
    REQUIRE(event.tags["request_id"] == "req-12345");
}

TEST_CASE("CorrelationExtractor normalizes request_id variants", "[correlation_extractor]") {
    CorrelationExtractor extractor;
    
    Event event1;
    event1.raw = "msg";
    event1.tags["req_id"] = "abc123";
    extractor.extract(event1);
    REQUIRE(event1.tags["request_id"] == "abc123");
    
    Event event2;
    event2.raw = "msg";
    event2.tags["x-request-id"] = "xyz789";
    extractor.extract(event2);
    REQUIRE(event2.tags["request_id"] == "xyz789");
}

TEST_CASE("CorrelationExtractor extracts request_id from text", "[correlation_extractor]") {
    CorrelationExtractor extractor;
    Event event;
    event.raw = "Processing request request_id=abc-123 from user";
    
    extractor.extract(event);
    
    REQUIRE(event.tags["request_id"] == "abc-123");
}

TEST_CASE("CorrelationExtractor extracts trace_id from tags", "[correlation_extractor]") {
    CorrelationExtractor extractor;
    Event event;
    event.raw = "Trace message";
    event.tags["traceId"] = "trace-789";
    
    extractor.extract(event);
    
    REQUIRE(event.tags["trace_id"] == "trace-789");
}

TEST_CASE("CorrelationExtractor normalizes trace_id variants", "[correlation_extractor]") {
    CorrelationExtractor extractor;
    
    Event event1;
    event1.raw = "msg";
    event1.tags["x-trace-id"] = "trace123";
    extractor.extract(event1);
    REQUIRE(event1.tags["trace_id"] == "trace123");
    
    Event event2;
    event2.raw = "msg";
    event2.tags["span_id"] = "span456";
    extractor.extract(event2);
    REQUIRE(event2.tags["trace_id"] == "span456");
}

TEST_CASE("CorrelationExtractor extracts UUIDs", "[correlation_extractor]") {
    CorrelationExtractor extractor;
    Event event;
    event.raw = "Request ID: 550e8400-e29b-41d4-a716-446655440000 processing";
    
    extractor.extract(event);
    
    REQUIRE(event.tags["uuid"] == "550e8400-e29b-41d4-a716-446655440000");
}

TEST_CASE("CorrelationExtractor handles multiple UUIDs", "[correlation_extractor]") {
    CorrelationExtractor extractor;
    Event event;
    event.raw = "UUID1: 123e4567-e89b-12d3-a456-426614174000 UUID2: 987fcdeb-51a2-43f7-8765-210fedcba321";
    
    extractor.extract(event);
    
    // Should extract the first UUID
    REQUIRE(event.tags.find("uuid") != event.tags.end());
    REQUIRE(event.tags["uuid"] == "123e4567-e89b-12d3-a456-426614174000");
}

TEST_CASE("CorrelationExtractor doesn't overwrite existing normalized IDs", "[correlation_extractor]") {
    CorrelationExtractor extractor;
    Event event;
    event.raw = "message";
    event.tags["request_id"] = "already-set";
    event.tags["requestId"] = "variant";
    
    extractor.extract(event);
    
    // Should keep the normalized version
    REQUIRE(event.tags["request_id"] == "already-set");
}

// Event Index Tests

TEST_CASE("EventIndex builds from events", "[event_index]") {
    EventIndex index;
    std::vector<Event> events;
    
    Event e1;
    e1.id = 1;
    e1.sev = Severity::ERROR;
    events.push_back(e1);
    
    Event e2;
    e2.id = 2;
    e2.sev = Severity::WARN;
    events.push_back(e2);
    
    index.build(events);
    
    REQUIRE(index.get_all_events().size() == 2);
}

TEST_CASE("EventIndex queries by severity", "[event_index]") {
    EventIndex index;
    std::vector<Event> events;
    
    Event e1;
    e1.id = 1;
    e1.sev = Severity::ERROR;
    events.push_back(e1);
    
    Event e2;
    e2.id = 2;
    e2.sev = Severity::ERROR;
    events.push_back(e2);
    
    Event e3;
    e3.id = 3;
    e3.sev = Severity::WARN;
    events.push_back(e3);
    
    index.build(events);
    
    auto errors = index.get_by_severity(Severity::ERROR);
    REQUIRE(errors.size() == 2);
    REQUIRE(errors[0] == 1);
    REQUIRE(errors[1] == 2);
    
    auto warnings = index.get_by_severity(Severity::WARN);
    REQUIRE(warnings.size() == 1);
    REQUIRE(warnings[0] == 3);
}

TEST_CASE("EventIndex counts by severity", "[event_index]") {
    EventIndex index;
    std::vector<Event> events;
    
    for (int i = 0; i < 5; i++) {
        Event e;
        e.id = i + 1;
        e.sev = Severity::ERROR;
        events.push_back(e);
    }
    
    for (int i = 0; i < 3; i++) {
        Event e;
        e.id = i + 6;
        e.sev = Severity::WARN;
        events.push_back(e);
    }
    
    index.build(events);
    
    REQUIRE(index.count_by_severity(Severity::ERROR) == 5);
    REQUIRE(index.count_by_severity(Severity::WARN) == 3);
    REQUIRE(index.count_by_severity(Severity::INFO) == 0);
}

TEST_CASE("EventIndex queries by correlation ID", "[event_index]") {
    EventIndex index;
    std::vector<Event> events;
    
    Event e1;
    e1.id = 1;
    e1.tags["request_id"] = "req-123";
    events.push_back(e1);
    
    Event e2;
    e2.id = 2;
    e2.tags["request_id"] = "req-123";
    events.push_back(e2);
    
    Event e3;
    e3.id = 3;
    e3.tags["request_id"] = "req-456";
    events.push_back(e3);
    
    index.build(events);
    
    auto correlated = index.get_by_correlation_id("req-123");
    REQUIRE(correlated.size() == 2);
    REQUIRE(correlated[0] == 1);
    REQUIRE(correlated[1] == 2);
}

TEST_CASE("EventIndex handles multiple correlation types", "[event_index]") {
    EventIndex index;
    std::vector<Event> events;
    
    Event e1;
    e1.id = 1;
    e1.tags["request_id"] = "req-123";
    e1.tags["trace_id"] = "trace-abc";
    events.push_back(e1);
    
    index.build(events);
    
    auto by_req = index.get_by_correlation_id("req-123");
    auto by_trace = index.get_by_correlation_id("trace-abc");
    
    REQUIRE(by_req.size() == 1);
    REQUIRE(by_trace.size() == 1);
    REQUIRE(by_req[0] == 1);
    REQUIRE(by_trace[0] == 1);
}

TEST_CASE("EventIndex queries by time range", "[event_index]") {
    EventIndex index;
    std::vector<Event> events;
    
    auto now = std::chrono::system_clock::now();
    auto hour_ago = now - std::chrono::hours(1);
    auto two_hours_ago = now - std::chrono::hours(2);
    
    Event e1;
    e1.id = 1;
    e1.ts = Timestamp(two_hours_ago, 100, false);
    events.push_back(e1);
    
    Event e2;
    e2.id = 2;
    e2.ts = Timestamp(hour_ago, 100, false);
    events.push_back(e2);
    
    Event e3;
    e3.id = 3;
    e3.ts = Timestamp(now, 100, false);
    events.push_back(e3);
    
    index.build(events);
    
    // Query for events in the last 90 minutes
    auto recent = index.get_by_time_range(now - std::chrono::minutes(90), now);
    
    REQUIRE(recent.size() >= 1);  // Should include at least the most recent
}

TEST_CASE("EventIndex handles events without timestamps", "[event_index]") {
    EventIndex index;
    std::vector<Event> events;
    
    Event e1;
    e1.id = 1;
    e1.sev = Severity::ERROR;
    // No timestamp
    events.push_back(e1);
    
    index.build(events);
    
    // Should still be queryable by severity
    auto errors = index.get_by_severity(Severity::ERROR);
    REQUIRE(errors.size() == 1);
}

TEST_CASE("EventIndex returns empty for non-existent queries", "[event_index]") {
    EventIndex index;
    std::vector<Event> events;
    
    Event e;
    e.id = 1;
    e.sev = Severity::INFO;
    events.push_back(e);
    
    index.build(events);
    
    REQUIRE(index.get_by_severity(Severity::FATAL).empty());
    REQUIRE(index.get_by_correlation_id("nonexistent").empty());
}
