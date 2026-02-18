#pragma once

#include "logstory/rules/rule.hpp"

namespace logstory::rules::builtin {

// Detects patterns where retries eventually lead to timeouts
class RetryToTimeoutRule : public Rule {
public:
    RetryToTimeoutRule() = default;
    
    std::string id() const override { return "retry-to-timeout"; }
    std::string name() const override { return "Retry Leading to Timeout"; }
    int priority() const override { return 70; }
    
    std::vector<Finding> evaluate(const RuleContext& context) override;
    
private:
    bool is_retry_event(const core::Event& event) const;
    bool is_timeout_event(const core::Event& event) const;
};

} // namespace logstory::rules::builtin
