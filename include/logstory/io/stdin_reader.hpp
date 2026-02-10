#pragma once

#include "logstory/io/raw_line.hpp"
#include "logstory/core/error.hpp"
#include <vector>

namespace logstory::io {

/// Reads lines from stdin
class StdinReader {
public:
    /// Read all lines from stdin until EOF
    /// Lines are tagged with source_path="stdin"
    core::Status read(std::vector<RawLine>& out_lines);

private:
    /// Normalize line endings by stripping trailing \r
    static void normalize_line_ending(std::string& line);
};

} // namespace logstory::io
