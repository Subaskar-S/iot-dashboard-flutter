/**
 * @file       rule_repository_test.cpp
 * @brief      Unit tests for RuleRepository
 * @standard   C++23
 */

#include "automation/rule_repository.hpp"
#include <gtest/gtest.h>

namespace
{
    using namespace iot;
    using namespace iot::automation;

    static AutomationRule MakeRule( std::string id, bool enabled = true )
    {
        AutomationRule r;
        r.m_id = std::move( id );
        r.m_name = "Test Rule";
        r.m_enabled = enabled;
        r.m_conditions = { { "temperature", ComparisonOp::GreaterThan, 30.0 } };
        r.m_actions = { { "fan-001", "turn_on", {} } };
        return r;
    }

    TEST( RuleRepositoryTest, AddAndGetById )
    {
        RuleRepository repo;
        ASSERT_TRUE( repo.Add( MakeRule( "rule-001" ) ).has_value() );

        auto result = repo.GetById( "rule-001" );
        ASSERT_TRUE( result.has_value() );
        EXPECT_EQ( result->m_id, "rule-001" );
    }

    TEST( RuleRepositoryTest, AddDuplicateReturnsError )
    {
        RuleRepository repo;
        ASSERT_TRUE( repo.Add( MakeRule( "rule-001" ) ).has_value() );

        auto result = repo.Add( MakeRule( "rule-001" ) );
        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::InvalidInput );
    }

    TEST( RuleRepositoryTest, GetMissingReturnsError )
    {
        RuleRepository repo;
        auto result = repo.GetById( "ghost" );
        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::DataNotFound );
    }

    TEST( RuleRepositoryTest, UpdateExisting )
    {
        RuleRepository repo;
        ASSERT_TRUE( repo.Add( MakeRule( "rule-001" ) ).has_value() );

        auto rule = repo.GetById( "rule-001" ).value();
        rule.m_name = "Updated Name";
        ASSERT_TRUE( repo.Update( rule ).has_value() );

        EXPECT_EQ( repo.GetById( "rule-001" )->m_name, "Updated Name" );
    }

    TEST( RuleRepositoryTest, UpdateMissingReturnsError )
    {
        RuleRepository repo;
        auto result = repo.Update( MakeRule( "ghost" ) );
        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::DataNotFound );
    }

    TEST( RuleRepositoryTest, RemoveExisting )
    {
        RuleRepository repo;
        ASSERT_TRUE( repo.Add( MakeRule( "rule-001" ) ).has_value() );
        ASSERT_TRUE( repo.Remove( "rule-001" ).has_value() );

        EXPECT_FALSE( repo.GetById( "rule-001" ).has_value() );
    }

    TEST( RuleRepositoryTest, RemoveMissingReturnsError )
    {
        RuleRepository repo;
        auto result = repo.Remove( "ghost" );
        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::DataNotFound );
    }

    TEST( RuleRepositoryTest, GetAllReturnsAllRules )
    {
        RuleRepository repo;
        ASSERT_TRUE( repo.Add( MakeRule( "r-001" ) ).has_value() );
        ASSERT_TRUE( repo.Add( MakeRule( "r-002" ) ).has_value() );

        auto result = repo.GetAll();
        ASSERT_TRUE( result.has_value() );
        EXPECT_EQ( result->size(), 2u );
    }

    TEST( RuleRepositoryTest, GetEnabledFiltersDisabled )
    {
        RuleRepository repo;
        ASSERT_TRUE( repo.Add( MakeRule( "r-enabled", true ) ).has_value() );
        ASSERT_TRUE( repo.Add( MakeRule( "r-disabled", false ) ).has_value() );

        auto result = repo.GetEnabled();
        ASSERT_TRUE( result.has_value() );
        ASSERT_EQ( result->size(), 1u );
        EXPECT_EQ( result->front().m_id, "r-enabled" );
    }

} // namespace
