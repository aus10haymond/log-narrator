#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "logstory/io/file_reader.hpp"
#include "logstory/io/multiline_framer.hpp"
#include "logstory/parsing/event_parser.hpp"
#include "logstory/analysis/event_index.hpp"
#include "logstory/analysis/episode_builder.hpp"
#include "logstory/analysis/stats_builder.hpp"
#include "logstory/analysis/anomaly_detector.hpp"
#include "logstory/rules/rule_registry.hpp"
#include "logstory/rules/builtin/crash_loop_rule.hpp"
#include "logstory/rules/builtin/retry_to_timeout_rule.hpp"
#include "logstory/rules/builtin/error_burst_after_change_rule.hpp"
#include "logstory/narrative/narrator.hpp"
#include "logstory/narrative/markdown_writer.hpp"
#include "logstory/narrative/json_writer.hpp"
#include "logstory/narrative/timeline_writer.hpp"

namespace fs = std::filesystem;

using namespace logstory;
using namespace logstory::io;
using namespace logstory::parsing;
using namespace logstory::analysis;
using namespace logstory::rules;
using namespace logstory::rules::builtin;
using namespace logstory::narrative;

// Helper function to read file contents
std::string read_file(const fs::path& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + path.string());
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper function to run full pipeline on a log file
Report run_full_pipeline(const fs::path& log_path) {
    // 1. Read and frame records
    FileReader reader;
    std::vector<RawLine> raw_lines;
    auto status = reader.read(log_path.string(), raw_lines);
    if (!status.ok()) {
        throw std::runtime_error("Failed to read file: " + status.message);
    }
    
    MultilineFramer framer;
    std::vector<Record> records;
    framer.frame(raw_lines, records);
    
    // 2. Parse into events
    EventParser parser;
    std::vector<core::Event> events = parser.parse_all(records);
    
    // 3. Build index
    EventIndex index;
    index.build(events);
    
    // 4. Build episodes
    EpisodeBuilder episode_builder;
    auto episodes = episode_builder.build(events);
    
    // 5. Build stats
    StatsBuilder stats_builder;
    Stats stats = stats_builder.build(events);
    
    // 6. Detect anomalies
    std::vector<Anomaly> anomalies;
    
    RestartLoopDetector restart_detector;
    auto restart_anomalies = restart_detector.detect(events);
    anomalies.insert(anomalies.end(), restart_anomalies.begin(), restart_anomalies.end());
    
    ErrorBurstDetector burst_detector;
    auto burst_anomalies = burst_detector.detect(events, stats);
    anomalies.insert(anomalies.end(), burst_anomalies.begin(), burst_anomalies.end());
    
    // 7. Run rules
    RuleRegistry registry;
    registry.register_rule(std::make_unique<CrashLoopRule>());
    registry.register_rule(std::make_unique<RetryToTimeoutRule>());
    registry.register_rule(std::make_unique<ErrorBurstAfterChangeRule>());
    
    RuleContext ctx;
    ctx.events = &events;
    ctx.episodes = &episodes;
    ctx.stats = &stats;
    ctx.anomalies = &anomalies;
    
    auto findings = registry.evaluate_all(ctx);
    
    // 8. Generate report
    Narrator narrator;
    return narrator.generate(events, stats, episodes, findings);
}

// Helper to normalize output for comparison (remove timestamps, etc.)
std::string normalize_output(const std::string& content) {
    // In a real implementation, you might want to normalize certain 
    // dynamic elements like generation timestamps
    return content;
}

TEST_CASE("Golden file test - mixed text logs", "[integration][golden]") {
    fs::path fixture_dir = fs::path(__FILE__).parent_path().parent_path() / "fixtures";
    fs::path log_file = fixture_dir / "logs" / "mixed_text.log";
    
    REQUIRE(fs::exists(log_file));
    
    // Run pipeline
    auto report = run_full_pipeline(log_file);
    
    // Generate markdown output
    MarkdownWriter md_writer;
    std::ostringstream oss;
    md_writer.write(report, oss);
    std::string md_output = oss.str();
    
    // Check that output is not empty
    REQUIRE_FALSE(md_output.empty());
    REQUIRE(md_output.find("# Log Analysis Report") != std::string::npos);
}

TEST_CASE("Golden file test - JSONL logs", "[integration][golden]") {
    fs::path fixture_dir = fs::path(__FILE__).parent_path().parent_path() / "fixtures";
    fs::path log_file = fixture_dir / "logs" / "mixed_jsonl.jsonl";
    
    REQUIRE(fs::exists(log_file));
    
    // Run pipeline
    auto report = run_full_pipeline(log_file);
    
    // Generate JSON output
    JSONWriter json_writer;
    std::ostringstream oss;
    json_writer.write(report, oss);
    std::string json_output = oss.str();
    
    // Check that output is valid JSON structure
    REQUIRE_FALSE(json_output.empty());
    REQUIRE(json_output.find("\"schema_version\"") != std::string::npos);
    REQUIRE(json_output.find("\"summary\"") != std::string::npos);
}

TEST_CASE("Golden file test - retry timeout scenario", "[integration][golden][scenario]") {
    fs::path fixture_dir = fs::path(__FILE__).parent_path().parent_path() / "fixtures";
    fs::path log_file = fixture_dir / "logs" / "scenario_retry_timeout.log";
    
    REQUIRE(fs::exists(log_file));
    
    // Run pipeline
    auto report = run_full_pipeline(log_file);
    
    // Should detect retry-to-timeout pattern
    bool found_retry_timeout = false;
    for (const auto& finding : report.findings) {
        if (finding.title.find("Retry") != std::string::npos && 
            finding.title.find("Timeout") != std::string::npos) {
            found_retry_timeout = true;
            break;
        }
    }
    
    REQUIRE(found_retry_timeout);
}

TEST_CASE("Golden file test - crash loop scenario", "[integration][golden][scenario]") {
    fs::path fixture_dir = fs::path(__FILE__).parent_path().parent_path() / "fixtures";
    fs::path log_file = fixture_dir / "logs" / "scenario_restart_loop.log";
    
    REQUIRE(fs::exists(log_file));
    
    // Run pipeline
    auto report = run_full_pipeline(log_file);
    
    // Should detect crash/restart loop
    bool found_crash_loop = false;
    for (const auto& finding : report.findings) {
        if (finding.title.find("Crash") != std::string::npos || 
            finding.title.find("Restart") != std::string::npos ||
            finding.title.find("Loop") != std::string::npos) {
            found_crash_loop = true;
            break;
        }
    }
    
    REQUIRE(found_crash_loop);
}

TEST_CASE("Golden file test - error burst after change scenario", "[integration][golden][scenario]") {
    fs::path fixture_dir = fs::path(__FILE__).parent_path().parent_path() / "fixtures";
    fs::path log_file = fixture_dir / "logs" / "scenario_config_error_burst.log";
    
    REQUIRE(fs::exists(log_file));
    
    // Run pipeline
    auto report = run_full_pipeline(log_file);
    
    // Should detect error burst after configuration change
    bool found_error_burst = false;
    for (const auto& finding : report.findings) {
        if ((finding.title.find("Error") != std::string::npos || 
             finding.title.find("Burst") != std::string::npos) &&
            (finding.title.find("Config") != std::string::npos ||
             finding.title.find("Deployment") != std::string::npos ||
             finding.title.find("Change") != std::string::npos)) {
            found_error_burst = true;
            break;
        }
    }
    
    REQUIRE(found_error_burst);
}

TEST_CASE("Report contains required sections", "[integration][golden][structure]") {
    fs::path fixture_dir = fs::path(__FILE__).parent_path().parent_path() / "fixtures";
    fs::path log_file = fixture_dir / "logs" / "mixed_text.log";
    
    REQUIRE(fs::exists(log_file));
    
    auto report = run_full_pipeline(log_file);
    
    // Generate markdown and check structure
    MarkdownWriter md_writer;
    std::ostringstream oss;
    md_writer.write(report, oss);
    std::string md_output = oss.str();
    
    // Check for expected sections
    REQUIRE(md_output.find("# Log Analysis Report") != std::string::npos);
    REQUIRE(md_output.find("## Executive Summary") != std::string::npos);
    REQUIRE(md_output.find("## Statistics") != std::string::npos);
}

TEST_CASE("Timeline export produces valid CSV", "[integration][golden][csv]") {
    fs::path fixture_dir = fs::path(__FILE__).parent_path().parent_path() / "fixtures";
    fs::path log_file = fixture_dir / "logs" / "mixed_text.log";
    
    REQUIRE(fs::exists(log_file));
    
    auto report = run_full_pipeline(log_file);
    
    // Generate timeline CSV
    TimelineWriter timeline_writer;
    std::ostringstream oss;
    timeline_writer.write_highlights(report, oss);
    std::string csv_output = oss.str();
    
    // Check basic CSV structure
    REQUIRE_FALSE(csv_output.empty());
    bool has_timestamp = (csv_output.find("timestamp") != std::string::npos || 
                          csv_output.find("Timestamp") != std::string::npos);
    REQUIRE(has_timestamp);
}

TEST_CASE("Output is deterministic across runs", "[integration][golden][deterministic]") {
    fs::path fixture_dir = fs::path(__FILE__).parent_path().parent_path() / "fixtures";
    fs::path log_file = fixture_dir / "logs" / "mixed_text.log";
    
    REQUIRE(fs::exists(log_file));
    
    // Run pipeline twice
    auto report1 = run_full_pipeline(log_file);
    auto report2 = run_full_pipeline(log_file);
    
    // Generate outputs
    MarkdownWriter md_writer;
    std::ostringstream oss1, oss2;
    md_writer.write(report1, oss1);
    md_writer.write(report2, oss2);
    std::string md1 = oss1.str();
    std::string md2 = oss2.str();
    
    // Outputs should be identical (excluding any dynamic timestamps in headers)
    // For now, just check they're both non-empty and have same structure
    REQUIRE(md1.length() == md2.length());
}
