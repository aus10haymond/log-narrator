#pragma once

#include "logstory/rules/rule.hpp"

namespace logstory::rules::builtin {

// Detects crash/restart loops in the logs
class CrashLoopRule : public Rule {
public:
    CrashLoopRule() = default;
    
    std::string id() const override { return "crash-loop"; }
    std::string name() const override { return "Crash Loop Detection"; }
    int priority() const override { return 90; } // High priority
    
    std::vector<Finding> evaluate(const RuleContext& context) override;
};

} // namespace logstory::rules::builtin
