/**
 * @file       rule_repository.hpp
 * @brief      In-memory automation rule store
 * @standard   C++23
 */

#ifndef IOT_AUTOMATION_RULE_REPOSITORY_HPP
#define IOT_AUTOMATION_RULE_REPOSITORY_HPP

#include "automation/rule_types.hpp"
#include "common/error.hpp"
#include <mutex>
#include <unordered_map>
#include <vector>

namespace iot::automation
{
    /**
     * Thread-safe, in-memory store for AutomationRule objects.
     *
     * Persistence (SQLite) will be added in a future iteration using the
     * automation_rules table already created by the database migration.
     * The interface is kept narrow so the swap is transparent to RuleEngine.
     */
    class RuleRepository
    {
        public:
        [[nodiscard]] Result<void> Add( AutomationRule rule );

        [[nodiscard]] Result<void> Update( const AutomationRule& rule );

        [[nodiscard]] Result<void> Remove( const std::string& ruleId );

        [[nodiscard]] Result<AutomationRule> GetById( const std::string& ruleId ) const;

        [[nodiscard]] Result<std::vector<AutomationRule>> GetAll() const;

        [[nodiscard]] Result<std::vector<AutomationRule>> GetEnabled() const;

        private:
        mutable std::mutex m_mutex;
        std::unordered_map<std::string, AutomationRule> m_rules;
    };

} // namespace iot::automation

#endif // IOT_AUTOMATION_RULE_REPOSITORY_HPP
