#pragma once

#include "logstory/io/raw_line.hpp"
#include "logstory/io/record.hpp"
#include <vector>
#include <string>

namespace logstory::io {

/// Configuration for multiline framing
struct MultilineFramerConfig {
    size_t max_lines_per_record = 1000;
    size_t max_chars_per_record = 100000;
};

/// Converts RawLines into Records with multiline stack trace detection
class MultilineFramer {
public:
    explicit MultilineFramer(MultilineFramerConfig config = MultilineFramerConfig())
        : config_(config) {}

    /// Frame lines into records, merging continuation lines
    void frame(const std::vector<RawLine>& lines, std::vector<Record>& out_records);

private:
    MultilineFramerConfig config_;

    /// Check if a line is a continuation of the previous record
    bool is_continuation(const Record& prev_record, const RawLine& next_line) const;

    /// Check if text looks like an error/exception message
    bool looks_like_error(const std::string& text) const;

    /// Check if line starts with whitespace
    bool starts_with_whitespace(const std::string& text) const;
};

} // namespace logstory::io
