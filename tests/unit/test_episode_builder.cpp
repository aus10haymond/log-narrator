#include <catch2/catch_test_macros.hpp>
#include "logstory/analysis/episode_builder.hpp"

using namespace logstory::analysis;
using namespace logstory::core;

// Helper to create events with timestamps
Event create_event(EventId id, Severity sev, std::chrono::system_clock::time_point tp) {
    Event e;
    e.id = id;
    e.sev = sev;
    e.ts = Timestamp(tp, 100, false);
    e.raw = "Event " + std::to_string(id);
    return e;
}

TEST_CASE("EpisodeBuilder creates single episode from sequential events", "[episode_builder]") {
    EpisodeBuilder builder;
    std::vector<Event> events;
    
    auto now = std::chrono::system_clock::now();
    events.push_back(create_event(1, Severity::INFO, now));
    events.push_back(create_event(2, Severity::INFO, now + std::chrono::seconds(10)));
    events.push_back(create_event(3, Severity::INFO, now + std::chrono::seconds(20)));
    
    auto episodes = builder.build(events);
    
    REQUIRE(episodes.size() == 1);
    REQUIRE(episodes[0].event_ids.size() == 3);
}

TEST_CASE("EpisodeBuilder splits on time gaps", "[episode_builder]") {
    EpisodeConfig config;
    config.time_gap_threshold = std::chrono::minutes(5);
    EpisodeBuilder builder(config);
    
    std::vector<Event> events;
    auto now = std::chrono::system_clock::now();
    
    // Episode 1: events close together
    events.push_back(create_event(1, Severity::INFO, now));
    events.push_back(create_event(2, Severity::INFO, now + std::chrono::minutes(1)));
    
    // Large gap
    
    // Episode 2: events after gap
    events.push_back(create_event(3, Severity::INFO, now + std::chrono::minutes(10)));
    events.push_back(create_event(4, Severity::INFO, now + std::chrono::minutes(11)));
    
    auto episodes = builder.build(events);
    
    REQUIRE(episodes.size() == 2);
    REQUIRE(episodes[0].event_ids.size() == 2);
    REQUIRE(episodes[1].event_ids.size() == 2);
}

TEST_CASE("EpisodeBuilder handles events without timestamps", "[episode_builder]") {
    EpisodeBuilder builder;
    std::vector<Event> events;
    
    Event e1;
    e1.id = 1;
    e1.sev = Severity::INFO;
    // No timestamp
    events.push_back(e1);
    
    Event e2;
    e2.id = 2;
    e2.sev = Severity::WARN;
    events.push_back(e2);
    
    auto episodes = builder.build(events);
    
    // Should create one episode (no time gap without timestamps)
    REQUIRE(episodes.size() == 1);
    REQUIRE(episodes[0].event_ids.size() == 2);
}

TEST_CASE("EpisodeBuilder merges episodes with shared correlation IDs", "[episode_builder]") {
    EpisodeConfig config;
    config.time_gap_threshold = std::chrono::minutes(1);
    config.merge_by_correlation = true;
    EpisodeBuilder builder(config);
    
    std::vector<Event> events;
    auto now = std::chrono::system_clock::now();
    
    // Episode 1
    Event e1 = create_event(1, Severity::INFO, now);
    e1.tags["request_id"] = "req-123";
    events.push_back(e1);
    
    // Gap that would normally split
    
    // Episode 2 with same request_id (should merge)
    Event e2 = create_event(2, Severity::ERROR, now + std::chrono::minutes(5));
    e2.tags["request_id"] = "req-123";
    events.push_back(e2);
    
    auto episodes = builder.build(events);
    
    // Should merge into one episode
    REQUIRE(episodes.size() == 1);
    REQUIRE(episodes[0].event_ids.size() == 2);
}

TEST_CASE("EpisodeBuilder doesn't merge without shared correlation IDs", "[episode_builder]") {
    EpisodeConfig config;
    config.time_gap_threshold = std::chrono::minutes(1);
    config.merge_by_correlation = true;
    EpisodeBuilder builder(config);
    
    std::vector<Event> events;
    auto now = std::chrono::system_clock::now();
    
    Event e1 = create_event(1, Severity::INFO, now);
    e1.tags["request_id"] = "req-123";
    events.push_back(e1);
    
    Event e2 = create_event(2, Severity::ERROR, now + std::chrono::minutes(5));
    e2.tags["request_id"] = "req-456";  // Different request
    events.push_back(e2);
    
    auto episodes = builder.build(events);
    
    // Should NOT merge (different correlation IDs)
    REQUIRE(episodes.size() == 2);
}

TEST_CASE("EpisodeBuilder can disable correlation merging", "[episode_builder]") {
    EpisodeConfig config;
    config.time_gap_threshold = std::chrono::minutes(1);
    config.merge_by_correlation = false;
    EpisodeBuilder builder(config);
    
    std::vector<Event> events;
    auto now = std::chrono::system_clock::now();
    
    Event e1 = create_event(1, Severity::INFO, now);
    e1.tags["request_id"] = "req-123";
    events.push_back(e1);
    
    Event e2 = create_event(2, Severity::ERROR, now + std::chrono::minutes(5));
    e2.tags["request_id"] = "req-123";
    events.push_back(e2);
    
    auto episodes = builder.build(events);
    
    // Should NOT merge (merging disabled)
    REQUIRE(episodes.size() == 2);
}

TEST_CASE("EpisodeBuilder tracks start and end times", "[episode_builder]") {
    EpisodeBuilder builder;
    std::vector<Event> events;
    
    auto start = std::chrono::system_clock::now();
    auto end = start + std::chrono::minutes(10);
    
    events.push_back(create_event(1, Severity::INFO, start));
    events.push_back(create_event(2, Severity::INFO, start + std::chrono::minutes(5)));
    events.push_back(create_event(3, Severity::INFO, end));
    
    auto episodes = builder.build(events);
    
    REQUIRE(episodes.size() == 1);
    REQUIRE(episodes[0].start_time.has_value());
    REQUIRE(episodes[0].end_time.has_value());
    REQUIRE(*episodes[0].start_time == start);
    REQUIRE(*episodes[0].end_time == end);
}

TEST_CASE("EpisodeBuilder identifies first error as highlight", "[episode_builder]") {
    EpisodeBuilder builder;
    std::vector<Event> events;
    
    auto now = std::chrono::system_clock::now();
    events.push_back(create_event(1, Severity::INFO, now));
    events.push_back(create_event(2, Severity::ERROR, now + std::chrono::seconds(10)));
    events.push_back(create_event(3, Severity::INFO, now + std::chrono::seconds(20)));
    
    auto episodes = builder.build(events);
    
    REQUIRE(episodes.size() == 1);
    REQUIRE_FALSE(episodes[0].highlights.empty());
    REQUIRE(episodes[0].highlights[0] == 2);  // First error
}

TEST_CASE("EpisodeBuilder tracks max severity", "[episode_builder]") {
    EpisodeBuilder builder;
    std::vector<Event> events;
    
    auto now = std::chrono::system_clock::now();
    events.push_back(create_event(1, Severity::INFO, now));
    events.push_back(create_event(2, Severity::WARN, now + std::chrono::seconds(10)));
    events.push_back(create_event(3, Severity::ERROR, now + std::chrono::seconds(20)));
    events.push_back(create_event(4, Severity::INFO, now + std::chrono::seconds(30)));
    
    auto episodes = builder.build(events);
    
    REQUIRE(episodes.size() == 1);
    REQUIRE(episodes[0].max_severity == Severity::ERROR);
}

TEST_CASE("EpisodeBuilder collects correlation IDs", "[episode_builder]") {
    EpisodeBuilder builder;
    std::vector<Event> events;
    
    auto now = std::chrono::system_clock::now();
    
    Event e1 = create_event(1, Severity::INFO, now);
    e1.tags["request_id"] = "req-123";
    events.push_back(e1);
    
    Event e2 = create_event(2, Severity::INFO, now + std::chrono::seconds(10));
    e2.tags["trace_id"] = "trace-abc";
    events.push_back(e2);
    
    auto episodes = builder.build(events);
    
    REQUIRE(episodes.size() == 1);
    REQUIRE(episodes[0].correlation_ids.size() == 2);
}

TEST_CASE("EpisodeBuilder handles empty input", "[episode_builder]") {
    EpisodeBuilder builder;
    std::vector<Event> events;
    
    auto episodes = builder.build(events);
    
    REQUIRE(episodes.empty());
}

TEST_CASE("EpisodeBuilder handles single event", "[episode_builder]") {
    EpisodeBuilder builder;
    std::vector<Event> events;
    
    auto now = std::chrono::system_clock::now();
    events.push_back(create_event(1, Severity::INFO, now));
    
    auto episodes = builder.build(events);
    
    REQUIRE(episodes.size() == 1);
    REQUIRE(episodes[0].event_ids.size() == 1);
    REQUIRE(episodes[0].event_ids[0] == 1);
}

TEST_CASE("EpisodeBuilder assigns unique episode IDs", "[episode_builder]") {
    EpisodeBuilder builder;
    std::vector<Event> events;
    
    auto now = std::chrono::system_clock::now();
    events.push_back(create_event(1, Severity::INFO, now));
    events.push_back(create_event(2, Severity::INFO, now + std::chrono::minutes(10)));
    
    auto episodes = builder.build(events);
    
    REQUIRE(episodes.size() == 2);
    REQUIRE(episodes[0].id != 0);
    REQUIRE(episodes[1].id != 0);
    REQUIRE(episodes[0].id != episodes[1].id);
}

TEST_CASE("Episode struct size and empty methods work", "[episode]") {
    Episode ep;
    
    REQUIRE(ep.empty());
    REQUIRE(ep.size() == 0);
    
    ep.event_ids.push_back(1);
    ep.event_ids.push_back(2);
    
    REQUIRE_FALSE(ep.empty());
    REQUIRE(ep.size() == 2);
}
