/**
 * @file       rule_engine.hpp
 * @brief      Evaluates automation rules and dispatches actions
 * @standard   C++23
 */

#ifndef IOT_AUTOMATION_RULE_ENGINE_HPP
#define IOT_AUTOMATION_RULE_ENGINE_HPP

#include "automation/rule_repository.hpp"
#include "automation/rule_types.hpp"
#include "core/interfaces/i_rule_evaluator.hpp"
#include <memory>
#include <spdlog/spdlog.h>

namespace iot::automation
{
    /**
     * Implements IRuleEvaluator — iterates all enabled rules and returns
     * the actions whose conditions are fully satisfied by a sensor reading.
     *
     * Thread-safe: delegates locking to RuleRepository.
     */
    class RuleEngine final : public core::IRuleEvaluator
    {
        public:
        explicit RuleEngine( RuleRepository& repository );

        // IRuleEvaluator
        [[nodiscard]] Result<std::vector<core::Action>> Evaluate( const SensorReading& reading ) override;

        // Rule management
        [[nodiscard]] Result<void> AddRule( AutomationRule rule );
        [[nodiscard]] Result<void> UpdateRule( const AutomationRule& rule );
        [[nodiscard]] Result<void> RemoveRule( const std::string& ruleId );
        [[nodiscard]] Result<void> EnableRule( const std::string& ruleId );
        [[nodiscard]] Result<void> DisableRule( const std::string& ruleId );

        [[nodiscard]] Result<AutomationRule> GetRule( const std::string& ruleId );
        [[nodiscard]] Result<std::vector<AutomationRule>> ListRules();

        private:
        RuleRepository& m_repository;
        std::shared_ptr<spdlog::logger> m_logger;
    };

} // namespace iot::automation

#endif // IOT_AUTOMATION_RULE_ENGINE_HPP
