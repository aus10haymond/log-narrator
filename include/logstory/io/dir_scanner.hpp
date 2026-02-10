#pragma once

#include "logstory/core/error.hpp"
#include <vector>
#include <string>

namespace logstory::io {

/// Scans directories recursively for log files
class DirScanner {
public:
    /// Default extensions to search for
    static const std::vector<std::string> DEFAULT_EXTENSIONS;

    /// Scan a directory recursively for files with matching extensions
    /// Results are sorted lexicographically for deterministic output
    core::Status scan(const std::string& dir_path, std::vector<std::string>& out_files);

    /// Scan with custom extensions
    core::Status scan(const std::string& dir_path, 
                     const std::vector<std::string>& extensions,
                     std::vector<std::string>& out_files);

private:
    /// Check if a file has one of the allowed extensions
    static bool has_allowed_extension(const std::string& path, 
                                      const std::vector<std::string>& extensions);
};

} // namespace logstory::io
