#pragma once

#include "logstory/rules/rule.hpp"

namespace logstory::rules::builtin {

// Detects error spikes following deployment or configuration changes
class ErrorBurstAfterChangeRule : public Rule {
public:
    ErrorBurstAfterChangeRule() = default;
    
    std::string id() const override { return "error-burst-after-change"; }
    std::string name() const override { return "Error Burst After Deployment/Config Change"; }
    int priority() const override { return 85; }
    
    std::vector<Finding> evaluate(const RuleContext& context) override;
    
private:
    bool is_change_event(const core::Event& event) const;
    std::vector<size_t> find_change_events(const std::vector<core::Event>& events) const;
};

} // namespace logstory::rules::builtin
