#pragma once

#include "logstory/narrative/report.hpp"
#include <string>
#include <ostream>

namespace logstory::narrative {

// Writes reports in Markdown format
class MarkdownWriter {
public:
    MarkdownWriter() = default;
    
    // Write report to output stream
    void write(const Report& report, std::ostream& out);
    
    // Write report to file
    bool write_to_file(const Report& report, const std::string& filepath);
    
private:
    void write_header(const Report& report, std::ostream& out);
    void write_summary(const Report& report, std::ostream& out);
    void write_timeline(const Report& report, std::ostream& out);
    void write_findings(const Report& report, std::ostream& out);
    void write_evidence(const Report& report, std::ostream& out);
    
    std::string escape_markdown(const std::string& text);
    std::string format_timestamp(const std::chrono::system_clock::time_point& tp);
};

} // namespace logstory::narrative
