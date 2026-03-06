# Phase 10 Quick Reference

## Files Added

### Headers (include/)
```
include/logstory/narrative/timeline_writer.hpp    - CSV timeline export
include/logstory/narrative/schema_version.hpp     - JSON schema version constant
include/logstory/io/output_manager.hpp            - Output directory management
```

### Implementation (src/)
```
src/narrative/timeline_writer.cpp                 - CSV writer implementation
src/io/output_manager.cpp                         - Output manager implementation
```

### Documentation (docs/)
```
docs/schema.json                                   - JSON Schema v7 definition
docs/phase10_summary.md                           - Implementation summary
docs/PHASE10_REFERENCE.md                         - This file
```

### Tests (tests/)
```
tests/unit/test_output_phase10.cpp                - Comprehensive unit tests
```

## Key Classes

### TimelineWriter
```cpp
namespace logstory::narrative {
    class TimelineWriter {
        // Write report timeline highlights to CSV
        void write_highlights(const Report& report, std::ostream& out);
        
        // Write all events to CSV  
        void write_events(const std::vector<core::Event>& events, std::ostream& out);
        
        // Write to file
        bool write_to_file(const Report& report, const std::string& filepath);
    };
}
```

### OutputManager
```cpp
namespace logstory::io {
    enum class OutputFormat {
        MARKDOWN = 0x01,
        JSON     = 0x02,
        CSV      = 0x04,
        ALL      = MARKDOWN | JSON | CSV
    };
    
    class OutputManager {
        // Prepare output directory
        bool prepare_directory();
        
        // Write report in configured formats
        WriteResult write_report(const narrative::Report& report);
        
        // Write all outputs (report + timeline)
        WriteResult write_all(const narrative::Report& report,
                             const std::vector<core::Event>& events);
    };
}
```

## Configuration Options

### TimelineWriterConfig
```cpp
struct TimelineWriterConfig {
    bool include_all_events = false;  // Export all events or just highlights
    bool include_raw_text = true;     // Include raw log text column
    size_t max_text_length = 200;     // Truncate long messages
};
```

### OutputConfig
```cpp
struct OutputConfig {
    std::filesystem::path output_dir = "output";
    OutputFormat formats = OutputFormat::ALL;
    bool overwrite_existing = true;
    bool create_directories = true;
};
```

## Output Files

| File | Format | Contents |
|------|--------|----------|
| `report.md` | Markdown | Human-readable analysis report |
| `report.json` | JSON | Machine-readable report (schema v1) |
| `timeline.csv` | CSV | Timeline highlights for spreadsheets |
| `events_timeline.csv` | CSV | Full event timeline (optional) |

## Test Results

**Status:** ✅ All Phase 10 tests passing

- TimelineWriter: 3 test cases, 16 assertions
- OutputManager: 5 test cases, 39 assertions
- Total: 8 test cases, 55 assertions

## Build Integration

### CMakeLists.txt changes:
```cmake
# Added to main executable:
src/narrative/timeline_writer.cpp
src/io/output_manager.cpp

# Added to test executable:
tests/unit/test_output_phase10.cpp
```

## Schema Version

Current version: **1**

Schema location: `docs/schema.json`

JSON reports include `"schema_version": 1` field.

## Usage Example

```cpp
#include "logstory/io/output_manager.hpp"

// Configure output
io::OutputConfig config;
config.output_dir = "analysis_output";
config.formats = io::OutputFormat::ALL;

// Create manager
io::OutputManager manager(config);

// Write all reports
auto result = manager.write_all(report, events);

// Check results
if (result.success) {
    std::cout << "Generated " << result.written_files.size() << " files:\n";
    for (const auto& file : result.written_files) {
        std::cout << "  - " << file << "\n";
    }
} else {
    std::cerr << "Error: " << result.error_message << "\n";
}
```

## CSV Format Details

### Timeline Highlights CSV:
```
Timestamp,Severity,Event ID,Description[,Raw Text]
2023-01-01 10:30:45.123,ERROR,42,Database connection failed
```

### Full Events CSV:
```
Event ID,Timestamp,Severity,Source,Message[,Raw Text]
1,2023-01-01 10:30:45.123,ERROR,app.log:42,Connection timeout,...
```

## Notes

- CSV fields with commas/quotes are properly escaped
- Timestamps use ISO 8601 format with milliseconds
- All file operations include error handling
- Directory creation is automatic (configurable)
- Windows and Unix filesystem compatible
