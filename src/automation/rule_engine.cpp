/**
 * @file       rule_engine.cpp
 * @brief      Evaluates enabled automation rules against sensor readings
 * @standard   C++23
 */

#include "automation/rule_engine.hpp"
#include "automation/condition_evaluator.hpp"
#include "common/logging.hpp"

namespace iot::automation
{
    RuleEngine::RuleEngine( RuleRepository& repository )
        : m_repository( repository )
        , m_logger( CreateLogger( "RuleEngine" ) )
    {
    }

    Result<std::vector<core::Action>> RuleEngine::Evaluate( const SensorReading& reading )
    {
        auto rulesResult = m_repository.GetEnabled();
        if ( !rulesResult )
        {
            return std::unexpected( rulesResult.error() );
        }

        std::vector<core::Action> fired;

        for ( const auto& rule : rulesResult.value() )
        {
            if ( ConditionEvaluator::EvaluateAll( rule.m_conditions, reading ) )
            {
                m_logger->info( "Rule '{}' triggered by sensor '{}' = {}", rule.m_name, reading.m_sensor,
                                reading.m_value );
                for ( const auto& action : rule.m_actions )
                {
                    fired.push_back( action );
                }
            }
        }

        return fired;
    }

    Result<void> RuleEngine::AddRule( AutomationRule rule )
    {
        return m_repository.Add( std::move( rule ) );
    }

    Result<void> RuleEngine::UpdateRule( const AutomationRule& rule )
    {
        return m_repository.Update( rule );
    }

    Result<void> RuleEngine::RemoveRule( const std::string& ruleId )
    {
        return m_repository.Remove( ruleId );
    }

    Result<void> RuleEngine::EnableRule( const std::string& ruleId )
    {
        auto result = m_repository.GetById( ruleId );
        if ( !result )
        {
            return std::unexpected( result.error() );
        }
        result->m_enabled = true;
        return m_repository.Update( result.value() );
    }

    Result<void> RuleEngine::DisableRule( const std::string& ruleId )
    {
        auto result = m_repository.GetById( ruleId );
        if ( !result )
        {
            return std::unexpected( result.error() );
        }
        result->m_enabled = false;
        return m_repository.Update( result.value() );
    }

    Result<AutomationRule> RuleEngine::GetRule( const std::string& ruleId )
    {
        return m_repository.GetById( ruleId );
    }

    Result<std::vector<AutomationRule>> RuleEngine::ListRules()
    {
        return m_repository.GetAll();
    }

} // namespace iot::automation
