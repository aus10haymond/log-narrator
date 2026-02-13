#include "logstory/parsing/kv_extractor.hpp"
#include <regex>
#include <algorithm>

namespace logstory::parsing {

void KVExtractor::extract(const std::string& text, core::TagMap& tags) {
    // Match key=value patterns (conservative to avoid false positives)
    // Matches: key=value, key="value", key='value'
    std::regex kv_regex(R"((\w+)\s*=\s*(?:\"([^\"]*)\"|'([^']*)'|([^\s,;]+)))");
    
    auto begin = std::sregex_iterator(text.begin(), text.end(), kv_regex);
    auto end = std::sregex_iterator();
    
    for (auto it = begin; it != end; ++it) {
        std::smatch match = *it;
        std::string key = match[1].str();
        
        // Get the value from whichever group matched
        std::string value;
        if (match[2].matched) {
            value = match[2].str(); // Double-quoted
        } else if (match[3].matched) {
            value = match[3].str(); // Single-quoted
        } else {
            value = match[4].str(); // Unquoted
        }
        
        // Clean and store
        value = clean_value(value);
        
        // Skip if key or value is empty
        if (key.empty() || value.empty()) {
            continue;
        }
        
        // Skip common non-metadata patterns (too generic)
        static const std::vector<std::string> skip_keys = {
            "at", "in", "of", "to", "for", "the"
        };
        std::string lower_key = key;
        std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        
        bool should_skip = false;
        for (const auto& skip : skip_keys) {
            if (lower_key == skip) {
                should_skip = true;
                break;
            }
        }
        
        if (!should_skip) {
            tags[key] = value;
        }
    }
}

std::string KVExtractor::clean_value(const std::string& value) {
    std::string cleaned = value;
    
    // Trim whitespace
    cleaned.erase(0, cleaned.find_first_not_of(" \t\n\r"));
    cleaned.erase(cleaned.find_last_not_of(" \t\n\r") + 1);
    
    // Strip trailing punctuation (comma, semicolon, period)
    while (!cleaned.empty()) {
        char last = cleaned.back();
        if (last == ',' || last == ';' || last == '.') {
            cleaned.pop_back();
        } else {
            break;
        }
    }
    
    return cleaned;
}

} // namespace logstory::parsing
