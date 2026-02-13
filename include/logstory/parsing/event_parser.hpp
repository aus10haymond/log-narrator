#pragma once

#include "logstory/io/record.hpp"
#include "logstory/core/event.hpp"
#include "logstory/parsing/timestamp_detector.hpp"
#include "logstory/parsing/severity_detector.hpp"
#include "logstory/parsing/kv_extractor.hpp"
#include <vector>

namespace logstory::parsing {

/// Converts Records to Events by parsing and extracting metadata
class EventParser {
public:
    EventParser()
        : next_id_(1) {}
    
    /// Parse a single record into an event
    core::Event parse(const io::Record& record);
    
    /// Parse multiple records into events
    std::vector<core::Event> parse_all(const std::vector<io::Record>& records);

private:
    core::EventId next_id_;
    TimestampDetector ts_detector_;
    SeverityDetector sev_detector_;
    KVExtractor kv_extractor_;
};

} // namespace logstory::parsing
