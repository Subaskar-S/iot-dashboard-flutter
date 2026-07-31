/**
 * @file       rule_repository.cpp
 * @brief      In-memory rule store implementation
 * @standard   C++23
 */

#include "automation/rule_repository.hpp"

namespace iot::automation
{
    Result<void> RuleRepository::Add( AutomationRule rule )
    {
        std::lock_guard lock( m_mutex );
        if ( m_rules.contains( rule.m_id ) )
        {
            return std::unexpected( Error::InvalidInput );
        }
        m_rules.emplace( rule.m_id, std::move( rule ) );
        return {};
    }

    Result<void> RuleRepository::Update( const AutomationRule& rule )
    {
        std::lock_guard lock( m_mutex );
        auto it = m_rules.find( rule.m_id );
        if ( it == m_rules.end() )
        {
            return std::unexpected( Error::DataNotFound );
        }
        it->second = rule;
        return {};
    }

    Result<void> RuleRepository::Remove( const std::string& ruleId )
    {
        std::lock_guard lock( m_mutex );
        if ( m_rules.erase( ruleId ) == 0 )
        {
            return std::unexpected( Error::DataNotFound );
        }
        return {};
    }

    Result<AutomationRule> RuleRepository::GetById( const std::string& ruleId ) const
    {
        std::lock_guard lock( m_mutex );
        auto it = m_rules.find( ruleId );
        if ( it == m_rules.end() )
        {
            return std::unexpected( Error::DataNotFound );
        }
        return it->second;
    }

    Result<std::vector<AutomationRule>> RuleRepository::GetAll() const
    {
        std::lock_guard lock( m_mutex );
        std::vector<AutomationRule> result;
        result.reserve( m_rules.size() );
        for ( const auto& [id, rule] : m_rules )
        {
            result.push_back( rule );
        }
        return result;
    }

    Result<std::vector<AutomationRule>> RuleRepository::GetEnabled() const
    {
        std::lock_guard lock( m_mutex );
        std::vector<AutomationRule> result;
        for ( const auto& [id, rule] : m_rules )
        {
            if ( rule.m_enabled )
            {
                result.push_back( rule );
            }
        }
        return result;
    }

} // namespace iot::automation
