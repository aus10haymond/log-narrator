#include "logstory/core/source_ref.hpp"
#include <sstream>

namespace logstory::core {

std::string SourceRef::to_string() const {
    std::ostringstream oss;
    oss << source_path << ":" << start_line;
    if (end_line != start_line) {
        oss << "-" << end_line;
    }
    return oss.str();
}

} // namespace logstory::core
