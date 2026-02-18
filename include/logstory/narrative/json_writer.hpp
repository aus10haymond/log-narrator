#pragma once

#include "logstory/narrative/report.hpp"
#include <string>
#include <ostream>

namespace logstory::narrative {

// Schema version for JSON output
constexpr int kSchemaVersion = 1;

// Writes reports in JSON format
class JSONWriter {
public:
    JSONWriter() = default;
    
    // Write report to output stream
    void write(const Report& report, std::ostream& out);
    
    // Write report to file
    bool write_to_file(const Report& report, const std::string& filepath);
    
private:
    void write_string(std::ostream& out, const std::string& str);
    void write_timestamp(std::ostream& out, const std::chrono::system_clock::time_point& tp);
    std::string escape_json(const std::string& str);
};

} // namespace logstory::narrative
