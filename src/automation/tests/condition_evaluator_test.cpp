/**
 * @file       condition_evaluator_test.cpp
 * @brief      Unit tests for ConditionEvaluator
 * @standard   C++23
 */

#include "automation/condition_evaluator.hpp"
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
        r.m_unit = "unit";
        r.m_timestamp = std::chrono::system_clock::now();
        return r;
    }

    static Condition MakeCond( std::string sensor, ComparisonOp op, double threshold )
    {
        return { .m_sensorType = std::move( sensor ), .m_op = op, .m_threshold = threshold };
    }

    // ── Single condition ─────────────────────────────────────────────────

    TEST( ConditionEvaluatorTest, GreaterThanTrue )
    {
        EXPECT_TRUE( ConditionEvaluator::Evaluate( MakeCond( "temperature", ComparisonOp::GreaterThan, 30.0 ),
                                                   MakeReading( "temperature", 32.0 ) ) );
    }

    TEST( ConditionEvaluatorTest, GreaterThanFalse )
    {
        EXPECT_FALSE( ConditionEvaluator::Evaluate( MakeCond( "temperature", ComparisonOp::GreaterThan, 30.0 ),
                                                    MakeReading( "temperature", 28.0 ) ) );
    }

    TEST( ConditionEvaluatorTest, LessThanTrue )
    {
        EXPECT_TRUE( ConditionEvaluator::Evaluate( MakeCond( "battery", ComparisonOp::LessThan, 20.0 ),
                                                   MakeReading( "battery", 15.0 ) ) );
    }

    TEST( ConditionEvaluatorTest, EqualTrue )
    {
        EXPECT_TRUE( ConditionEvaluator::Evaluate( MakeCond( "switch", ComparisonOp::Equal, 1.0 ),
                                                   MakeReading( "switch", 1.0 ) ) );
    }

    TEST( ConditionEvaluatorTest, NotEqualTrue )
    {
        EXPECT_TRUE( ConditionEvaluator::Evaluate( MakeCond( "switch", ComparisonOp::NotEqual, 0.0 ),
                                                   MakeReading( "switch", 1.0 ) ) );
    }

    TEST( ConditionEvaluatorTest, GreaterOrEqualOnBoundary )
    {
        EXPECT_TRUE( ConditionEvaluator::Evaluate( MakeCond( "humidity", ComparisonOp::GreaterOrEqual, 80.0 ),
                                                   MakeReading( "humidity", 80.0 ) ) );
    }

    TEST( ConditionEvaluatorTest, LessOrEqualOnBoundary )
    {
        EXPECT_TRUE( ConditionEvaluator::Evaluate( MakeCond( "humidity", ComparisonOp::LessOrEqual, 80.0 ),
                                                   MakeReading( "humidity", 80.0 ) ) );
    }

    TEST( ConditionEvaluatorTest, WrongSensorTypeReturnsFalse )
    {
        EXPECT_FALSE( ConditionEvaluator::Evaluate( MakeCond( "temperature", ComparisonOp::GreaterThan, 30.0 ),
                                                    MakeReading( "humidity", 90.0 ) ) );
    }

    // ── EvaluateAll ──────────────────────────────────────────────────────

    TEST( ConditionEvaluatorTest, AllConditionsMetReturnsTrue )
    {
        // Both conditions share the same sensor type — reading satisfies both.
        std::vector<Condition> conds = {
            MakeCond( "temperature", ComparisonOp::GreaterThan, 25.0 ),
            MakeCond( "temperature", ComparisonOp::LessThan, 40.0 ),
        };
        EXPECT_TRUE( ConditionEvaluator::EvaluateAll( conds, MakeReading( "temperature", 32.0 ) ) );
    }

    TEST( ConditionEvaluatorTest, OneConditionFailsReturnsFalse )
    {
        std::vector<Condition> conds = {
            MakeCond( "temperature", ComparisonOp::GreaterThan, 25.0 ),
            MakeCond( "temperature", ComparisonOp::LessThan, 30.0 ),
        };
        // 32 > 25 true, 32 < 30 false → overall false
        EXPECT_FALSE( ConditionEvaluator::EvaluateAll( conds, MakeReading( "temperature", 32.0 ) ) );
    }

    TEST( ConditionEvaluatorTest, EmptyConditionsReturnsFalse )
    {
        EXPECT_FALSE( ConditionEvaluator::EvaluateAll( {}, MakeReading( "temperature", 32.0 ) ) );
    }

    // ── OpFromString / OpToString round-trip ─────────────────────────────

    TEST( ConditionEvaluatorTest, OpRoundTrip )
    {
        for ( auto op : { ComparisonOp::GreaterThan, ComparisonOp::LessThan, ComparisonOp::Equal,
                          ComparisonOp::NotEqual, ComparisonOp::GreaterOrEqual, ComparisonOp::LessOrEqual } )
        {
            EXPECT_EQ( OpFromString( OpToString( op ) ), op );
        }
    }

} // namespace
