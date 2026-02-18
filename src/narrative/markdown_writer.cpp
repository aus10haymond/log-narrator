#include "logstory/narrative/markdown_writer.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>

namespace logstory::narrative {

void MarkdownWriter::write(const Report& report, std::ostream& out) {
    write_header(report, out);
    write_summary(report, out);
    write_findings(report, out);
    write_timeline(report, out);
    write_evidence(report, out);
}

bool MarkdownWriter::write_to_file(const Report& report, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    write(report, file);
    file.close();
    return true;
}

void MarkdownWriter::write_header(const Report& report, std::ostream& out) {
    out << "# " << report.title << "\n\n";
    out << "**Generated:** " << format_timestamp(report.generated_at) << "\n\n";
    
    if (report.log_start_time.has_value() && report.log_end_time.has_value()) {
        out << "**Analysis Period:** " 
            << format_timestamp(*report.log_start_time) << " to "
            << format_timestamp(*report.log_end_time) << "\n\n";
    }
    
    out << "---\n\n";
}

void MarkdownWriter::write_summary(const Report& report, std::ostream& out) {
    out << "## Executive Summary\n\n";
    
    for (const auto& bullet : report.summary) {
        out << "- " << bullet.text << "\n";
    }
    
    out << "\n### Key Metrics\n\n";
    out << "- **Total Events:** " << report.total_events << "\n";
    out << "- **Errors:** " << report.error_count << "\n";
    out << "- **Warnings:** " << report.warning_count << "\n";
    out << "- **Findings:** " << report.findings.size() << "\n";
    
    out << "\n---\n\n";
}

void MarkdownWriter::write_findings(const Report& report, std::ostream& out) {
    out << "## Findings\n\n";
    
    if (report.findings.empty()) {
        out << "*No findings detected.*\n\n";
        return;
    }
    
    for (size_t i = 0; i < report.findings.size(); i++) {
        const auto& finding = report.findings[i];
        
        out << "### " << (i + 1) << ". " << finding.title << "\n\n";
        out << "**Severity:** " << rules::to_string(finding.severity) 
            << " | **Confidence:** " << std::fixed << std::setprecision(0) 
            << (finding.confidence * 100) << "%\n\n";
        
        out << finding.summary << "\n\n";
        
        if (!finding.evidence.empty()) {
            out << "**Evidence:**\n";
            for (const auto& ev : finding.evidence) {
                out << "- Event #" << ev.event_id << ": " << ev.description << "\n";
            }
            out << "\n";
        }
    }
    
    out << "---\n\n";
}

void MarkdownWriter::write_timeline(const Report& report, std::ostream& out) {
    out << "## Timeline Highlights\n\n";
    
    if (report.timeline.empty()) {
        out << "*No timeline highlights available.*\n\n";
        return;
    }
    
    out << "| Timestamp | Event | Severity |\n";
    out << "|-----------|-------|----------|\n";
    
    for (const auto& hl : report.timeline) {
        out << "| " << format_timestamp(hl.timestamp) 
            << " | " << escape_markdown(hl.description)
            << " | " << core::to_string(hl.severity) << " |\n";
    }
    
    out << "\n---\n\n";
}

void MarkdownWriter::write_evidence(const Report& report, std::ostream& out) {
    out << "## Evidence Appendix\n\n";
    
    if (report.evidence.empty()) {
        out << "*No evidence excerpts available.*\n\n";
        return;
    }
    
    for (const auto& excerpt : report.evidence) {
        out << "### Event #" << excerpt.event_id << "\n\n";
        out << "**Source:** `" << excerpt.source_ref << "`  \n";
        out << "**Time:** " << excerpt.timestamp_str << "  \n";
        out << "**Severity:** " << excerpt.severity_str << "\n\n";
        out << "```\n" << excerpt.text << "\n```\n\n";
    }
}

std::string MarkdownWriter::escape_markdown(const std::string& text) {
    std::string result;
    result.reserve(text.length());
    
    for (char c : text) {
        // Escape special markdown characters
        if (c == '|' || c == '\\' || c == '*' || c == '_' || c == '[' || c == ']') {
            result += '\\';
        }
        result += c;
    }
    
    return result;
}

std::string MarkdownWriter::format_timestamp(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_buf;
    
#ifdef _WIN32
    localtime_s(&tm_buf, &time_t);
#else
    localtime_r(&time_t, &tm_buf);
#endif
    
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

} // namespace logstory::narrative
