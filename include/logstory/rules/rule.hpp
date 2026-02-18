#pragma once

#include "logstory/rules/finding.hpp"
#include "logstory/core/event.hpp"
#include "logstory/analysis/stats.hpp"
#include "logstory/analysis/episode.hpp"
#include "logstory/analysis/anomaly_detector.hpp"
#include <vector>
#include <memory>

namespace logstory::rules {

// Context provided to rules during execution
struct RuleContext {
    const std::vector<core::Event>* events = nullptr;
    const analysis::Stats* stats = nullptr;
    const std::vector<analysis::Episode>* episodes = nullptr;
    const std::vector<analysis::Anomaly>* anomalies = nullptr;
    
    RuleContext() = default;
};

// Base interface for all rules
class Rule {
public:
    virtual ~Rule() = default;
    
    // Unique identifier for this rule
    virtual std::string id() const = 0;
    
    // Human-readable name
    virtual std::string name() const = 0;
    
    // Execute the rule and return findings
    virtual std::vector<Finding> evaluate(const RuleContext& context) = 0;
    
    // Optional: specify rule priority (higher = runs first)
    virtual int priority() const { return 0; }
};

} // namespace logstory::rules
