#pragma once

#include "logstory/rules/rule.hpp"
#include <memory>
#include <vector>
#include <map>

namespace logstory::rules {

// Registry for managing rules
class RuleRegistry {
public:
    RuleRegistry() = default;
    
    // Register a rule
    void register_rule(std::unique_ptr<Rule> rule);
    
    // Execute all registered rules
    std::vector<Finding> evaluate_all(const RuleContext& context);
    
    // Get rule by ID
    Rule* get_rule(const std::string& rule_id);
    
    // Get all registered rules (sorted by priority)
    std::vector<Rule*> get_all_rules();
    
    // Clear all rules
    void clear();
    
    // Get count of registered rules
    size_t size() const { return rules_.size(); }
    
private:
    std::vector<std::unique_ptr<Rule>> rules_;
    std::map<std::string, Rule*> rule_map_;
    
    void sort_rules_by_priority();
};

} // namespace logstory::rules
