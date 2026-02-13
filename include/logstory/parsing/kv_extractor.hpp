#pragma once

#include "logstory/core/tags.hpp"
#include <string>

namespace logstory::parsing {

/// Extracts key=value pairs from log text
class KVExtractor {
public:
    /// Extract key-value pairs from text
    /// Populates the provided TagMap with extracted pairs
    void extract(const std::string& text, core::TagMap& tags);

private:
    /// Clean extracted value (strip quotes, trim, etc.)
    std::string clean_value(const std::string& value);
};

} // namespace logstory::parsing
