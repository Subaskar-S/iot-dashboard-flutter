/**
 * @file       rule_types.hpp
 * @brief      Automation rule domain types
 * @standard   C++23
 */

#ifndef IOT_AUTOMATION_RULE_TYPES_HPP
#define IOT_AUTOMATION_RULE_TYPES_HPP

#include "core/interfaces/i_rule_evaluator.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace iot::automation
{
    enum class ComparisonOp : uint8_t
    {
        GreaterThan,
        LessThan,
        Equal,
        NotEqual,
        GreaterOrEqual,
        LessOrEqual
    };

    inline ComparisonOp OpFromString( std::string_view s )
    {
        if ( s == ">" )
            return ComparisonOp::GreaterThan;
        if ( s == "<" )
            return ComparisonOp::LessThan;
        if ( s == "==" )
            return ComparisonOp::Equal;
        if ( s == "!=" )
            return ComparisonOp::NotEqual;
        if ( s == ">=" )
            return ComparisonOp::GreaterOrEqual;
        if ( s == "<=" )
            return ComparisonOp::LessOrEqual;
        return ComparisonOp::Equal;
    }

    inline std::string OpToString( ComparisonOp op )
    {
        switch ( op )
        {
            case ComparisonOp::GreaterThan:
                return ">";
            case ComparisonOp::LessThan:
                return "<";
            case ComparisonOp::Equal:
                return "==";
            case ComparisonOp::NotEqual:
                return "!=";
            case ComparisonOp::GreaterOrEqual:
                return ">=";
            case ComparisonOp::LessOrEqual:
                return "<=";
        }
        return "==";
    }

    /// A single condition — e.g. "temperature > 30.0".
    struct Condition
    {
        std::string m_sensorType;
        ComparisonOp m_op = ComparisonOp::GreaterThan;
        double m_threshold = 0.0;
    };

    /// One automation rule (all conditions must match — AND logic).
    struct AutomationRule
    {
        std::string m_id;
        std::string m_name;
        bool m_enabled = true;
        std::vector<Condition> m_conditions; // AND-combined
        std::vector<core::Action> m_actions;
        std::string m_createdBy;
    };

    // ── JSON serialisation helpers ────────────────────────────────────────

    inline void to_json( nlohmann::json& j, const Condition& c )
    {
        j = { { "sensor_type", c.m_sensorType }, { "operator", OpToString( c.m_op ) }, { "threshold", c.m_threshold } };
    }

    inline void from_json( const nlohmann::json& j, Condition& c )
    {
        c.m_sensorType = j.at( "sensor_type" ).get<std::string>();
        c.m_op = OpFromString( j.at( "operator" ).get<std::string>() );
        c.m_threshold = j.at( "threshold" ).get<double>();
    }

    inline void to_json( nlohmann::json& j, const core::Action& a )
    {
        j = { { "device_id", a.m_deviceId }, { "command", a.m_command }, { "parameters", a.m_parameters } };
    }

    inline void from_json( const nlohmann::json& j, core::Action& a )
    {
        a.m_deviceId = j.at( "device_id" ).get<std::string>();
        a.m_command = j.at( "command" ).get<std::string>();
        a.m_parameters = j.value( "parameters", nlohmann::json::object() );
    }

} // namespace iot::automation

#endif // IOT_AUTOMATION_RULE_TYPES_HPP
