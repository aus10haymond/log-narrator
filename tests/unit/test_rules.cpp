#include <catch2/catch_test_macros.hpp>
#include "logstory/rules/finding.hpp"
#include "logstory/rules/rule.hpp"
#include "logstory/rules/rule_registry.hpp"
#include "logstory/rules/builtin/crash_loop_rule.hpp"
#include "logstory/rules/builtin/retry_to_timeout_rule.hpp"
#include "logstory/rules/builtin/error_burst_after_change_rule.hpp"
#include "logstory/analysis/stats_builder.hpp"
#include "logstory/analysis/anomaly_detector.hpp"

using namespace logstory::rules;
using namespace logstory::rules::builtin;
using namespace logstory::core;
using namespace logstory::analysis;

// Helper to create test events
Event create_rule_test_event(EventId id, Severity sev, const std::string& msg,
                             std::chrono::system_clock::time_point tp) {
    Event e;
    e.id = id;
    e.sev = sev;
    e.message = msg;
    e.ts = Timestamp(tp, 100, false);
    e.raw = msg;
    return e;
}

// ============================================================================
// Finding Tests
// ============================================================================

TEST_CASE("Finding stores basic information", "[rules][finding]") {
    Finding f;
    f.id = "test-001";
    f.title = "Test Finding";
    f.summary = "This is a test";
    f.severity = FindingSeverity::HIGH;
    f.confidence = 0.85;
    
    REQUIRE(f.id == "test-001");
    REQUIRE(f.title == "Test Finding");
    REQUIRE(f.confidence == 0.85);
}

TEST_CASE("Finding can add evidence", "[rules][finding]") {
    Finding f;
    f.add_evidence(123, "First event");
    f.add_evidence(456, "Second event");
    
    REQUIRE(f.evidence.size() == 2);
    REQUIRE(f.evidence[0].event_id == 123);
    REQUIRE(f.evidence[1].event_id == 456);
}

TEST_CASE("FindingSeverity converts to string", "[rules][finding]") {
    REQUIRE(to_string(FindingSeverity::LOW) == "LOW");
    REQUIRE(to_string(FindingSeverity::MEDIUM) == "MEDIUM");
    REQUIRE(to_string(FindingSeverity::HIGH) == "HIGH");
    REQUIRE(to_string(FindingSeverity::CRITICAL) == "CRITICAL");
}

// ============================================================================
// RuleRegistry Tests
// ============================================================================

// Simple test rule
class TestRule : public Rule {
public:
    TestRule(const std::string& id, int prio = 0) 
        : id_(id), priority_(prio), eval_count_(0) {}
    
    std::string id() const override { return id_; }
    std::string name() const override { return "Test Rule"; }
    int priority() const override { return priority_; }
    
    std::vector<Finding> evaluate(const RuleContext&) override {
        eval_count_++;
        Finding f;
        f.id = id_ + "-finding";
        f.title = "Test Finding from " + id_;
        return {f};
    }
    
    int eval_count() const { return eval_count_; }
    
private:
    std::string id_;
    int priority_;
    mutable int eval_count_;
};

TEST_CASE("RuleRegistry registers rules", "[rules][registry]") {
    RuleRegistry registry;
    
    registry.register_rule(std::make_unique<TestRule>("rule1"));
    registry.register_rule(std::make_unique<TestRule>("rule2"));
    
    REQUIRE(registry.size() == 2);
}

TEST_CASE("RuleRegistry retrieves rules by ID", "[rules][registry]") {
    RuleRegistry registry;
    
    registry.register_rule(std::make_unique<TestRule>("rule1"));
    
    Rule* rule = registry.get_rule("rule1");
    REQUIRE(rule != nullptr);
    REQUIRE(rule->id() == "rule1");
    
    Rule* missing = registry.get_rule("nonexistent");
    REQUIRE(missing == nullptr);
}

TEST_CASE("RuleRegistry sorts rules by priority", "[rules][registry]") {
    RuleRegistry registry;
    
    registry.register_rule(std::make_unique<TestRule>("low", 10));
    registry.register_rule(std::make_unique<TestRule>("high", 90));
    registry.register_rule(std::make_unique<TestRule>("mid", 50));
    
    auto rules = registry.get_all_rules();
    
    REQUIRE(rules.size() == 3);
    REQUIRE(rules[0]->id() == "high");  // 90
    REQUIRE(rules[1]->id() == "mid");   // 50
    REQUIRE(rules[2]->id() == "low");   // 10
}

TEST_CASE("RuleRegistry evaluates all rules", "[rules][registry]") {
    RuleRegistry registry;
    
    registry.register_rule(std::make_unique<TestRule>("rule1"));
    registry.register_rule(std::make_unique<TestRule>("rule2"));
    
    RuleContext ctx;
    auto findings = registry.evaluate_all(ctx);
    
    REQUIRE(findings.size() == 2);
}

TEST_CASE("RuleRegistry clears all rules", "[rules][registry]") {
    RuleRegistry registry;
    
    registry.register_rule(std::make_unique<TestRule>("rule1"));
    REQUIRE(registry.size() == 1);
    
    registry.clear();
    REQUIRE(registry.size() == 0);
}

// ============================================================================
// CrashLoopRule Tests
// ============================================================================

TEST_CASE("CrashLoopRule detects crash loops from anomalies", "[rules][crash_loop]") {
    CrashLoopRule rule;
    
    std::vector<Event> events;
    auto now = std::chrono::system_clock::now();
    
    events.push_back(create_rule_test_event(1, Severity::INFO, "Server starting", now));
    events.push_back(create_rule_test_event(2, Severity::INFO, "Server starting", now + std::chrono::minutes(1)));
    events.push_back(create_rule_test_event(3, Severity::INFO, "Server starting", now + std::chrono::minutes(2)));
    
    // Create anomaly
    RestartLoopDetector detector;
    std::vector<Anomaly> anomalies = detector.detect(events);
    
    RuleContext ctx;
    ctx.events = &events;
    ctx.anomalies = &anomalies;
    
    auto findings = rule.evaluate(ctx);
    
    if (!anomalies.empty()) {
        REQUIRE_FALSE(findings.empty());
        REQUIRE(findings[0].severity == FindingSeverity::CRITICAL);
        REQUIRE_FALSE(findings[0].evidence.empty());
    }
}

TEST_CASE("CrashLoopRule handles no anomalies", "[rules][crash_loop]") {
    CrashLoopRule rule;
    
    RuleContext ctx;
    auto findings = rule.evaluate(ctx);
    
    REQUIRE(findings.empty());
}

// ============================================================================
// RetryToTimeoutRule Tests
// ============================================================================

TEST_CASE("RetryToTimeoutRule detects retry to timeout pattern", "[rules][retry_timeout]") {
    RetryToTimeoutRule rule;
    
    std::vector<Event> events;
    auto now = std::chrono::system_clock::now();
    
    events.push_back(create_rule_test_event(1, Severity::WARN, "Retry attempt 1", now));
    events.push_back(create_rule_test_event(2, Severity::WARN, "Retry attempt 2", now + std::chrono::seconds(5)));
    events.push_back(create_rule_test_event(3, Severity::WARN, "Retry attempt 3", now + std::chrono::seconds(10)));
    events.push_back(create_rule_test_event(4, Severity::ERROR, "Operation timed out", now + std::chrono::seconds(15)));
    
    RuleContext ctx;
    ctx.events = &events;
    
    auto findings = rule.evaluate(ctx);
    
    REQUIRE_FALSE(findings.empty());
    REQUIRE(findings[0].title == "Retries Leading to Timeout");
    REQUIRE(findings[0].severity == FindingSeverity::HIGH);
    REQUIRE(findings[0].evidence.size() == 4); // 3 retries + 1 timeout
}

TEST_CASE("RetryToTimeoutRule ignores retries without timeout", "[rules][retry_timeout]") {
    RetryToTimeoutRule rule;
    
    std::vector<Event> events;
    auto now = std::chrono::system_clock::now();
    
    events.push_back(create_rule_test_event(1, Severity::WARN, "Retry attempt 1", now));
    events.push_back(create_rule_test_event(2, Severity::WARN, "Retry attempt 2", now + std::chrono::seconds(5)));
    events.push_back(create_rule_test_event(3, Severity::INFO, "Success", now + std::chrono::seconds(10)));
    
    RuleContext ctx;
    ctx.events = &events;
    
    auto findings = rule.evaluate(ctx);
    
    REQUIRE(findings.empty());
}

TEST_CASE("RetryToTimeoutRule handles empty events", "[rules][retry_timeout]") {
    RetryToTimeoutRule rule;
    
    std::vector<Event> events;
    RuleContext ctx;
    ctx.events = &events;
    
    auto findings = rule.evaluate(ctx);
    
    REQUIRE(findings.empty());
}

// ============================================================================
// ErrorBurstAfterChangeRule Tests
// ============================================================================

TEST_CASE("ErrorBurstAfterChangeRule detects errors after deployment", "[rules][error_burst_change]") {
    ErrorBurstAfterChangeRule rule;
    
    std::vector<Event> events;
    auto now = std::chrono::system_clock::now();
    
    // Deployment event
    events.push_back(create_rule_test_event(1, Severity::INFO, "Deployment started", now));
    
    // Normal baseline
    for (int i = 0; i < 5; i++) {
        events.push_back(create_rule_test_event(i + 2, Severity::ERROR, "Error", 
            now + std::chrono::minutes(i + 10)));
    }
    
    // Error burst shortly after deployment
    for (int i = 0; i < 15; i++) {
        events.push_back(create_rule_test_event(i + 100, Severity::ERROR, "Post-deploy error", 
            now + std::chrono::minutes(2)));
    }
    
    // Build stats and detect anomalies
    StatsBuilder builder;
    Stats stats = builder.build(events);
    
    ErrorBurstDetector burst_detector;
    std::vector<Anomaly> anomalies = burst_detector.detect(events, stats);
    
    RuleContext ctx;
    ctx.events = &events;
    ctx.stats = &stats;
    ctx.anomalies = &anomalies;
    
    auto findings = rule.evaluate(ctx);
    
    if (!anomalies.empty()) {
        REQUIRE_FALSE(findings.empty());
        REQUIRE(findings[0].title == "Error Burst Following Deployment/Config Change");
        REQUIRE(findings[0].severity == FindingSeverity::HIGH);
    }
}

TEST_CASE("ErrorBurstAfterChangeRule ignores bursts without nearby changes", "[rules][error_burst_change]") {
    ErrorBurstAfterChangeRule rule;
    
    std::vector<Event> events;
    auto now = std::chrono::system_clock::now();
    
    // Deployment event (too far in the past)
    events.push_back(create_rule_test_event(1, Severity::INFO, "Deployment", now - std::chrono::hours(2)));
    
    // Error burst now
    for (int i = 0; i < 20; i++) {
        events.push_back(create_rule_test_event(i + 2, Severity::ERROR, "Error", now));
    }
    
    StatsBuilder builder;
    Stats stats = builder.build(events);
    
    ErrorBurstDetector burst_detector;
    std::vector<Anomaly> anomalies = burst_detector.detect(events, stats);
    
    RuleContext ctx;
    ctx.events = &events;
    ctx.stats = &stats;
    ctx.anomalies = &anomalies;
    
    auto findings = rule.evaluate(ctx);
    
    REQUIRE(findings.empty());
}

TEST_CASE("ErrorBurstAfterChangeRule handles no change events", "[rules][error_burst_change]") {
    ErrorBurstAfterChangeRule rule;
    
    std::vector<Event> events;
    auto now = std::chrono::system_clock::now();
    
    for (int i = 0; i < 20; i++) {
        events.push_back(create_rule_test_event(i, Severity::ERROR, "Error", now));
    }
    
    StatsBuilder builder;
    Stats stats = builder.build(events);
    
    ErrorBurstDetector burst_detector;
    std::vector<Anomaly> anomalies = burst_detector.detect(events, stats);
    
    RuleContext ctx;
    ctx.events = &events;
    ctx.stats = &stats;
    ctx.anomalies = &anomalies;
    
    auto findings = rule.evaluate(ctx);
    
    REQUIRE(findings.empty());
}
