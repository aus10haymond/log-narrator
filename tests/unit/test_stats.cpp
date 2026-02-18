#include <catch2/catch_test_macros.hpp>
#include "logstory/analysis/stats.hpp"
#include "logstory/analysis/stats_builder.hpp"
#include "logstory/analysis/anomaly_detector.hpp"

using namespace logstory::analysis;
using namespace logstory::core;

// Helper to create test events
Event create_test_event(EventId id, Severity sev, const std::string& msg,
                        std::chrono::system_clock::time_point tp) {
    Event e;
    e.id = id;
    e.sev = sev;
    e.message = msg;
    e.ts = Timestamp(tp, 100, false);
    e.src.source_path = "test.log";
    e.raw = msg;
    return e;
}

// ============================================================================
// TimeSeries Tests
// ============================================================================

TEST_CASE("TimeSeries adds events to correct buckets", "[stats][timeseries]") {
    TimeSeries ts(std::chrono::minutes(5));
    
    auto now = std::chrono::system_clock::now();
    ts.add_event(now);
    ts.add_event(now + std::chrono::seconds(30));
    ts.add_event(now + std::chrono::minutes(6));
    
    REQUIRE(ts.points.size() == 2);
    REQUIRE(ts.total_count() == 3);
}

TEST_CASE("TimeSeries finds max point", "[stats][timeseries]") {
    TimeSeries ts(std::chrono::minutes(1));
    
    auto now = std::chrono::system_clock::now();
    ts.add_event(now);
    ts.add_event(now + std::chrono::minutes(1));
    ts.add_event(now + std::chrono::minutes(1));
    ts.add_event(now + std::chrono::minutes(1));
    
    auto max = ts.max_point();
    REQUIRE(max.has_value());
    REQUIRE(max->count == 3);
}

TEST_CASE("TimeSeries handles empty series", "[stats][timeseries]") {
    TimeSeries ts;
    
    REQUIRE(ts.total_count() == 0);
    REQUIRE_FALSE(ts.max_point().has_value());
}

// ============================================================================
// Stats Tests
// ============================================================================

TEST_CASE("Stats tracks error and warn counts", "[stats]") {
    Stats stats;
    stats.severity_counts[Severity::ERROR] = 10;
    stats.severity_counts[Severity::WARN] = 5;
    stats.total_events = 20;
    
    REQUIRE(stats.error_count() == 10);
    REQUIRE(stats.warn_count() == 5);
    REQUIRE(stats.error_rate() == 0.5);
}

TEST_CASE("Stats handles no errors", "[stats]") {
    Stats stats;
    stats.total_events = 10;
    
    REQUIRE(stats.error_count() == 0);
    REQUIRE(stats.warn_count() == 0);
    REQUIRE(stats.error_rate() == 0.0);
}

// ============================================================================
// StatsBuilder Tests
// ============================================================================

TEST_CASE("StatsBuilder builds severity counts", "[stats][builder]") {
    StatsBuilder builder;
    std::vector<Event> events;
    
    auto now = std::chrono::system_clock::now();
    events.push_back(create_test_event(1, Severity::INFO, "Info message", now));
    events.push_back(create_test_event(2, Severity::ERROR, "Error message", now));
    events.push_back(create_test_event(3, Severity::ERROR, "Another error", now));
    events.push_back(create_test_event(4, Severity::WARN, "Warning", now));
    
    Stats stats = builder.build(events);
    
    REQUIRE(stats.total_events == 4);
    REQUIRE(stats.severity_counts[Severity::INFO] == 1);
    REQUIRE(stats.severity_counts[Severity::ERROR] == 2);
    REQUIRE(stats.severity_counts[Severity::WARN] == 1);
}

TEST_CASE("StatsBuilder tracks time boundaries", "[stats][builder]") {
    StatsBuilder builder;
    std::vector<Event> events;
    
    auto start = std::chrono::system_clock::now();
    auto end = start + std::chrono::hours(1);
    
    events.push_back(create_test_event(1, Severity::INFO, "First", start));
    events.push_back(create_test_event(2, Severity::INFO, "Last", end));
    
    Stats stats = builder.build(events);
    
    REQUIRE(stats.start_time.has_value());
    REQUIRE(stats.end_time.has_value());
    REQUIRE(*stats.start_time == start);
    REQUIRE(*stats.end_time == end);
}

TEST_CASE("StatsBuilder builds time series by severity", "[stats][builder]") {
    StatsBuilder builder;
    std::vector<Event> events;
    
    auto now = std::chrono::system_clock::now();
    events.push_back(create_test_event(1, Severity::ERROR, "Error 1", now));
    events.push_back(create_test_event(2, Severity::ERROR, "Error 2", now + std::chrono::seconds(30)));
    events.push_back(create_test_event(3, Severity::INFO, "Info", now + std::chrono::minutes(5)));
    
    Stats stats = builder.build(events);
    
    REQUIRE(stats.severity_time_series.find(Severity::ERROR) != stats.severity_time_series.end());
    REQUIRE(stats.severity_time_series.find(Severity::INFO) != stats.severity_time_series.end());
    REQUIRE(stats.severity_time_series[Severity::ERROR].total_count() == 2);
}

TEST_CASE("StatsBuilder counts by source", "[stats][builder]") {
    StatsBuilder builder;
    std::vector<Event> events;
    
    auto now = std::chrono::system_clock::now();
    
    Event e1 = create_test_event(1, Severity::INFO, "Message 1", now);
    e1.src.source_path = "file1.log";
    events.push_back(e1);
    
    Event e2 = create_test_event(2, Severity::INFO, "Message 2", now);
    e2.src.source_path = "file1.log";
    events.push_back(e2);
    
    Event e3 = create_test_event(3, Severity::INFO, "Message 3", now);
    e3.src.source_path = "file2.log";
    events.push_back(e3);
    
    Stats stats = builder.build(events);
    
    REQUIRE(stats.source_counts["file1.log"] == 2);
    REQUIRE(stats.source_counts["file2.log"] == 1);
}

TEST_CASE("StatsBuilder extracts frequent patterns", "[stats][builder]") {
    StatsConfig config;
    config.top_n_patterns = 2;
    StatsBuilder builder(config);
    
    std::vector<Event> events;
    auto now = std::chrono::system_clock::now();
    
    events.push_back(create_test_event(1, Severity::ERROR, "Connection failed to server 123", now));
    events.push_back(create_test_event(2, Severity::ERROR, "Connection failed to server 456", now));
    events.push_back(create_test_event(3, Severity::ERROR, "Connection failed to server 789", now));
    events.push_back(create_test_event(4, Severity::WARN, "Timeout after 5000ms", now));
    events.push_back(create_test_event(5, Severity::WARN, "Timeout after 3000ms", now));
    
    Stats stats = builder.build(events);
    
    REQUIRE_FALSE(stats.frequent_patterns.empty());
    REQUIRE(stats.frequent_patterns.size() <= 2);
    
    // Most frequent should be connection failures
    if (!stats.frequent_patterns.empty()) {
        REQUIRE(stats.frequent_patterns[0].count >= 2);
    }
}

TEST_CASE("StatsBuilder handles empty events", "[stats][builder]") {
    StatsBuilder builder;
    std::vector<Event> events;
    
    Stats stats = builder.build(events);
    
    REQUIRE(stats.total_events == 0);
    REQUIRE(stats.severity_counts.empty());
}

// ============================================================================
// ErrorBurstDetector Tests
// ============================================================================

TEST_CASE("ErrorBurstDetector detects error spike", "[anomaly][error_burst]") {
    ErrorBurstConfig config;
    config.min_errors_for_burst = 5;
    config.threshold_multiplier = 2.0;
    ErrorBurstDetector detector(config);
    
    std::vector<Event> events;
    auto now = std::chrono::system_clock::now();
    
    // Normal baseline: 1 error per minute for 10 minutes
    for (int i = 0; i < 10; i++) {
        events.push_back(create_test_event(i, Severity::ERROR, "Error", 
            now + std::chrono::minutes(i)));
    }
    
    // Burst: 10 errors in one minute
    for (int i = 10; i < 20; i++) {
        events.push_back(create_test_event(i, Severity::ERROR, "Burst error", 
            now + std::chrono::minutes(15)));
    }
    
    // Build stats
    StatsBuilder builder;
    Stats stats = builder.build(events);
    
    auto anomalies = detector.detect(events, stats);
    
    REQUIRE_FALSE(anomalies.empty());
    REQUIRE(anomalies[0].type == Anomaly::Type::ERROR_BURST);
    REQUIRE(anomalies[0].confidence > 0.0);
}

TEST_CASE("ErrorBurstDetector ignores low error counts", "[anomaly][error_burst]") {
    ErrorBurstConfig config;
    config.min_errors_for_burst = 100;
    ErrorBurstDetector detector(config);
    
    std::vector<Event> events;
    auto now = std::chrono::system_clock::now();
    
    for (int i = 0; i < 10; i++) {
        events.push_back(create_test_event(i, Severity::ERROR, "Error", now));
    }
    
    StatsBuilder builder;
    Stats stats = builder.build(events);
    
    auto anomalies = detector.detect(events, stats);
    
    REQUIRE(anomalies.empty());
}

TEST_CASE("ErrorBurstDetector handles no errors", "[anomaly][error_burst]") {
    ErrorBurstDetector detector;
    
    std::vector<Event> events;
    auto now = std::chrono::system_clock::now();
    
    events.push_back(create_test_event(1, Severity::INFO, "Info", now));
    
    StatsBuilder builder;
    Stats stats = builder.build(events);
    
    auto anomalies = detector.detect(events, stats);
    
    REQUIRE(anomalies.empty());
}

// ============================================================================
// RestartLoopDetector Tests
// ============================================================================

TEST_CASE("RestartLoopDetector detects restart loop", "[anomaly][restart_loop]") {
    RestartLoopConfig config;
    config.min_restart_count = 3;
    config.time_window = std::chrono::minutes(10);
    RestartLoopDetector detector(config);
    
    std::vector<Event> events;
    auto now = std::chrono::system_clock::now();
    
    events.push_back(create_test_event(1, Severity::INFO, "Server starting", now));
    events.push_back(create_test_event(2, Severity::ERROR, "Crash", now + std::chrono::minutes(1)));
    events.push_back(create_test_event(3, Severity::INFO, "Server starting", now + std::chrono::minutes(2)));
    events.push_back(create_test_event(4, Severity::ERROR, "Crash", now + std::chrono::minutes(3)));
    events.push_back(create_test_event(5, Severity::INFO, "Server starting", now + std::chrono::minutes(4)));
    
    auto anomalies = detector.detect(events);
    
    REQUIRE_FALSE(anomalies.empty());
    REQUIRE(anomalies[0].type == Anomaly::Type::RESTART_LOOP);
    REQUIRE(anomalies[0].evidence_ids.size() >= 3);
}

TEST_CASE("RestartLoopDetector ignores isolated restarts", "[anomaly][restart_loop]") {
    RestartLoopConfig config;
    config.min_restart_count = 3;
    config.time_window = std::chrono::minutes(5);
    RestartLoopDetector detector(config);
    
    std::vector<Event> events;
    auto now = std::chrono::system_clock::now();
    
    events.push_back(create_test_event(1, Severity::INFO, "Server starting", now));
    events.push_back(create_test_event(2, Severity::INFO, "Normal operation", now + std::chrono::minutes(10)));
    events.push_back(create_test_event(3, Severity::INFO, "Server starting", now + std::chrono::minutes(20)));
    
    auto anomalies = detector.detect(events);
    
    REQUIRE(anomalies.empty());
}

TEST_CASE("RestartLoopDetector recognizes various restart keywords", "[anomaly][restart_loop]") {
    RestartLoopDetector detector;
    
    std::vector<Event> events;
    auto now = std::chrono::system_clock::now();
    
    events.push_back(create_test_event(1, Severity::INFO, "Application STARTING now", now));
    events.push_back(create_test_event(2, Severity::INFO, "System shutdown initiated", now + std::chrono::minutes(1)));
    events.push_back(create_test_event(3, Severity::INFO, "Initializing components", now + std::chrono::minutes(2)));
    
    auto anomalies = detector.detect(events);
    
    REQUIRE_FALSE(anomalies.empty());
}

TEST_CASE("RestartLoopDetector handles events without timestamps", "[anomaly][restart_loop]") {
    RestartLoopDetector detector;
    
    std::vector<Event> events;
    
    Event e1;
    e1.id = 1;
    e1.message = "Starting";
    e1.sev = Severity::INFO;
    events.push_back(e1);
    
    Event e2;
    e2.id = 2;
    e2.message = "Starting";
    e2.sev = Severity::INFO;
    events.push_back(e2);
    
    auto anomalies = detector.detect(events);
    
    REQUIRE(anomalies.empty());
}
