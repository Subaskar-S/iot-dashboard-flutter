/**
 * @file       condition_evaluator.hpp
 * @brief      Evaluates a Condition against a sensor value
 * @standard   C++23
 */

#ifndef IOT_AUTOMATION_CONDITION_EVALUATOR_HPP
#define IOT_AUTOMATION_CONDITION_EVALUATOR_HPP

#include "automation/rule_types.hpp"
#include "common/types.hpp"

namespace iot::automation
{
    /**
     * Pure function — no state, no dependencies.
     * Returns true when the reading satisfies the condition.
     */
    class ConditionEvaluator
    {
        public:
        /// Returns true if the reading matches the condition.
        [[nodiscard]] static bool Evaluate( const Condition& condition, const SensorReading& reading );

        /// Returns true only when ALL conditions are satisfied (AND logic).
        [[nodiscard]] static bool EvaluateAll( const std::vector<Condition>& conditions, const SensorReading& reading );
    };

} // namespace iot::automation

#endif // IOT_AUTOMATION_CONDITION_EVALUATOR_HPP
