#pragma once

#include "logstory/cli/args.hpp"
#include "logstory/core/event.hpp"
#include "logstory/core/error.hpp"
#include "logstory/analysis/stats.hpp"
#include "logstory/analysis/episode.hpp"
#include "logstory/rules/finding.hpp"
#include "logstory/narrative/report.hpp"
#include <vector>
#include <string>

namespace logstory::cli {

/// Main application class that runs the analysis pipeline
class App {
public:
    explicit App(const Args& args);
    
    /// Run the full analysis pipeline
    core::Status run();
    
private:
    Args args_;
    
    // Pipeline steps
    core::Status ingest(std::vector<core::Event>& out_events);
    core::Status analyze(const std::vector<core::Event>& events,
                        analysis::Stats& out_stats,
                        std::vector<analysis::Episode>& out_episodes,
                        std::vector<rules::Finding>& out_findings);
    core::Status generate_reports(const std::vector<core::Event>& events,
                                  const analysis::Stats& stats,
                                  const std::vector<analysis::Episode>& episodes,
                                  const std::vector<rules::Finding>& findings);
    
    // Input helpers
    core::Status read_stdin(std::vector<core::Event>& out_events);
    core::Status read_file(const std::string& path, std::vector<core::Event>& out_events);
    core::Status read_directory(const std::string& path, std::vector<core::Event>& out_events);
    
    // Output helpers
    core::Status write_markdown(const narrative::Report& report, const std::string& path);
    core::Status write_json(const narrative::Report& report, const std::string& path);
    core::Status write_csv(const narrative::Report& report, const std::string& path);
    
    // Utility
    bool should_write_format(OutputFormat fmt) const;
    core::Status create_output_directory();
};

} // namespace logstory::cli
