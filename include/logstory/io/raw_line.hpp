#pragma once

#include <string>
#include <cstdint>

namespace logstory::io {

/// A single raw line read from input with source metadata
struct RawLine {
    std::string text;
    std::string source_path;
    uint32_t line_no;

    RawLine() : line_no(0) {}

    RawLine(std::string txt, std::string src, uint32_t ln)
        : text(std::move(txt)), source_path(std::move(src)), line_no(ln) {}
};

} // namespace logstory::io
