#pragma once

#include "logstory/core/source_ref.hpp"
#include <string>

namespace logstory::io {

/// A record represents one logical log entry, which may span multiple lines
struct Record {
    core::SourceRef src;
    std::string text;

    Record() = default;

    Record(core::SourceRef source, std::string content)
        : src(std::move(source)), text(std::move(content)) {}
};

} // namespace logstory::io
