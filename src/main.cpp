#include <iostream>
#include <filesystem>
#include "logstory/io/file_reader.hpp"
#include "logstory/io/dir_scanner.hpp"
#include "logstory/io/stdin_reader.hpp"
#include "logstory/core/source_ref.hpp"

using namespace logstory;

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [file|directory|-]\n";
    std::cout << "\n";
    std::cout << "Phase 1 Demo - Log Ingestion\n";
    std::cout << "  file       - Read a single log file\n";
    std::cout << "  directory  - Recursively scan directory for .log, .txt, .jsonl files\n";
    std::cout << "  -          - Read from stdin\n";
    std::cout << "\n";
    std::cout << "Example: " << prog_name << " logs/app.log\n";
    std::cout << "Example: " << prog_name << " logs/\n";
    std::cout << "Example: type logs.txt | " << prog_name << " -\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::string input = argv[1];
    std::vector<io::RawLine> all_lines;

    // Handle stdin
    if (input == "-") {
        std::cout << "Reading from stdin...\n";
        io::StdinReader reader;
        core::Status status = reader.read(all_lines);
        if (!status.ok()) {
            std::cerr << "Error: " << status.message << "\n";
            return 1;
        }
    }
    // Handle directory
    else if (std::filesystem::is_directory(input)) {
        std::cout << "Scanning directory: " << input << "\n";
        
        io::DirScanner scanner;
        std::vector<std::string> files;
        core::Status status = scanner.scan(input, files);
        
        if (!status.ok()) {
            std::cerr << "Error: " << status.message << "\n";
            return 1;
        }
        
        std::cout << "Found " << files.size() << " file(s)\n";
        
        // Read each file
        io::FileReader file_reader;
        for (const auto& file : files) {
            std::cout << "  Reading: " << file << "\n";
            std::vector<io::RawLine> lines;
            status = file_reader.read(file, lines);
            if (!status.ok()) {
                std::cerr << "  Warning: " << status.message << "\n";
                continue;
            }
            all_lines.insert(all_lines.end(), lines.begin(), lines.end());
        }
    }
    // Handle single file
    else {
        std::cout << "Reading file: " << input << "\n";
        io::FileReader reader;
        core::Status status = reader.read(input, all_lines);
        if (!status.ok()) {
            std::cerr << "Error: " << status.message << "\n";
            return 1;
        }
    }

    // Display results
    std::cout << "\n=== Ingestion Complete ===";
    std::cout << "\nTotal lines read: " << all_lines.size() << "\n";
    
    if (!all_lines.empty()) {
        std::cout << "\nFirst 5 lines:\n";
        for (size_t i = 0; i < std::min(size_t(5), all_lines.size()); ++i) {
            core::SourceRef ref(all_lines[i].source_path, all_lines[i].line_no);
            std::cout << "  " << ref.to_string() << " | " << all_lines[i].text << "\n";
        }
        
        if (all_lines.size() > 5) {
            std::cout << "  ...\n";
            std::cout << "\nLast line:\n";
            const auto& last = all_lines.back();
            core::SourceRef ref(last.source_path, last.line_no);
            std::cout << "  " << ref.to_string() << " | " << last.text << "\n";
        }
    }

    return 0;
}
