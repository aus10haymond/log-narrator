#pragma once

#include <string>
#include <cstdint>

namespace logstory::core {

/// Reference to a source location (file path + line range)
struct SourceRef {
    std::string source_path;
    uint32_t start_line;
    uint32_t end_line;

    SourceRef() : start_line(0), end_line(0) {}

    SourceRef(std::string path, uint32_t line)
        : source_path(std::move(path)), start_line(line), end_line(line) {}

    SourceRef(std::string path, uint32_t start, uint32_t end)
        : source_path(std::move(path)), start_line(start), end_line(end) {}

    /// Format as "path:start" or "path:start-end"
    std::string to_string() const;
};

} // namespace logstory::core
