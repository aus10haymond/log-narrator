#pragma once

#include "logstory/io/raw_line.hpp"
#include "logstory/io/record.hpp"
#include <vector>

namespace logstory::io {

/// Converts RawLines into Records (single-line framing)
class RecordFramer {
public:
    /// Convert each RawLine into a single-line Record
    void frame(const std::vector<RawLine>& lines, std::vector<Record>& out_records);
};

} // namespace logstory::io
