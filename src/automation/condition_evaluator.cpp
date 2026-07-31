/**
 * @file       condition_evaluator.cpp
 * @brief      Condition evaluation logic
 * @standard   C++23
 */

#include "automation/condition_evaluator.hpp"
#include <algorithm>

namespace iot::automation
{
    bool ConditionEvaluator::Evaluate( const Condition& condition, const SensorReading& reading )
    {
        // Condition only applies when sensor types match.
        if ( reading.m_sensor != condition.m_sensorType )
        {
            return false;
        }

        const double v = reading.m_value;
        const double t = condition.m_threshold;

        switch ( condition.m_op )
        {
            case ComparisonOp::GreaterThan:
                return v > t;
            case ComparisonOp::LessThan:
                return v < t;
            case ComparisonOp::Equal:
                return v == t;
            case ComparisonOp::NotEqual:
                return v != t;
            case ComparisonOp::GreaterOrEqual:
                return v >= t;
            case ComparisonOp::LessOrEqual:
                return v <= t;
        }

        return false;
    }

    bool ConditionEvaluator::EvaluateAll( const std::vector<Condition>& conditions, const SensorReading& reading )
    {
        if ( conditions.empty() )
        {
            return false; // a rule with no conditions never fires
        }

        return std::ranges::all_of( conditions,
                                    [&reading]( const Condition& c )
                                    {
                                        return Evaluate( c, reading );
                                    } );
    }

} // namespace iot::automation
