#include "logstory/rules/rule_registry.hpp"
#include <algorithm>

namespace logstory::rules {

void RuleRegistry::register_rule(std::unique_ptr<Rule> rule) {
    if (!rule) {
        return;
    }
    
    Rule* raw_ptr = rule.get();
    std::string rule_id = rule->id();
    
    rules_.push_back(std::move(rule));
    rule_map_[rule_id] = raw_ptr;
    
    // Sort after each registration to maintain priority order
    sort_rules_by_priority();
}

std::vector<Finding> RuleRegistry::evaluate_all(const RuleContext& context) {
    std::vector<Finding> all_findings;
    
    for (const auto& rule : rules_) {
        std::vector<Finding> findings = rule->evaluate(context);
        all_findings.insert(all_findings.end(), findings.begin(), findings.end());
    }
    
    return all_findings;
}

Rule* RuleRegistry::get_rule(const std::string& rule_id) {
    auto it = rule_map_.find(rule_id);
    if (it != rule_map_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<Rule*> RuleRegistry::get_all_rules() {
    std::vector<Rule*> result;
    result.reserve(rules_.size());
    
    for (const auto& rule : rules_) {
        result.push_back(rule.get());
    }
    
    return result;
}

void RuleRegistry::clear() {
    rules_.clear();
    rule_map_.clear();
}

void RuleRegistry::sort_rules_by_priority() {
    std::stable_sort(rules_.begin(), rules_.end(),
        [](const std::unique_ptr<Rule>& a, const std::unique_ptr<Rule>& b) {
            return a->priority() > b->priority();
        });
    
    // Rebuild map after sort
    rule_map_.clear();
    for (const auto& rule : rules_) {
        rule_map_[rule->id()] = rule.get();
    }
}

} // namespace logstory::rules
