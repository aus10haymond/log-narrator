#include <iostream>
#include "logstory/cli/args.hpp"
#include "logstory/cli/app.hpp"
#include "logstory/core/logger.hpp"

using namespace logstory;

int main(int argc, char** argv) {
    // Parse arguments
    cli::ArgParser parser;
    cli::Args args = parser.parse(argc, argv);
    
    // Handle help
    if (args.show_help) {
        parser.print_help(argv[0]);
        return 0;
    }
    
    // Handle version
    if (args.show_version) {
        parser.print_version();
        return 0;
    }
    
    // Validate arguments
    if (!args.is_valid()) {
        std::cerr << "Error: " << args.get_error_message() << "\n\n";
        parser.print_help(argv[0]);
        return 1;
    }
    
    // Run application
    cli::App app(args);
    auto status = app.run();
    
    if (!status.ok()) {
        core::g_logger.error(status.message);
        return 1;
    }
    
    return 0;
}
