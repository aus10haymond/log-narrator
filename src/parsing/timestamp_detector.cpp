#include "logstory/parsing/timestamp_detector.hpp"
#include <regex>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <unordered_map>

namespace logstory::parsing {

std::optional<core::Timestamp> TimestampDetector::detect(const std::string& text) {
    // Try formats in order of specificity/reliability
    
    // 1. ISO 8601 (most specific, highest confidence)
    if (auto ts = try_iso8601(text)) {
        return ts;
    }
    
    // 2. Common log patterns
    if (auto ts = try_common_patterns(text)) {
        return ts;
    }
    
    // 3. Syslog format (less specific, medium confidence)
    if (auto ts = try_syslog(text)) {
        return ts;
    }
    
    // 4. Epoch (least specific, check last to avoid false positives)
    if (auto ts = try_epoch(text)) {
        return ts;
    }
    
    return std::nullopt;
}

std::optional<core::Timestamp> TimestampDetector::try_iso8601(const std::string& text) {
    // Match ISO 8601 patterns: YYYY-MM-DD, YYYY-MM-DDTHH:MM:SS, etc.
    std::regex iso_regex(
        R"((\d{4})-(\d{2})-(\d{2})(?:[T ](\d{2}):(\d{2}):(\d{2})(?:\.(\d+))?(?:Z|([+-]\d{2}):?(\d{2}))?)?)"
    );
    
    std::smatch match;
    if (!std::regex_search(text, match, iso_regex)) {
        return std::nullopt;
    }
    
    // Parse matched components
    int year = std::stoi(match[1].str());
    int month = std::stoi(match[2].str());
    int day = std::stoi(match[3].str());
    int hour = match[4].matched ? std::stoi(match[4].str()) : 0;
    int minute = match[5].matched ? std::stoi(match[5].str()) : 0;
    int second = match[6].matched ? std::stoi(match[6].str()) : 0;
    
    // Basic validation
    if (year < 1970 || year > 2100 || month < 1 || month > 12 || day < 1 || day > 31) {
        return std::nullopt;
    }
    
    // Convert to time_point (simplified - assumes UTC)
    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    
    auto time_c = std::mktime(&tm);
    auto tp = std::chrono::system_clock::from_time_t(time_c);
    
    // Determine confidence and timezone awareness
    bool has_tz = match[8].matched || text.find('Z') != std::string::npos;
    uint8_t confidence = 95; // ISO 8601 is highly reliable
    
    return core::Timestamp(tp, confidence, has_tz);
}

std::optional<core::Timestamp> TimestampDetector::try_common_patterns(const std::string& text) {
    // Match common patterns: YYYY/MM/DD HH:MM:SS, YYYY-MM-DD HH:MM:SS
    std::regex pattern_regex(
        R"((\d{4})[-/](\d{2})[-/](\d{2})\s+(\d{2}):(\d{2}):(\d{2})(?:\.(\d+))?)"
    );
    
    std::smatch match;
    if (!std::regex_search(text, match, pattern_regex)) {
        return std::nullopt;
    }
    
    int year = std::stoi(match[1].str());
    int month = std::stoi(match[2].str());
    int day = std::stoi(match[3].str());
    int hour = std::stoi(match[4].str());
    int minute = std::stoi(match[5].str());
    int second = std::stoi(match[6].str());
    
    // Validation
    if (year < 1970 || year > 2100 || month < 1 || month > 12 || day < 1 || day > 31) {
        return std::nullopt;
    }
    if (hour > 23 || minute > 59 || second > 59) {
        return std::nullopt;
    }
    
    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    
    auto time_c = std::mktime(&tm);
    auto tp = std::chrono::system_clock::from_time_t(time_c);
    
    return core::Timestamp(tp, 90, false); // High confidence, no explicit timezone
}

std::optional<core::Timestamp> TimestampDetector::try_syslog(const std::string& text) {
    // Match syslog format: Mon DD HH:MM:SS
    std::regex syslog_regex(
        R"((Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+(\d{1,2})\s+(\d{2}):(\d{2}):(\d{2}))"
    );
    
    std::smatch match;
    if (!std::regex_search(text, match, syslog_regex)) {
        return std::nullopt;
    }
    
    // Map month names
    static const std::unordered_map<std::string, int> month_map = {
        {"Jan", 0}, {"Feb", 1}, {"Mar", 2}, {"Apr", 3}, {"May", 4}, {"Jun", 5},
        {"Jul", 6}, {"Aug", 7}, {"Sep", 8}, {"Oct", 9}, {"Nov", 10}, {"Dec", 11}
    };
    
    int month = month_map.at(match[1].str());
    int day = std::stoi(match[2].str());
    int hour = std::stoi(match[3].str());
    int minute = std::stoi(match[4].str());
    int second = std::stoi(match[5].str());
    
    // Syslog doesn't include year, assume current year
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_c);
    
    std::tm tm = {};
    tm.tm_year = now_tm->tm_year;
    tm.tm_mon = month;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    
    auto time_c = std::mktime(&tm);
    auto tp = std::chrono::system_clock::from_time_t(time_c);
    
    return core::Timestamp(tp, 70, false); // Lower confidence due to missing year
}

std::optional<core::Timestamp> TimestampDetector::try_epoch(const std::string& text) {
    // Look for epoch timestamps (10 or 13 digits)
    std::regex epoch_regex(R"(\b(1[0-9]{9}|1[0-9]{12})\b)");
    
    std::smatch match;
    if (!std::regex_search(text, match, epoch_regex)) {
        return std::nullopt;
    }
    
    std::string epoch_str = match[1].str();
    long long epoch_value = std::stoll(epoch_str);
    
    // Check if it looks like a reasonable timestamp (year 2001-2100)
    long long min_epoch = 978307200;      // 2001-01-01
    long long max_epoch = 4102444800;     // 2100-01-01
    long long min_epoch_ms = min_epoch * 1000;
    long long max_epoch_ms = max_epoch * 1000;
    
    std::chrono::system_clock::time_point tp;
    
    if (epoch_value >= min_epoch && epoch_value <= max_epoch) {
        // Seconds
        tp = std::chrono::system_clock::from_time_t(static_cast<time_t>(epoch_value));
        return core::Timestamp(tp, 60, false);
    } else if (epoch_value >= min_epoch_ms && epoch_value <= max_epoch_ms) {
        // Milliseconds
        tp = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(epoch_value)
        );
        return core::Timestamp(tp, 60, false);
    }
    
    return std::nullopt;
}

} // namespace logstory::parsing
