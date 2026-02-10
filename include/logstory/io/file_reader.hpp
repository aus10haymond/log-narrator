#pragma once

#include "logstory/io/raw_line.hpp"
#include "logstory/core/error.hpp"
#include <vector>
#include <string>

namespace logstory::io {

/// Reads a file line-by-line and produces RawLine objects
class FileReader {
public:
    /// Read all lines from the given file path
    /// Returns error status if file cannot be opened or read
    core::Status read(const std::string& path, std::vector<RawLine>& out_lines);

private:
    /// Normalize line endings by stripping trailing \r
    static void normalize_line_ending(std::string& line);
};

} // namespace logstory::io
