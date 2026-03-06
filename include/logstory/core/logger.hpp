#pragma once

#include "logstory/cli/args.hpp"
#include <string>
#include <iostream>
#include <sstream>

namespace logstory::core {

/// Simple logger with verbosity control
class Logger {
public:
    explicit Logger(cli::Verbosity level = cli::Verbosity::NORMAL)
        : verbosity_(level) {}
    
    /// Set verbosity level
    void set_verbosity(cli::Verbosity level) { verbosity_ = level; }
    
    /// Get current verbosity
    cli::Verbosity verbosity() const { return verbosity_; }
    
    /// Log an info message (shown in NORMAL and above)
    template<typename... Args>
    void info(Args&&... args) {
        if (verbosity_ >= cli::Verbosity::NORMAL) {
            print(std::cout, std::forward<Args>(args)...);
        }
    }
    
    /// Log a verbose message (shown in VERBOSE and above)
    template<typename... Args>
    void verbose(Args&&... args) {
        if (verbosity_ >= cli::Verbosity::VERBOSE) {
            print(std::cout, "[VERBOSE] ", std::forward<Args>(args)...);
        }
    }
    
    /// Log a debug message (shown in DEBUG only)
    template<typename... Args>
    void debug(Args&&... args) {
        if (verbosity_ >= cli::Verbosity::DEBUG) {
            print(std::cout, "[DEBUG] ", std::forward<Args>(args)...);
        }
    }
    
    /// Log an error message (always shown)
    template<typename... Args>
    void error(Args&&... args) {
        print(std::cerr, "[ERROR] ", std::forward<Args>(args)...);
    }
    
    /// Log a warning message (shown in NORMAL and above)
    template<typename... Args>
    void warning(Args&&... args) {
        if (verbosity_ >= cli::Verbosity::NORMAL) {
            print(std::cerr, "[WARN] ", std::forward<Args>(args)...);
        }
    }
    
    /// Check if quiet mode is enabled
    bool is_quiet() const { return verbosity_ == cli::Verbosity::QUIET; }
    
    /// Check if verbose mode is enabled
    bool is_verbose() const { return verbosity_ >= cli::Verbosity::VERBOSE; }
    
    /// Check if debug mode is enabled
    bool is_debug() const { return verbosity_ >= cli::Verbosity::DEBUG; }
    
private:
    cli::Verbosity verbosity_;
    
    /// Helper to print variadic arguments
    template<typename Stream, typename... Args>
    void print(Stream& stream, Args&&... args) {
        (stream << ... << args) << '\n';
    }
};

/// Global logger instance
extern Logger g_logger;

} // namespace logstory::core
