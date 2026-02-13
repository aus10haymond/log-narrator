#include "logstory/parsing/event_parser.hpp"

namespace logstory::parsing {

core::Event EventParser::parse(const io::Record& record) {
    // Create event with ID and source reference
    core::Event event(next_id_++, record.src);
    
    // Store raw text
    event.raw = record.text;
    
    // Try to parse timestamp
    event.ts = ts_detector_.detect(record.text);
    
    // Detect severity
    event.sev = sev_detector_.detect(record.text);
    
    // Extract key-value pairs into tags
    kv_extractor_.extract(record.text, event.tags);
    
    // Use the full text as message for now (could be refined later)
    event.message = record.text;
    
    return event;
}

std::vector<core::Event> EventParser::parse_all(const std::vector<io::Record>& records) {
    std::vector<core::Event> events;
    events.reserve(records.size());
    
    for (const auto& record : records) {
        events.push_back(parse(record));
    }
    
    return events;
}

} // namespace logstory::parsing
