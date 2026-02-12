#include "logstory/io/multiline_framer.hpp"
#include <algorithm>
#include <cctype>

namespace logstory::io {

void MultilineFramer::frame(const std::vector<RawLine>& lines, std::vector<Record>& out_records) {
    out_records.clear();
    
    if (lines.empty()) {
        return;
    }
    
    // Start with the first line as a new record
    core::SourceRef current_src(lines[0].source_path, lines[0].line_no);
    std::string current_text = lines[0].text;
    size_t lines_in_record = 1;
    
    for (size_t i = 1; i < lines.size(); ++i) {
        const RawLine& line = lines[i];
        
        // Create a temporary record for continuation check
        Record temp_record(current_src, current_text);
        
        // Check if this line should be merged with the current record
        bool should_merge = is_continuation(temp_record, line);
        
        // Check safety limits
        if (should_merge) {
            if (lines_in_record >= config_.max_lines_per_record ||
                current_text.size() + line.text.size() + 1 >= config_.max_chars_per_record) {
                should_merge = false;
            }
        }
        
        if (should_merge) {
            // Merge this line into the current record
            current_text += '\n';
            current_text += line.text;
            current_src.end_line = line.line_no;
            ++lines_in_record;
        } else {
            // Save the current record and start a new one
            out_records.emplace_back(std::move(current_src), std::move(current_text));
            
            // Start new record
            current_src = core::SourceRef(line.source_path, line.line_no);
            current_text = line.text;
            lines_in_record = 1;
        }
    }
    
    // Don't forget the last record
    out_records.emplace_back(std::move(current_src), std::move(current_text));
}

bool MultilineFramer::is_continuation(const Record& prev_record, const RawLine& next_line) const {
    const std::string& text = next_line.text;
    
    // Empty lines are not continuations
    if (text.empty()) {
        return false;
    }
    
    // Check for explicit stack trace markers
    // Java/Kotlin stack traces
    if (text.find("at ") == 0 || 
        text.find("\tat ") == 0) {
        return true;
    }
    
    // Java caused by
    if (text.find("Caused by:") == 0) {
        return true;
    }
    
    // Python traceback
    if (text.find("Traceback") == 0) {
        return true;
    }
    
    // Python file location
    if (text.find("  File \"") == 0) {
        return true;
    }
    
    // Common indented continuation patterns
    if (text.find("    at ") == 0 ||
        text.find("\t... ") == 0 ||
        text.find("... ") == 0) {
        return true;
    }
    
    // If the line starts with whitespace AND previous record looks like an error
    if (starts_with_whitespace(text) && looks_like_error(prev_record.text)) {
        return true;
    }
    
    return false;
}

bool MultilineFramer::looks_like_error(const std::string& text) const {
    // Check for common error/exception keywords
    static const std::vector<std::string> error_keywords = {
        "Exception", "Error", "ERROR", "FATAL", "SEVERE",
        "Traceback", "Stack trace", "stacktrace",
        "Caused by", "exception in", "failed",
        "Unhandled", "RuntimeException", "NullPointerException"
    };
    
    for (const auto& keyword : error_keywords) {
        if (text.find(keyword) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

bool MultilineFramer::starts_with_whitespace(const std::string& text) const {
    return !text.empty() && std::isspace(static_cast<unsigned char>(text[0]));
}

} // namespace logstory::io
