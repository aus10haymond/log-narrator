#pragma once

#include <chrono>
#include <cstdint>

namespace logstory::core {

/// Timestamp with confidence and timezone information
struct Timestamp {
    using time_point = std::chrono::system_clock::time_point;
    
    time_point tp;
    uint8_t confidence;  // 0-100, higher is more confident
    bool tz_known;       // true if timezone was explicitly specified
    
    Timestamp()
        : tp(), confidence(0), tz_known(false) {}
    
    Timestamp(time_point t, uint8_t conf = 100, bool tz = false)
        : tp(t), confidence(conf), tz_known(tz) {}
    
    /// Check if timestamp is valid (confidence > 0)
    bool is_valid() const {
        return confidence > 0;
    }
};

} // namespace logstory::core
