#include "logstory/cli/args.hpp"
#include <iostream>
#include <algorithm>

namespace logstory::cli {

bool Args::is_valid() const {
    // Must have either stdin or input paths
    if (!use_stdin && input_paths.empty()) {
        return false;
    }
    
    // Can't have both stdin and files
    if (use_stdin && !input_paths.empty()) {
        return false;
    }
    
    return true;
}

std::string Args::get_error_message() const {
    if (!use_stdin && input_paths.empty()) {
        return "No input specified. Use '-' for stdin or provide file/directory paths.";
    }
    
    if (use_stdin && !input_paths.empty()) {
        return "Cannot use both stdin and file paths. Choose one input method.";
    }
    
    return "";
}

Args ArgParser::parse(int argc, char** argv) {
    Args args;
    error_message_.clear();
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        // Help flag
        if (arg == "-h" || arg == "--help") {
            args.show_help = true;
            return args;
        }
        
        // Version flag
        if (arg == "-v" || arg == "--version") {
            args.show_version = true;
            return args;
        }
        
        // Output directory
        if (arg == "-o" || arg == "--out" || arg == "--output") {
            if (i + 1 >= argc) {
                error_message_ = "Option " + arg + " requires an argument";
                return args;
            }
            args.output_dir = argv[++i];
            continue;
        }
        
        // Format selection
        if (arg == "-f" || arg == "--format") {
            if (i + 1 >= argc) {
                error_message_ = "Option " + arg + " requires an argument";
                return args;
            }
            if (!parse_output_format(argv[++i], args.format)) {
                error_message_ = "Invalid format. Use: md, json, csv, or all";
                return args;
            }
            continue;
        }
        
        // Since filter
        if (arg == "--since") {
            if (i + 1 >= argc) {
                error_message_ = "Option --since requires an argument";
                return args;
            }
            args.since = argv[++i];
            continue;
        }
        
        // Until filter
        if (arg == "--until") {
            if (i + 1 >= argc) {
                error_message_ = "Option --until requires an argument";
                return args;
            }
            args.until = argv[++i];
            continue;
        }
        
        // Verbosity
        if (arg == "-q" || arg == "--quiet") {
            args.verbosity = Verbosity::QUIET;
            continue;
        }
        
        if (arg == "--verbose") {
            args.verbosity = Verbosity::VERBOSE;
            continue;
        }
        
        if (arg == "--debug") {
            args.verbosity = Verbosity::DEBUG;
            continue;
        }
        
        // Stdin input
        if (arg == "-") {
            args.use_stdin = true;
            continue;
        }
        
        // Unknown option
        if (arg[0] == '-') {
            error_message_ = "Unknown option: " + arg;
            return args;
        }
        
        // Positional argument (input path)
        args.input_paths.push_back(arg);
    }
    
    return args;
}

void ArgParser::print_help(const std::string& program_name) const {
    std::cout << "Log Narrator - Automated log analysis and reporting tool\n\n";
    std::cout << "USAGE:\n";
    std::cout << "  " << program_name << " [OPTIONS] <INPUT>...\n";
    std::cout << "  " << program_name << " [OPTIONS] -\n\n";
    
    std::cout << "ARGUMENTS:\n";
    std::cout << "  <INPUT>...    One or more log files or directories to analyze\n";
    std::cout << "  -             Read from standard input\n\n";
    
    std::cout << "OPTIONS:\n";
    std::cout << "  -h, --help              Show this help message\n";
    std::cout << "  -v, --version           Show version information\n";
    std::cout << "  -o, --out <DIR>         Output directory [default: out]\n";
    std::cout << "  -f, --format <FMT>      Output format: md, json, csv, all [default: all]\n";
    std::cout << "  --since <TIME>          Only analyze events after this time (ISO8601)\n";
    std::cout << "  --until <TIME>          Only analyze events before this time (ISO8601)\n";
    std::cout << "  -q, --quiet             Suppress progress output\n";
    std::cout << "  --verbose               Show detailed progress information\n";
    std::cout << "  --debug                 Show debug output\n\n";
    
    std::cout << "EXAMPLES:\n";
    std::cout << "  # Analyze a single log file\n";
    std::cout << "  " << program_name << " app.log\n\n";
    
    std::cout << "  # Analyze all logs in a directory\n";
    std::cout << "  " << program_name << " logs/\n\n";
    
    std::cout << "  # Analyze multiple files and directories\n";
    std::cout << "  " << program_name << " app.log system.log logs/\n\n";
    
    std::cout << "  # Read from stdin\n";
    std::cout << "  cat app.log | " << program_name << " -\n\n";
    
    std::cout << "  # Generate only markdown output\n";
    std::cout << "  " << program_name << " --format md app.log\n\n";
    
    std::cout << "  # Analyze logs from a specific time range\n";
    std::cout << "  " << program_name << " --since 2024-03-01T00:00:00Z --until 2024-03-02T00:00:00Z app.log\n\n";
    
    std::cout << "  # Custom output directory with verbose logging\n";
    std::cout << "  " << program_name << " --verbose --out reports/ logs/\n\n";
}

void ArgParser::print_version() const {
    std::cout << "log-narrator version 0.1.0\n";
    std::cout << "Automated log analysis and narrative report generation\n";
}

bool ArgParser::parse_output_format(const std::string& format_str, OutputFormat& out) {
    std::string lower = format_str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "md" || lower == "markdown") {
        out = OutputFormat::MARKDOWN;
        return true;
    }
    if (lower == "json") {
        out = OutputFormat::JSON;
        return true;
    }
    if (lower == "csv") {
        out = OutputFormat::CSV;
        return true;
    }
    if (lower == "all") {
        out = OutputFormat::ALL;
        return true;
    }
    
    return false;
}

bool ArgParser::parse_verbosity(const std::string& verb_str, Verbosity& out) {
    std::string lower = verb_str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "quiet" || lower == "q") {
        out = Verbosity::QUIET;
        return true;
    }
    if (lower == "normal") {
        out = Verbosity::NORMAL;
        return true;
    }
    if (lower == "verbose" || lower == "v") {
        out = Verbosity::VERBOSE;
        return true;
    }
    if (lower == "debug" || lower == "d") {
        out = Verbosity::DEBUG;
        return true;
    }
    
    return false;
}

std::string to_string(OutputFormat fmt) {
    switch (fmt) {
        case OutputFormat::MARKDOWN: return "markdown";
        case OutputFormat::JSON: return "json";
        case OutputFormat::CSV: return "csv";
        case OutputFormat::ALL: return "all";
        default: return "unknown";
    }
}

std::string to_string(Verbosity verb) {
    switch (verb) {
        case Verbosity::QUIET: return "quiet";
        case Verbosity::NORMAL: return "normal";
        case Verbosity::VERBOSE: return "verbose";
        case Verbosity::DEBUG: return "debug";
        default: return "unknown";
    }
}

} // namespace logstory::cli
