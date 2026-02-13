#include "logstory/analysis/correlation_extractor.hpp"
#include <regex>
#include <algorithm>

namespace logstory::analysis {

void CorrelationExtractor::extract(core::Event& event) {
    extract_request_id(event);
    extract_trace_id(event);
    
    // Extract UUIDs and store the first one if not already present
    auto uuids = extract_uuids(event.raw);
    if (!uuids.empty() && event.tags.find("uuid") == event.tags.end()) {
        event.tags["uuid"] = uuids[0];
    }
}

void CorrelationExtractor::extract_request_id(core::Event& event) {
    // Check existing tags for various request ID field names
    static const std::vector<std::string> request_id_variants = {
        "request_id", "requestId", "reqId", "req_id",
        "x-request-id", "x_request_id", "RequestId"
    };
    
    for (const auto& variant : request_id_variants) {
        auto it = event.tags.find(variant);
        if (it != event.tags.end()) {
            // Normalize to standard "request_id"
            if (variant != "request_id") {
                event.tags["request_id"] = it->second;
            }
            return;
        }
    }
    
    // Try to extract from raw text using patterns
    std::regex request_pattern(
        R"((request[_-]?id|req[_-]?id|x[_-]?request[_-]?id)[=:\s]+([a-zA-Z0-9\-_]+))",
        std::regex_constants::icase
    );
    
    std::smatch match;
    if (std::regex_search(event.raw, match, request_pattern)) {
        event.tags["request_id"] = match[2].str();
    }
}

void CorrelationExtractor::extract_trace_id(core::Event& event) {
    // Check existing tags for various trace ID field names
    static const std::vector<std::string> trace_id_variants = {
        "trace_id", "traceId", "trace", "x-trace-id",
        "x_trace_id", "TraceId", "span_id", "spanId"
    };
    
    for (const auto& variant : trace_id_variants) {
        auto it = event.tags.find(variant);
        if (it != event.tags.end()) {
            // Normalize to standard "trace_id"
            if (variant != "trace_id") {
                event.tags["trace_id"] = it->second;
            }
            return;
        }
    }
    
    // Try to extract from raw text
    std::regex trace_pattern(
        R"((trace[_-]?id|span[_-]?id)[=:\s]+([a-zA-Z0-9\-_]+))",
        std::regex_constants::icase
    );
    
    std::smatch match;
    if (std::regex_search(event.raw, match, trace_pattern)) {
        event.tags["trace_id"] = match[2].str();
    }
}

std::vector<std::string> CorrelationExtractor::extract_uuids(const std::string& text) {
    std::vector<std::string> uuids;
    
    // UUID pattern: 8-4-4-4-12 hex digits
    std::regex uuid_pattern(
        R"(\b[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}\b)"
    );
    
    auto begin = std::sregex_iterator(text.begin(), text.end(), uuid_pattern);
    auto end = std::sregex_iterator();
    
    for (auto it = begin; it != end; ++it) {
        uuids.push_back(it->str());
    }
    
    return uuids;
}

bool CorrelationExtractor::is_uuid(const std::string& str) {
    std::regex uuid_pattern(
        R"(^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$)"
    );
    return std::regex_match(str, uuid_pattern);
}

} // namespace logstory::analysis
