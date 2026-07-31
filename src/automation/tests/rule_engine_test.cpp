/**
 * @file       rule_engine_test.cpp
 * @brief      Unit tests for RuleEngine
 * @standard   C++23
 */

#include "automation/rule_engine.hpp"
#include <gtest/gtest.h>

namespace
{
    using namespace iot;
    using namespace iot::automation;

    static SensorReading MakeReading( std::string sensor, double value )
    {
        SensorReading r;
        r.m_deviceId = "dev-001";
        r.m_sensor = std::move( sensor );
        r.m_value = value;
        r.m_unit = "";
        r.m_timestamp = std::chrono::system_clock::now();
        return r;
    }

    static AutomationRule MakeRule( std::string id,
                                    std::string sensor,
                                    ComparisonOp op,
                                    double threshold,
                                    bool enabled = true )
    {
        AutomationRule r;
        r.m_id = std::move( id );
        r.m_name = "Rule " + r.m_id;
        r.m_enabled = enabled;
        r.m_conditions = { { std::move( sensor ), op, threshold } };
        r.m_actions = { { "fan-001", "turn_on", {} } };
        return r;
    }

    class RuleEngineTest : public ::testing::Test
    {
        protected:
        RuleRepository m_repo;
        RuleEngine m_engine{ m_repo };
    };

    // ── Evaluate ─────────────────────────────────────────────────────────

    TEST_F( RuleEngineTest, EvaluateMatchingRuleReturnsActions )
    {
        ASSERT_TRUE(
            m_engine.AddRule( MakeRule( "r-001", "temperature", ComparisonOp::GreaterThan, 30.0 ) ).has_value() );

        auto result = m_engine.Evaluate( MakeReading( "temperature", 35.0 ) );
        ASSERT_TRUE( result.has_value() );
        ASSERT_EQ( result->size(), 1u );
        EXPECT_EQ( result->front().m_deviceId, "fan-001" );
        EXPECT_EQ( result->front().m_command, "turn_on" );
    }

    TEST_F( RuleEngineTest, EvaluateNonMatchingRuleReturnsEmpty )
    {
        ASSERT_TRUE(
            m_engine.AddRule( MakeRule( "r-001", "temperature", ComparisonOp::GreaterThan, 30.0 ) ).has_value() );

        auto result = m_engine.Evaluate( MakeReading( "temperature", 20.0 ) );
        ASSERT_TRUE( result.has_value() );
        EXPECT_TRUE( result->empty() );
    }

    TEST_F( RuleEngineTest, DisabledRuleNotEvaluated )
    {
        ASSERT_TRUE(
            m_engine
                .AddRule( MakeRule( "r-001", "temperature", ComparisonOp::GreaterThan, 30.0, false /* disabled */ ) )
                .has_value() );

        auto result = m_engine.Evaluate( MakeReading( "temperature", 99.0 ) );
        ASSERT_TRUE( result.has_value() );
        EXPECT_TRUE( result->empty() );
    }

    TEST_F( RuleEngineTest, MultipleActionsAllReturned )
    {
        AutomationRule rule;
        rule.m_id = "r-multi";
        rule.m_name = "Multi Action";
        rule.m_enabled = true;
        rule.m_conditions = { { "temperature", ComparisonOp::GreaterThan, 30.0 } };
        rule.m_actions = {
            { "fan-001", "turn_on", {} },
            { "alert-system", "notify", { { "msg", "hot" } } },
        };
        ASSERT_TRUE( m_engine.AddRule( rule ).has_value() );

        auto result = m_engine.Evaluate( MakeReading( "temperature", 32.0 ) );
        ASSERT_TRUE( result.has_value() );
        EXPECT_EQ( result->size(), 2u );
    }

    TEST_F( RuleEngineTest, MultipleRulesMultipleMatchesAggregated )
    {
        ASSERT_TRUE(
            m_engine.AddRule( MakeRule( "r-001", "temperature", ComparisonOp::GreaterThan, 30.0 ) ).has_value() );
        ASSERT_TRUE(
            m_engine.AddRule( MakeRule( "r-002", "temperature", ComparisonOp::GreaterThan, 25.0 ) ).has_value() );

        auto result = m_engine.Evaluate( MakeReading( "temperature", 35.0 ) );
        ASSERT_TRUE( result.has_value() );
        EXPECT_EQ( result->size(), 2u ); // both rules match
    }

    // ── Enable / Disable ─────────────────────────────────────────────────

    TEST_F( RuleEngineTest, DisableRuleStopsEvaluation )
    {
        ASSERT_TRUE(
            m_engine.AddRule( MakeRule( "r-001", "temperature", ComparisonOp::GreaterThan, 30.0 ) ).has_value() );

        ASSERT_TRUE( m_engine.DisableRule( "r-001" ).has_value() );

        auto result = m_engine.Evaluate( MakeReading( "temperature", 99.0 ) );
        ASSERT_TRUE( result.has_value() );
        EXPECT_TRUE( result->empty() );
    }

    TEST_F( RuleEngineTest, EnableRuleResumesEvaluation )
    {
        ASSERT_TRUE( m_engine.AddRule( MakeRule( "r-001", "temperature", ComparisonOp::GreaterThan, 30.0, false ) )
                         .has_value() );

        ASSERT_TRUE( m_engine.EnableRule( "r-001" ).has_value() );

        auto result = m_engine.Evaluate( MakeReading( "temperature", 35.0 ) );
        ASSERT_TRUE( result.has_value() );
        EXPECT_EQ( result->size(), 1u );
    }

    TEST_F( RuleEngineTest, EnableMissingRuleReturnsError )
    {
        auto result = m_engine.EnableRule( "ghost" );
        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::DataNotFound );
    }

    // ── AddRule / RemoveRule ─────────────────────────────────────────────

    TEST_F( RuleEngineTest, RemoveRuleStopsEvaluation )
    {
        ASSERT_TRUE(
            m_engine.AddRule( MakeRule( "r-001", "temperature", ComparisonOp::GreaterThan, 30.0 ) ).has_value() );
        ASSERT_TRUE( m_engine.RemoveRule( "r-001" ).has_value() );

        auto result = m_engine.Evaluate( MakeReading( "temperature", 99.0 ) );
        ASSERT_TRUE( result.has_value() );
        EXPECT_TRUE( result->empty() );
    }

    TEST_F( RuleEngineTest, ListRulesReturnsAllRules )
    {
        ASSERT_TRUE(
            m_engine.AddRule( MakeRule( "r-001", "temperature", ComparisonOp::GreaterThan, 30.0 ) ).has_value() );
        ASSERT_TRUE( m_engine.AddRule( MakeRule( "r-002", "battery", ComparisonOp::LessThan, 20.0 ) ).has_value() );

        auto result = m_engine.ListRules();
        ASSERT_TRUE( result.has_value() );
        EXPECT_EQ( result->size(), 2u );
    }

    // ── JSON round-trip ──────────────────────────────────────────────────

    TEST_F( RuleEngineTest, ConditionJsonRoundTrip )
    {
        Condition c{ "temperature", ComparisonOp::GreaterThan, 30.0 };
        nlohmann::json j = c;

        Condition c2 = j.get<Condition>();
        EXPECT_EQ( c2.m_sensorType, c.m_sensorType );
        EXPECT_EQ( c2.m_op, c.m_op );
        EXPECT_DOUBLE_EQ( c2.m_threshold, c.m_threshold );
    }

} // namespace
