# Log Narrator

**Automated log analysis and narrative report generation**

Log Narrator transforms raw logs into structured, human-readable narratives that help you understand what happened in your system. Instead of manually grep-ing through thousands of log lines, get an executive summary, timeline highlights, and detected failure patterns—all automatically.

## Why Log Narrator?

Debugging production incidents often involves:
- 📄 Searching through thousands of log lines across multiple files
- 🔍 Manually correlating timestamps, request IDs, and error patterns
- 🧩 Piecing together what happened from scattered evidence
- ⏰ Spending hours reconstructing timelines

Log Narrator automates this:
- ✅ **Automated Analysis**: Ingests logs, detects patterns, and generates reports
- 📊 **Multiple Output Formats**: Markdown reports, JSON for tooling, CSV timelines
- 🎯 **Pattern Detection**: Identifies crash loops, retry-timeout chains, error bursts
- 🔗 **Correlation Tracking**: Links events by request_id, trace_id, and other IDs
- 📝 **Evidence-Based Findings**: Every finding cites specific log lines as evidence

## Quick Start

```bash
# Analyze a single log file
log-narrator app.log

# Analyze all logs in a directory
log-narrator logs/

# Generate only markdown report
log-narrator --format md app.log

# Analyze with time filter
log-narrator --since 2024-03-06T10:00:00Z --until 2024-03-06T11:00:00Z app.log

# Verbose output to see what's happening
log-narrator --verbose logs/
```

## Sample Output

Given a log file with repeated startup failures:

```log
2024-03-06 10:00:00 [INFO] Server starting on port 8080
2024-03-06 10:00:02 [ERROR] Fatal error: Cannot bind to port 8080
2024-03-06 10:00:02 [ERROR] Server crashed
2024-03-06 10:00:05 [INFO] Server starting on port 8080
2024-03-06 10:00:07 [ERROR] Fatal error: Cannot bind to port 8080
...
```

Log Narrator generates:

### Executive Summary
- ⚠️ **Critical**: Detected crash/restart loop (6 restarts in 60 seconds)
- 📊 **Statistics**: 30 total events, 12 errors, 6 warnings
- ⏱️ **Timeline**: Analysis covers 2024-03-06 10:00:00 - 10:01:00

### Findings

**🔴 Critical: Crash Loop Detected**
- **Confidence**: 95%
- **Description**: Application repeatedly crashed and restarted 6 times within 1 minute
- **Evidence**: Lines 1-20 in server.log show repeated startup → crash pattern
- **Impact**: Service unavailable during entire time window

## Installation

### Build from Source

```bash
# Prerequisites: CMake 3.20+, C++17 compiler
git clone https://github.com/yourusername/log-narrator.git
cd log-narrator
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Binary will be at: build/Release/log_narrator.exe (Windows) or build/log_narrator (Linux/Mac)
```

## Usage

### Basic Commands

```bash
# Analyze a file
log-narrator app.log

# Analyze multiple files
log-narrator app.log system.log error.log

# Analyze directory (recursively finds .log, .txt, .jsonl files)
log-narrator logs/

# Read from stdin
cat app.log | log-narrator -
kubectl logs pod-name | log-narrator -
```

### Output Control

```bash
# Custom output directory
log-narrator --out reports/ logs/

# Choose output format
log-narrator --format md app.log      # Markdown only
log-narrator --format json app.log    # JSON only
log-narrator --format csv app.log     # CSV timeline only
log-narrator --format all app.log     # All formats (default)
```

### Time Filtering

```bash
# ISO8601 timestamps
log-narrator --since 2024-03-06T10:00:00Z app.log
log-narrator --until 2024-03-06T12:00:00Z app.log
log-narrator --since 2024-03-06T10:00:00Z --until 2024-03-06T12:00:00Z app.log

# Relative times
log-narrator --since 1h app.log       # Last hour
log-narrator --since 30m app.log      # Last 30 minutes
log-narrator --since 2d app.log       # Last 2 days
```

### Verbosity

```bash
log-narrator --quiet logs/      # No progress output
log-narrator logs/              # Normal progress (default)
log-narrator --verbose logs/    # Detailed progress
log-narrator --debug logs/      # Full debug output
```

## Output Files

Log Narrator generates three types of output (in the `out/` directory by default):

### 1. Markdown Report (`report.md`)
Human-readable narrative with:
- Executive summary
- Timeline highlights
- Detailed findings with evidence
- Statistics and metrics

### 2. JSON Report (`report.json`)
Machine-readable structured data:
- Schema version for compatibility
- All findings with metadata
- Complete statistics
- Suitable for integration with other tools

### 3. CSV Timeline (`timeline.csv`)
Spreadsheet-friendly event timeline:
- Timestamp, severity, message, source location
- Easy to import into Excel, Google Sheets
- Filterable and sortable

## Supported Log Formats

Log Narrator automatically detects and parses:

- **Text logs**: Standard application logs with various timestamp formats
- **JSONL**: JSON Lines format with automatic field extraction
- **Multiline**: Stack traces and exception messages
- **Mixed formats**: Handles heterogeneous logs from multiple sources

Supported timestamp formats:
- ISO8601: `2024-03-06T10:30:45Z`
- Standard: `2024-03-06 10:30:45`
- Syslog: `Jan 15 10:30:45`
- Epoch: Unix timestamps (seconds or milliseconds)

## Pattern Detection

Log Narrator includes built-in rules to detect common failure patterns:

### Crash/Restart Loops
Detects when an application repeatedly crashes and restarts, indicating a persistent failure.

### Retry-to-Timeout
Identifies chains of retry attempts that eventually time out, suggesting connectivity or dependency issues.

### Error Burst After Deployment
Finds sudden spikes in errors shortly after deployment or configuration changes.

See [docs/rules.md](docs/rules.md) for detailed information about the rules engine.

## Architecture

Log Narrator processes logs through a multi-stage pipeline:

```
Ingestion → Framing → Parsing → Analysis → Rules → Reports
   ↓          ↓          ↓          ↓         ↓        ↓
 Files    Records    Events    Episodes  Findings  MD/JSON/CSV
```

See [docs/design.md](docs/design.md) for architectural details.

## Limitations

- **Timestamp dependency**: Time-based features require parseable timestamps
- **Memory usage**: Loads entire log set into memory (not suitable for TB-scale logs)
- **Pattern coverage**: Currently detects 3 built-in patterns (more coming)
- **No streaming**: Batch processing only (not real-time analysis)

## Roadmap

- [ ] More built-in detection rules (deadlocks, memory leaks, etc.)
- [ ] Custom rule support via configuration files
- [ ] Streaming analysis for real-time monitoring
- [ ] HTML report output with interactive visualizations
- [ ] Performance optimization for larger log files
- [ ] LLM integration for natural language summaries

## Contributing

Contributions welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Submit a pull request

## License

MIT License - See LICENSE file for details

## Acknowledgments

Built with:
- C++17
- CMake
- Catch2 for testing

---

**Version**: 0.1.0  
**Status**: Initial Release
