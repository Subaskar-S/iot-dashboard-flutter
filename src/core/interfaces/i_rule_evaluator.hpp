/**
 * @file       i_rule_evaluator.hpp
 * @brief      Port for automation rule evaluation
 * @standard   C++23
 */

#ifndef IOT_CORE_I_RULE_EVALUATOR_HPP
#define IOT_CORE_I_RULE_EVALUATOR_HPP

#include "common/error.hpp"
#include "common/types.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace iot::core
{
    /// A single action to perform when a rule's conditions are satisfied.
    struct Action
    {
        std::string m_deviceId;
        std::string m_command;
        nlohmann::json m_parameters;
    };

    /**
     * Abstract automation rule evaluator.
     *
     * The concrete rule engine implementation lives in the automation
     * module. This interface lets the network layer (which receives
     * sensor readings) trigger evaluation without depending on rule
     * storage or matching logic directly.
     */
    class IRuleEvaluator
    {
        public:
        virtual ~IRuleEvaluator() = default;

        /// Evaluate all enabled rules against a single sensor reading,
        /// returning the actions to execute (empty if none matched).
        [[nodiscard]] virtual Result<std::vector<Action>> Evaluate( const SensorReading& reading ) = 0;
    };

} // namespace iot::core

#endif // IOT_CORE_I_RULE_EVALUATOR_HPP
