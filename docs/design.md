# Architecture Design

## Overview

Log Narrator processes logs through a multi-stage pipeline that transforms raw text into structured analysis and narrative reports.

## Pipeline Stages

```
┌──────────┐    ┌─────────┐    ┌─────────┐    ┌──────────┐    ┌───────┐    ┌─────────┐
│          │    │         │    │         │    │          │    │       │    │         │
│ Ingestion├───►│ Framing ├───►│ Parsing ├───►│ Analysis ├───►│ Rules ├───►│ Reports │
│          │    │         │    │         │    │          │    │       │    │         │
└──────────┘    └─────────┘    └─────────┘    └──────────┘    └───────┘    └─────────┘
     │               │              │               │              │             │
     ▼               ▼              ▼               ▼              ▼             ▼
  Files          Records        Events         Episodes       Findings    MD/JSON/CSV
  (raw)       (multiline)    (structured)    (grouped)    (patterns)      (output)
```

## 1. Ingestion

**Purpose**: Read log data from various sources into memory

**Input**: 
- File paths (single or multiple)
- Directory paths (recursive scan)
- Standard input (stdin)

**Output**: `std::vector<RawLine>`
- Each RawLine contains:
  - `text`: The raw log line
  - `source_path`: Origin file/source
  - `line_no`: Line number in source

**Components**:
- `FileReader`: Reads individual files
- `DirScanner`: Recursively scans directories for .log, .txt, .jsonl files
- `StdinReader`: Reads from stdin until EOF

**Key Features**:
- Line ending normalization (strips `\r` for cross-platform compatibility)
- Preserves source location for later citation
- Deterministic ordering (sorted paths for directories)

## 2. Framing

**Purpose**: Convert lines into logical records (handle multiline entries)

**Input**: `std::vector<RawLine>`

**Output**: `std::vector<Record>`
- Each Record contains:
  - `text`: Complete record text (may span multiple lines)
  - `src`: Source reference with start/end line numbers

**Components**:
- `RecordFramer`: Single-line framing (1 line = 1 record)
- `MultilineFramer`: Multiline detection for stack traces

**Multiline Heuristics**:
- Lines starting with whitespace after ERROR/EXCEPTION
- Lines starting with `at `, `Caused by:`, or `Traceback`
- Safety limits: max 1000 lines or 100KB per record

**Why This Stage?**
Stack traces and exceptions often span multiple lines but represent a single logical event. Framing ensures these are treated as atomic units.

## 3. Parsing

**Purpose**: Extract structured metadata from raw text records

**Input**: `std::vector<Record>`

**Output**: `std::vector<Event>`
- Each Event contains:
  - `id`: Unique identifier
  - `ts`: Optional timestamp with confidence score
  - `sev`: Severity level (DEBUG/INFO/WARN/ERROR/FATAL)
  - `message`: Extracted or original text
  - `src`: Source reference
  - `tags`: Key-value metadata (request_id, user_id, etc.)
  - `raw`: Original record text

**Components**:
- `TimestampDetector`: Parses various timestamp formats
- `SeverityDetector`: Classifies log severity
- `KVExtractor`: Extracts key=value pairs
- `EventParser`: Coordinates all extractors
- `JSONLParser`: Specialized parser for JSON Lines format

**Timestamp Formats Supported**:
- ISO8601: `2024-03-06T10:30:45Z`
- Standard: `2024-03-06 10:30:45`
- Syslog: `Jan 15 10:30:45`
- Epoch: Unix seconds or milliseconds

**Severity Detection**:
1. Explicit tokens: `[ERROR]`, `level=warn`, `"severity":"INFO"`
2. Keyword scoring (conservative to avoid false positives)

## 4. Analysis

**Purpose**: Organize events and compute aggregate insights

**Input**: `std::vector<Event>`

**Output**:
- `Stats`: Aggregate statistics (counts by severity, time series)
- `Episodes`: Grouped event sequences
- `Anomalies`: Detected unusual patterns
- `EventIndex`: Fast lookup structures

**Components**:

### EventIndex
- Time-based index (minute buckets)
- Severity index
- Correlation ID index (request_id, trace_id)

### EpisodeBuilder
Groups events into coherent "episodes" based on:
- Time gaps (configurable threshold)
- Shared correlation IDs
- Episode boundaries (deploys, restarts)

### StatsBuilder
Computes:
- Total event counts
- Counts by severity
- Time series (events per minute)
- Source file statistics

### AnomalyDetector
Three detector types:
1. **ErrorBurstDetector**: Finds spikes in error rate (threshold-based)
2. **RestartLoopDetector**: Identifies repeated startup messages
3. **MissingHeartbeatDetector**: Detects gaps in periodic log patterns (future)

## 5. Rules Engine

**Purpose**: Apply pattern-matching rules to generate findings

**Input**:
- Events
- Episodes
- Stats
- Anomalies

**Output**: `std::vector<Finding>`
- Each Finding contains:
  - `id`: Rule identifier
  - `title`: Human-readable finding title
  - `summary`: Description
  - `severity`: LOW/MEDIUM/HIGH/CRITICAL
  - `confidence`: 0.0-1.0 (how certain are we?)
  - `evidence`: List of event IDs supporting this finding

**Architecture**:
- `Rule` interface: Base class for all rules
- `RuleRegistry`: Manages rule execution
- `RuleContext`: Provides data to rules

**Built-in Rules**:

### CrashLoopRule
- Looks for: Repeated restart anomalies
- Confidence: Based on restart frequency
- Evidence: Startup/crash events

### RetryToTimeoutRule
- Looks for: Multiple retry messages followed by timeout
- Pattern: 3+ warnings with "retry" → ERROR with "timeout"
- Evidence: Retry events + timeout event

### ErrorBurstAfterChangeRule
- Looks for: Error burst anomaly + nearby deployment/config event
- Time window: 30 minutes after change
- Evidence: Change event + burst events

**Confidence Scoring**:
- High frequency patterns → higher confidence
- Explicit keywords → higher confidence
- Timing correlation → higher confidence

## 6. Reports

**Purpose**: Transform structured data into human/machine-readable outputs

**Input**:
- Events
- Stats
- Episodes
- Findings

**Output**: Three formats

### Markdown Report
Human-readable narrative with:
- Executive Summary (key findings, stats)
- Timeline Highlights
- Detailed Findings (with evidence excerpts)
- Statistics Section
- Evidence Appendix

### JSON Report
Structured data with:
- Schema version
- All findings with metadata
- Complete statistics
- Suitable for tool integration

### CSV Timeline
Event-by-event timeline:
- Columns: timestamp, severity, message, source, tags
- Sortable and filterable in spreadsheets

**Components**:
- `Narrator`: Generates Report structure
- `MarkdownWriter`: Formats as Markdown
- `JSONWriter`: Formats as JSON
- `TimelineWriter`: Formats as CSV
- `OutputManager`: Manages file writing

## Data Flow Example

```
app.log (input)
    ↓
["2024-03-06 10:00:00 [ERROR] Connection failed"] (RawLine)
    ↓
Record { text: "...", src: "app.log:1" }
    ↓
Event {
    id: 1,
    ts: 2024-03-06T10:00:00Z,
    sev: ERROR,
    message: "Connection failed",
    src: "app.log:1",
    tags: {},
    raw: "..."
}
    ↓
[Analysis: grouped into Episode, counted in Stats]
    ↓
[Rules: matched by ErrorBurstRule if part of a burst]
    ↓
Finding {
    title: "Error Burst Detected",
    confidence: 0.85,
    evidence: [event_id: 1, ...]
}
    ↓
report.md (output)
```

## Key Design Principles

### 1. Preserve Original Data
- Always keep `raw` text in events
- Maintain source references throughout
- Enable accurate evidence citation

### 2. Deterministic Output
- Stable sorting everywhere
- Tie-breakers for equal timestamps
- Reproducible analysis across runs

### 3. Gradual Enrichment
- Each stage adds structure
- Never discard information
- Optional metadata (timestamps, tags)

### 4. Evidence-Based
- Every finding cites specific events
- Confidence scores quantify certainty
- Users can verify conclusions

### 5. Extensibility
- New rules can be added easily
- New parsers for different formats
- Pluggable output writers

## Performance Considerations

### Current Implementation
- **Memory**: Entire log set in memory (~10-100MB per million events)
- **Time Complexity**: O(n) for most stages, O(n log n) for sorting
- **Suitable for**: 1K-10M events (1MB-10GB logs)

### Not Suitable For
- TB-scale logs (would need streaming)
- Real-time monitoring (batch-oriented)
- Extremely high cardinality (millions of unique IDs)

## Future Enhancements

- **Streaming Analysis**: Process logs incrementally
- **Distributed Processing**: Shard large log sets
- **Custom Rules**: User-defined rule files
- **ML Integration**: Pattern learning from historical data
- **Incremental Updates**: Only reprocess new events
