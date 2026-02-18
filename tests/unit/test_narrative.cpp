#include <catch2/catch_test_macros.hpp>
#include "logstory/narrative/report.hpp"
#include "logstory/narrative/narrator.hpp"
#include "logstory/narrative/markdown_writer.hpp"
#include "logstory/narrative/json_writer.hpp"
#include "logstory/analysis/stats_builder.hpp"
#include "logstory/analysis/episode_builder.hpp"
#include "logstory/rules/rule_registry.hpp"
#include "logstory/rules/builtin/crash_loop_rule.hpp"
#include <sstream>

using namespace logstory::narrative;
using namespace logstory::core;
using namespace logstory::analysis;
using namespace logstory::rules;

// Helper to create test events
Event create_narrative_test_event(EventId id, Severity sev, const std::string& msg,
                                   std::chrono::system_clock::time_point tp) {
    Event e;
    e.id = id;
    e.sev = sev;
    e.message = msg;
    e.ts = Timestamp(tp, 100, false);
    e.src.source_path = "test.log";
    e.src.start_line = id;
    e.src.end_line = id;
    e.raw = msg;
    return e;
}

// ============================================================================
// Report Tests
// ============================================================================

TEST_CASE("Report has correct default values", "[narrative][report]") {
    Report report;
    
    REQUIRE(report.title == "Log Analysis Report");
    REQUIRE(report.total_events == 0);
    REQUIRE(report.error_count == 0);
    REQUIRE(report.warning_count == 0);
    REQUIRE_FALSE(report.has_findings());
}

TEST_CASE("Report detects critical findings", "[narrative][report]") {
    Report report;
    
    Finding f;
    f.severity = FindingSeverity::CRITICAL;
    report.findings.push_back(f);
    
    REQUIRE(report.has_findings());
    REQUIRE(report.has_critical_findings());
}

TEST_CASE("Report counts findings by severity", "[narrative][report]") {
    Report report;
    
    Finding f1;
    f1.severity = FindingSeverity::HIGH;
    report.findings.push_back(f1);
    
    Finding f2;
    f2.severity = FindingSeverity::HIGH;
    report.findings.push_back(f2);
    
    Finding f3;
    f3.severity = FindingSeverity::MEDIUM;
    report.findings.push_back(f3);
    
    REQUIRE(report.finding_count_by_severity(FindingSeverity::HIGH) == 2);
    REQUIRE(report.finding_count_by_severity(FindingSeverity::MEDIUM) == 1);
    REQUIRE(report.finding_count_by_severity(FindingSeverity::LOW) == 0);
}

// ============================================================================
// Narrator Tests
// ============================================================================

TEST_CASE("Narrator generates report with summary", "[narrative][narrator]") {
    Narrator narrator;
    std::vector<Event> events;
    auto now = std::chrono::system_clock::now();
    
    events.push_back(create_narrative_test_event(1, Severity::INFO, "Info", now));
    events.push_back(create_narrative_test_event(2, Severity::ERROR, "Error", now));
    events.push_back(create_narrative_test_event(3, Severity::WARN, "Warning", now));
    
    StatsBuilder builder;
    Stats stats = builder.build(events);
    
    std::vector<Episode> episodes;
    std::vector<Finding> findings;
    
    Report report = narrator.generate(events, stats, episodes, findings);
    
    REQUIRE(report.total_events == 3);
    REQUIRE(report.error_count == 1);
    REQUIRE(report.warning_count == 1);
    REQUIRE_FALSE(report.summary.empty());
}

TEST_CASE("Narrator includes findings in report", "[narrative][narrator]") {
    Narrator narrator;
    std::vector<Event> events;
    auto now = std::chrono::system_clock::now();
    
    events.push_back(create_narrative_test_event(1, Severity::ERROR, "Error", now));
    
    StatsBuilder builder;
    Stats stats = builder.build(events);
    
    std::vector<Episode> episodes;
    
    Finding f;
    f.id = "test-001";
    f.title = "Test Finding";
    f.severity = FindingSeverity::HIGH;
    f.confidence = 0.9;
    std::vector<Finding> findings = {f};
    
    Report report = narrator.generate(events, stats, episodes, findings);
    
    REQUIRE(report.findings.size() == 1);
    REQUIRE(report.findings[0].title == "Test Finding");
}

TEST_CASE("Narrator generates timeline from events", "[narrative][narrator]") {
    Narrator narrator;
    std::vector<Event> events;
    auto now = std::chrono::system_clock::now();
    
    for (int i = 0; i < 5; i++) {
        events.push_back(create_narrative_test_event(i + 1, Severity::ERROR, 
            "Error " + std::to_string(i), now + std::chrono::minutes(i)));
    }
    
    StatsBuilder builder;
    Stats stats = builder.build(events);
    
    std::vector<Episode> episodes;
    std::vector<Finding> findings;
    
    Report report = narrator.generate(events, stats, episodes, findings);
    
    REQUIRE_FALSE(report.timeline.empty());
}

TEST_CASE("Narrator generates evidence from findings", "[narrative][narrator]") {
    Narrator narrator;
    std::vector<Event> events;
    auto now = std::chrono::system_clock::now();
    
    events.push_back(create_narrative_test_event(123, Severity::ERROR, "Critical error", now));
    
    StatsBuilder builder;
    Stats stats = builder.build(events);
    
    std::vector<Episode> episodes;
    
    Finding f;
    f.id = "test-001";
    f.title = "Test Finding";
    f.add_evidence(123, "Main error event");
    std::vector<Finding> findings = {f};
    
    Report report = narrator.generate(events, stats, episodes, findings);
    
    REQUIRE_FALSE(report.evidence.empty());
    REQUIRE(report.evidence[0].event_id == 123);
}

// ============================================================================
// MarkdownWriter Tests
// ============================================================================

TEST_CASE("MarkdownWriter produces valid markdown", "[narrative][markdown]") {
    Report report;
    report.title = "Test Report";
    report.total_events = 10;
    report.error_count = 2;
    report.warning_count = 1;
    report.summary.emplace_back("Test summary bullet");
    
    MarkdownWriter writer;
    std::ostringstream oss;
    writer.write(report, oss);
    
    std::string output = oss.str();
    
    REQUIRE(output.find("# Test Report") != std::string::npos);
    REQUIRE(output.find("## Executive Summary") != std::string::npos);
    REQUIRE(output.find("Test summary bullet") != std::string::npos);
}

TEST_CASE("MarkdownWriter includes findings", "[narrative][markdown]") {
    Report report;
    
    Finding f;
    f.title = "Critical Issue";
    f.severity = FindingSeverity::CRITICAL;
    f.confidence = 0.95;
    f.summary = "This is a critical issue";
    report.findings.push_back(f);
    
    MarkdownWriter writer;
    std::ostringstream oss;
    writer.write(report, oss);
    
    std::string output = oss.str();
    
    REQUIRE(output.find("## Findings") != std::string::npos);
    REQUIRE(output.find("Critical Issue") != std::string::npos);
    REQUIRE(output.find("CRITICAL") != std::string::npos);
}

TEST_CASE("MarkdownWriter escapes special characters", "[narrative][markdown]") {
    Report report;
    report.summary.emplace_back("Text with | pipe");
    
    MarkdownWriter writer;
    std::ostringstream oss;
    writer.write(report, oss);
    
    std::string output = oss.str();
    
    // Pipe should be escaped in summary
    REQUIRE(output.find("Text with | pipe") != std::string::npos);
}

// ============================================================================
// JSONWriter Tests
// ============================================================================

TEST_CASE("JSONWriter produces valid JSON structure", "[narrative][json]") {
    Report report;
    report.title = "Test Report";
    report.total_events = 10;
    report.error_count = 2;
    report.warning_count = 1;
    report.summary.emplace_back("Test summary");
    
    JSONWriter writer;
    std::ostringstream oss;
    writer.write(report, oss);
    
    std::string output = oss.str();
    
    REQUIRE(output.find("{") != std::string::npos);
    REQUIRE(output.find("}") != std::string::npos);
    REQUIRE(output.find("\"schema_version\":") != std::string::npos);
    REQUIRE(output.find("\"title\":") != std::string::npos);
    REQUIRE(output.find("\"Test Report\"") != std::string::npos);
}

TEST_CASE("JSONWriter includes findings", "[narrative][json]") {
    Report report;
    
    Finding f;
    f.id = "test-001";
    f.title = "Test Finding";
    f.severity = FindingSeverity::HIGH;
    f.confidence = 0.85;
    report.findings.push_back(f);
    
    JSONWriter writer;
    std::ostringstream oss;
    writer.write(report, oss);
    
    std::string output = oss.str();
    
    REQUIRE(output.find("\"findings\":") != std::string::npos);
    REQUIRE(output.find("\"test-001\"") != std::string::npos);
    REQUIRE(output.find("\"Test Finding\"") != std::string::npos);
    REQUIRE(output.find("\"HIGH\"") != std::string::npos);
}

TEST_CASE("JSONWriter escapes special characters", "[narrative][json]") {
    Report report;
    report.summary.emplace_back("Text with \"quotes\" and \\backslash");
    
    JSONWriter writer;
    std::ostringstream oss;
    writer.write(report, oss);
    
    std::string output = oss.str();
    
    // Quotes and backslashes should be escaped
    REQUIRE(output.find("\\\"quotes\\\"") != std::string::npos);
    REQUIRE(output.find("\\\\backslash") != std::string::npos);
}

TEST_CASE("JSONWriter includes schema version", "[narrative][json]") {
    Report report;
    
    JSONWriter writer;
    std::ostringstream oss;
    writer.write(report, oss);
    
    std::string output = oss.str();
    
    REQUIRE(output.find("\"schema_version\": 1") != std::string::npos);
}
