#include "logstory/io/dir_scanner.hpp"
#include <filesystem>
#include <algorithm>

namespace logstory::io {

const std::vector<std::string> DirScanner::DEFAULT_EXTENSIONS = {
    ".log", ".txt", ".jsonl"
};

core::Status DirScanner::scan(const std::string& dir_path, 
                              std::vector<std::string>& out_files) {
    return scan(dir_path, DEFAULT_EXTENSIONS, out_files);
}

core::Status DirScanner::scan(const std::string& dir_path,
                              const std::vector<std::string>& extensions,
                              std::vector<std::string>& out_files) {
    namespace fs = std::filesystem;

    // Check if directory exists
    if (!fs::exists(dir_path)) {
        return core::Status(core::ErrorCode::DIRECTORY_NOT_FOUND,
                           "Directory not found: " + dir_path);
    }

    // Check if it's actually a directory
    if (!fs::is_directory(dir_path)) {
        return core::Status(core::ErrorCode::INVALID_INPUT,
                           "Not a directory: " + dir_path);
    }

    // Collect all matching files
    std::vector<std::string> found_files;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
            if (entry.is_regular_file()) {
                const std::string path = entry.path().string();
                if (has_allowed_extension(path, extensions)) {
                    found_files.push_back(path);
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        return core::Status(core::ErrorCode::UNKNOWN_ERROR,
                           "Error scanning directory: " + std::string(e.what()));
    }

    // Sort lexicographically for deterministic output
    std::sort(found_files.begin(), found_files.end());

    // Check if we found any files
    if (found_files.empty()) {
        return core::Status(core::ErrorCode::DIRECTORY_EMPTY,
                           "No matching log files found in directory: " + dir_path);
    }

    out_files = std::move(found_files);
    return core::Status::OK();
}

bool DirScanner::has_allowed_extension(const std::string& path,
                                       const std::vector<std::string>& extensions) {
    namespace fs = std::filesystem;
    
    std::string ext = fs::path(path).extension().string();
    
    // Convert to lowercase for case-insensitive comparison
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    for (const auto& allowed : extensions) {
        std::string allowed_lower = allowed;
        std::transform(allowed_lower.begin(), allowed_lower.end(), 
                      allowed_lower.begin(), ::tolower);
        if (ext == allowed_lower) {
            return true;
        }
    }
    
    return false;
}

} // namespace logstory::io
