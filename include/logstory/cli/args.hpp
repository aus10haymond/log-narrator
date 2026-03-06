#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>

namespace logstory::cli {

/// Output format options
enum class OutputFormat {
    MARKDOWN,
    JSON,
    CSV,
    ALL
};

/// Verbosity level
enum class Verbosity {
    QUIET,   // No progress output
    NORMAL,  // Standard progress messages
    VERBOSE, // Detailed progress
    DEBUG    // Full debug output
};

/// Configuration from command-line arguments
struct Args {
    // Input
    std::vector<std::string> input_paths;
    bool use_stdin = false;
    
    // Output
    std::string output_dir = "out";
    OutputFormat format = OutputFormat::ALL;
    
    // Filtering
    std::optional<std::string> since;  // Time filter start (ISO8601 or relative like "1h")
    std::optional<std::string> until;  // Time filter end
    
    // Behavior
    Verbosity verbosity = Verbosity::NORMAL;
    bool show_help = false;
    bool show_version = false;
    
    // Validation
    bool is_valid() const;
    std::string get_error_message() const;
};

/// Parse command-line arguments
class ArgParser {
public:
    ArgParser() = default;
    
    /// Parse arguments from main(argc, argv)
    Args parse(int argc, char** argv);
    
    /// Print usage/help message
    void print_help(const std::string& program_name) const;
    
    /// Print version information
    void print_version() const;
    
private:
    std::string error_message_;
    
    bool parse_output_format(const std::string& format_str, OutputFormat& out);
    bool parse_verbosity(const std::string& verb_str, Verbosity& out);
};

/// Convert OutputFormat to string
std::string to_string(OutputFormat fmt);

/// Convert Verbosity to string
std::string to_string(Verbosity verb);

} // namespace logstory::cli
