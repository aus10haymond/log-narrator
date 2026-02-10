#include "logstory/io/stdin_reader.hpp"
#include <iostream>

namespace logstory::io {

core::Status StdinReader::read(std::vector<RawLine>& out_lines) {
    std::string line;
    uint32_t line_no = 1;
    
    while (std::getline(std::cin, line)) {
        normalize_line_ending(line);
        out_lines.emplace_back(line, "stdin", line_no);
        ++line_no;
    }

    // Check for read errors (not EOF, which is expected)
    if (std::cin.bad()) {
        return core::Status(core::ErrorCode::FILE_UNREADABLE,
                           "Error reading from stdin");
    }

    return core::Status::OK();
}

void StdinReader::normalize_line_ending(std::string& line) {
    // Strip trailing \r to normalize Windows line endings
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
}

} // namespace logstory::io
