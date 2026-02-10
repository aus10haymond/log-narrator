#include "logstory/io/file_reader.hpp"
#include <fstream>
#include <filesystem>

namespace logstory::io {

core::Status FileReader::read(const std::string& path, std::vector<RawLine>& out_lines) {
    namespace fs = std::filesystem;

    // Check if file exists
    if (!fs::exists(path)) {
        return core::Status(core::ErrorCode::FILE_NOT_FOUND, 
                           "File not found: " + path);
    }

    // Check if it's a regular file
    if (!fs::is_regular_file(path)) {
        return core::Status(core::ErrorCode::INVALID_INPUT,
                           "Not a regular file: " + path);
    }

    // Try to open the file
    std::ifstream file(path);
    if (!file.is_open()) {
        return core::Status(core::ErrorCode::FILE_UNREADABLE,
                           "Failed to open file: " + path);
    }

    // Read line by line
    std::string line;
    uint32_t line_no = 1;
    while (std::getline(file, line)) {
        normalize_line_ending(line);
        out_lines.emplace_back(line, path, line_no);
        ++line_no;
    }

    // Check for read errors
    if (file.bad()) {
        return core::Status(core::ErrorCode::FILE_UNREADABLE,
                           "Error reading file: " + path);
    }

    return core::Status::OK();
}

void FileReader::normalize_line_ending(std::string& line) {
    // Strip trailing \r to normalize Windows line endings
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
}

} // namespace logstory::io
