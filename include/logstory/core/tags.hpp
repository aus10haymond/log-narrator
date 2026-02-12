#pragma once

#include <string>
#include <unordered_map>

namespace logstory::core {

/// Map of extracted metadata fields (key-value pairs)
using TagMap = std::unordered_map<std::string, std::string>;

} // namespace logstory::core
