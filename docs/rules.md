# Rules Engine

## Overview

The Rules Engine is the core intelligence of Log Narrator. It analyzes structured events and detected anomalies to identify meaningful failure patterns and operational issues.

## Architecture

### Rule Interface

Every rule implements the `Rule` interface:

```cpp
class Rule {
    virtual std::string id() const = 0;
    virtual std::string name() const = 0;
    virtual std::vector<Finding> evaluate(const RuleContext& context) = 0;
    virtual int priority() const { return 0; }  // Optional
};
```

### Rule Context

Rules receive a `RuleContext` containing:
- **Events**: All parsed log events
- **Stats**: Aggregate statistics
- **Episodes**: Grouped event sequences
- **Anomalies**: Detected patterns from anomaly detectors

### Finding Structure

Each finding produced by a rule contains:

```cpp
struct Finding {
    std::string id;              // Unique identifier
    std::string title;           // Human-readable title
    std::string summary;         // Detailed description
    FindingSeverity severity;    // LOW/MEDIUM/HIGH/CRITICAL
    double confidence;           // 0.0-1.0
    std::vector<Evidence> evidence;  // Supporting log events
};
```

## Built-in Rules

### 1. Crash Loop Rule

**ID**: `crash-loop`  
**Priority**: 90

**Purpose**: Detects when an application repeatedly crashes and restarts, indicating a persistent failure that prevents normal operation.

**Detection Logic**:
1. Looks for `RESTART_LOOP` anomalies from the `RestartLoopDetector`
2. Each anomaly represents 3+ restart events within a time window
3. Higher frequency → higher confidence

**Confidence Calculation**:
```
Base confidence: 0.75
+ 0.05 per restart beyond 3 (capped at 0.95)
```

**Example**:
```log
2024-03-06 10:00:00 [INFO] Server starting
2024-03-06 10:00:02 [ERROR] Failed to bind port
2024-03-06 10:00:05 [INFO] Server starting
2024-03-06 10:00:07 [ERROR] Failed to bind port
2024-03-06 10:00:10 [INFO] Server starting
...
```

**Finding**:
- Title: "Crash/Restart Loop Detected"
- Severity: CRITICAL
- Confidence: 85% (for 5 restarts)
- Evidence: All restart event IDs

**Common Root Causes**:
- Configuration errors (wrong port, missing files)
- Missing dependencies (libraries, environment variables)
- Resource conflicts (port already in use)
- Insufficient permissions

### 2. Retry-to-Timeout Rule

**ID**: `retry-to-timeout`  
**Priority**: 80

**Purpose**: Identifies chains of retry attempts that eventually exhaust and time out, suggesting connectivity or dependency failures.

**Detection Logic**:
1. Scans events for retry-related keywords: "retry", "retrying", "attempt"
2. Groups consecutive retry events within 5-minute windows
3. Checks if the sequence ends with a timeout error
4. Requires minimum 3 retries before timeout

**Pattern**:
```
WARN: retry 1
WARN: retry 2
WARN: retry 3
ERROR: timeout
```

**Confidence Calculation**:
```
Base confidence: 0.70
+ 0.05 per retry beyond 3
+ 0.10 if explicit "timeout" keyword in final error
```

**Example**:
```log
2024-03-06 10:00:00 [WARN] Payment gateway timeout, retry 1/5
2024-03-06 10:00:05 [WARN] Payment gateway timeout, retry 2/5
2024-03-06 10:00:10 [WARN] Payment gateway timeout, retry 3/5
2024-03-06 10:00:15 [WARN] Payment gateway timeout, retry 4/5
2024-03-06 10:00:20 [WARN] Payment gateway timeout, retry 5/5
2024-03-06 10:00:25 [ERROR] Operation timed out after 5 retries
```

**Finding**:
- Title: "Retries Leading to Timeout"
- Severity: HIGH
- Confidence: 90%
- Evidence: All retry events + timeout event

**Common Root Causes**:
- External service degradation
- Network connectivity issues
- Insufficient timeout values
- Rate limiting/throttling

### 3. Error Burst After Change Rule

**ID**: `error-burst-after-change`  
**Priority**: 85

**Purpose**: Correlates sudden error spikes with recent deployments or configuration changes, helping identify problematic releases.

**Detection Logic**:
1. Looks for `ERROR_BURST` anomalies (from `ErrorBurstDetector`)
2. Searches for change markers within 30 minutes before burst:
   - Keywords: "deploy", "deployment", "release", "config", "configuration"
   - Severity: INFO or higher
3. Correlates timing between change and burst

**Timing Window**:
```
[Change Event] ---[0-30 min]---> [Error Burst]
```

**Confidence Calculation**:
```
Base confidence: 0.75
+ 0.10 if change is within 10 minutes of burst
+ 0.05 if change explicitly mentions "deploy" or "config"
- 0.10 if time gap > 20 minutes
```

**Example**:
```log
2024-03-06 09:50:00 [INFO] ===== DEPLOYMENT STARTED: v2.1.0 =====
2024-03-06 09:50:30 [INFO] Configuration reloaded
2024-03-06 09:51:00 [ERROR] Database connection failed
2024-03-06 09:51:01 [ERROR] Authentication failed
2024-03-06 09:51:02 [ERROR] Service unavailable
... (15 more errors in next minute)
```

**Finding**:
- Title: "Error Burst Following Deployment/Config Change"
- Severity: HIGH
- Confidence: 85%
- Evidence: Change event + burst events

**Common Root Causes**:
- Breaking configuration changes
- Database migration issues
- Incompatible dependency versions
- Environment variable changes
- Feature flag toggles

## Evidence System

### Evidence Structure

```cpp
struct Evidence {
    EventId event_id;        // Reference to specific event
    std::string description;  // Why this event is relevant
    std::string excerpt;      // Sample of log text
};
```

### Evidence Collection

Rules must provide evidence for every finding:

1. **Primary Events**: Core events that triggered the pattern
   - Example: The timeout error in retry-timeout rule

2. **Supporting Events**: Context around the primary events
   - Example: All retry attempts before the timeout

3. **Correlation Events**: Events that help explain the pattern
   - Example: Deployment event before error burst

### Evidence in Reports

Evidence appears in reports as:

**Markdown**:
```markdown
### Evidence
- Line 10 (app.log): [ERROR] Connection timeout
- Line 5-9 (app.log): 5 retry attempts before timeout
```

**JSON**:
```json
{
  "evidence": [
    {"event_id": 10, "description": "Timeout error", "excerpt": "[ERROR] Connection timeout"},
    {"event_id": 5, "description": "First retry", "excerpt": "[WARN] Retry 1/5"}
  ]
}
```

## Confidence Scoring

### Philosophy

Confidence scores quantify how certain we are about a finding. They help users prioritize which findings to investigate first.

### Confidence Levels

- **< 0.50**: Low confidence (informational)
- **0.50-0.74**: Medium confidence (worth investigating)
- **0.75-0.89**: High confidence (likely issue)
- **≥ 0.90**: Very high confidence (definite issue)

### Factors That Increase Confidence

1. **Frequency**: More occurrences → higher confidence
   - 3 crashes: 75% confidence
   - 10 crashes: 90% confidence

2. **Explicit Keywords**: Specific error messages
   - Generic "error": Lower confidence
   - "TimeoutException": Higher confidence

3. **Timing Correlation**: Events close in time
   - Deploy + errors within 5 min: Higher confidence
   - Deploy + errors 25 min later: Lower confidence

4. **Pattern Completeness**: All expected components present
   - Retries with timeout: High confidence
   - Retries without timeout: Not a finding

### Factors That Decrease Confidence

1. **Ambiguity**: Multiple possible interpretations
2. **Sparse Data**: Too few events to establish pattern
3. **Long Time Gaps**: Events far apart in time
4. **Missing Context**: Incomplete event sequences

## Writing Custom Rules

### Step 1: Implement Rule Interface

```cpp
class MyCustomRule : public Rule {
public:
    std::string id() const override {
        return "my-custom-rule";
    }
    
    std::string name() const override {
        return "My Custom Pattern";
    }
    
    std::vector<Finding> evaluate(const RuleContext& ctx) override {
        std::vector<Finding> findings;
        
        // Your detection logic here
        
        return findings;
    }
};
```

### Step 2: Register the Rule

```cpp
RuleRegistry registry;
registry.register_rule(std::make_unique<MyCustomRule>());
```

### Step 3: Collect Evidence

```cpp
Finding finding;
finding.id = id() + "-001";
finding.title = "Pattern Detected";
finding.severity = FindingSeverity::HIGH;
finding.confidence = 0.85;

// Add evidence
for (auto& event : matching_events) {
    finding.add_evidence(event.id, "Relevant event");
}
```

## Best Practices

### 1. Be Conservative
- Start with lower confidence scores
- Require multiple confirming signals
- Avoid false positives

### 2. Provide Context
- Include at least 3 evidence events
- Show before/after context
- Explain why the pattern matters

### 3. Clear Descriptions
- Use specific, actionable titles
- Explain what was detected
- Suggest possible root causes

### 4. Respect Performance
- Don't scan all events for every rule
- Use anomalies and indices
- Bail out early if pattern can't match

### 5. Test Thoroughly
- Create fixture logs for each rule
- Test edge cases (0 events, 1 event, many events)
- Verify confidence scores make sense

## Future Enhancements

### Planned Features

1. **Custom Rule Files**: Load rules from YAML/JSON
2. **Rule Chaining**: Rules that build on other rules' findings
3. **Temporal Patterns**: Detect recurring daily/weekly patterns
4. **Correlation Rules**: Cross-system pattern detection
5. **Machine Learning**: Learn patterns from historical data

### Example Custom Rule File

```yaml
rules:
  - id: high-memory-before-crash
    name: Memory Pressure Before Crash
    pattern:
      - type: metric
        field: memory_usage
        operator: ">"
        value: 90
        within: 5m
      - type: keyword
        words: ["OutOfMemoryError", "OOM"]
    severity: CRITICAL
    confidence: 0.90
```

## Debugging Rules

### Verbose Mode

Run with `--debug` to see rule evaluation:

```bash
log-narrator --debug app.log
```

Output includes:
- Which rules ran
- How many findings each produced
- Confidence scores
- Evidence event IDs

### Testing Individual Rules

Create isolated test fixtures:

```cpp
TEST_CASE("RetryToTimeoutRule detects pattern") {
    std::vector<Event> events = create_test_events();
    RetryToTimeoutRule rule;
    RuleContext ctx;
    ctx.events = &events;
    
    auto findings = rule.evaluate(ctx);
    
    REQUIRE(findings.size() == 1);
    REQUIRE(findings[0].confidence > 0.75);
}
```

## Performance

### Typical Rule Evaluation Time

- Per rule: 1-50ms
- Total rules engine: 10-200ms
- Dominated by pattern scanning, not rule infrastructure

### Optimization Tips

1. Use anomaly pre-filtering
2. Early exit when pattern can't match
3. Cache event indices
4. Limit retrospective search windows

## Conclusion

The Rules Engine transforms raw log analysis into actionable insights. By combining pattern detection, evidence collection, and confidence scoring, it helps users quickly understand and respond to system issues.
