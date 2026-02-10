#pragma once

#include <string>
#include <system_error>

namespace logstory::core {

enum class ErrorCode {
    OK = 0,
    FILE_NOT_FOUND,
    FILE_UNREADABLE,
    DIRECTORY_NOT_FOUND,
    DIRECTORY_EMPTY,
    INVALID_INPUT,
    UNKNOWN_ERROR
};

/// Simple result type for operations that may fail
struct Status {
    ErrorCode code;
    std::string message;

    Status() : code(ErrorCode::OK) {}
    Status(ErrorCode c, std::string msg = "") : code(c), message(std::move(msg)) {}

    bool ok() const { return code == ErrorCode::OK; }
    explicit operator bool() const { return ok(); }

    static Status OK() { return Status(ErrorCode::OK); }
};

} // namespace logstory::core
