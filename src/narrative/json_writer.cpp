#include "logstory/narrative/json_writer.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>

namespace logstory::narrative {

void JSONWriter::write(const Report& report, std::ostream& out) {
    out << "{\n";
    out << "  \"schema_version\": " << kSchemaVersion << ",\n";
    out << "  \"title\": "; write_string(out, report.title); out << ",\n";
    out << "  \"generated_at\": "; write_timestamp(out, report.generated_at); out << ",\n";
    
    // Analysis period
    if (report.log_start_time.has_value()) {
        out << "  \"log_start_time\": "; write_timestamp(out, *report.log_start_time); out << ",\n";
    }
    if (report.log_end_time.has_value()) {
        out << "  \"log_end_time\": "; write_timestamp(out, *report.log_end_time); out << ",\n";
    }
    
    // Stats
    out << "  \"total_events\": " << report.total_events << ",\n";
    out << "  \"error_count\": " << report.error_count << ",\n";
    out << "  \"warning_count\": " << report.warning_count << ",\n";
    
    // Summary
    out << "  \"summary\": [\n";
    for (size_t i = 0; i < report.summary.size(); i++) {
        out << "    "; write_string(out, report.summary[i].text);
        if (i < report.summary.size() - 1) out << ",";
        out << "\n";
    }
    out << "  ],\n";
    
    // Findings
    out << "  \"findings\": [\n";
    for (size_t i = 0; i < report.findings.size(); i++) {
        const auto& f = report.findings[i];
        out << "    {\n";
        out << "      \"id\": "; write_string(out, f.id); out << ",\n";
        out << "      \"title\": "; write_string(out, f.title); out << ",\n";
        out << "      \"severity\": "; write_string(out, rules::to_string(f.severity)); out << ",\n";
        out << "      \"confidence\": " << f.confidence << ",\n";
        out << "      \"summary\": "; write_string(out, f.summary); out << ",\n";
        out << "      \"evidence\": [\n";
        for (size_t j = 0; j < f.evidence.size(); j++) {
            const auto& ev = f.evidence[j];
            out << "        {\n";
            out << "          \"event_id\": " << ev.event_id << ",\n";
            out << "          \"description\": "; write_string(out, ev.description); out << "\n";
            out << "        }";
            if (j < f.evidence.size() - 1) out << ",";
            out << "\n";
        }
        out << "      ]\n";
        out << "    }";
        if (i < report.findings.size() - 1) out << ",";
        out << "\n";
    }
    out << "  ],\n";
    
    // Timeline
    out << "  \"timeline\": [\n";
    for (size_t i = 0; i < report.timeline.size(); i++) {
        const auto& hl = report.timeline[i];
        out << "    {\n";
        out << "      \"timestamp\": "; write_timestamp(out, hl.timestamp); out << ",\n";
        out << "      \"description\": "; write_string(out, hl.description); out << ",\n";
        out << "      \"severity\": "; write_string(out, core::to_string(hl.severity)); out << ",\n";
        out << "      \"event_id\": " << hl.event_id << "\n";
        out << "    }";
        if (i < report.timeline.size() - 1) out << ",";
        out << "\n";
    }
    out << "  ],\n";
    
    // Evidence
    out << "  \"evidence\": [\n";
    for (size_t i = 0; i < report.evidence.size(); i++) {
        const auto& ex = report.evidence[i];
        out << "    {\n";
        out << "      \"event_id\": " << ex.event_id << ",\n";
        out << "      \"source_ref\": "; write_string(out, ex.source_ref); out << ",\n";
        out << "      \"timestamp\": "; write_string(out, ex.timestamp_str); out << ",\n";
        out << "      \"severity\": "; write_string(out, ex.severity_str); out << ",\n";
        out << "      \"text\": "; write_string(out, ex.text); out << "\n";
        out << "    }";
        if (i < report.evidence.size() - 1) out << ",";
        out << "\n";
    }
    out << "  ]\n";
    
    out << "}\n";
}

bool JSONWriter::write_to_file(const Report& report, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    write(report, file);
    file.close();
    return true;
}

void JSONWriter::write_string(std::ostream& out, const std::string& str) {
    out << "\"" << escape_json(str) << "\"";
}

void JSONWriter::write_timestamp(std::ostream& out, const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_buf;
    
#ifdef _WIN32
    gmtime_s(&tm_buf, &time_t);
#else
    gmtime_r(&time_t, &tm_buf);
#endif
    
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
    write_string(out, oss.str());
}

std::string JSONWriter::escape_json(const std::string& str) {
    std::string result;
    result.reserve(str.length());
    
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (c < 0x20) {
                    // Control character - use unicode escape
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    result += buf;
                } else {
                    result += c;
                }
        }
    }
    
    return result;
}

} // namespace logstory::narrative
