#include "logstory/io/record_framer.hpp"

namespace logstory::io {

void RecordFramer::frame(const std::vector<RawLine>& lines, std::vector<Record>& out_records) {
    out_records.clear();
    out_records.reserve(lines.size());
    
    for (const auto& line : lines) {
        // Create a SourceRef where start_line == end_line (single line)
        core::SourceRef src(line.source_path, line.line_no);
        
        // Create record with the line text
        out_records.emplace_back(std::move(src), line.text);
    }
}

} // namespace logstory::io
