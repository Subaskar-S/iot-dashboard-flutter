/**
 * @file       access_control.hpp
 * @brief      Role-based access control (RBAC) table
 * @standard   C++23
 */

#ifndef IOT_SECURITY_ACCESS_CONTROL_HPP
#define IOT_SECURITY_ACCESS_CONTROL_HPP

#include "core/interfaces/i_authentication_service.hpp"
#include <cstdint>
#include <string_view>

namespace iot::security
{
    enum class Permission : uint8_t
    {
        Read,
        Write,
        Delete,
        Admin
    };

    /**
     * Stateless RBAC table.
     *
     * Resources:  "devices", "sensors", "commands",
     *             "automation", "users", "metrics"
     *
     * Roles:
     *   Admin    — full access to everything
     *   Operator — read/write devices, sensors, commands, automation,
     *              metrics; no user management
     *   Viewer   — read-only on devices, sensors, metrics
     */
    class AccessControl
    {
        public:
        [[nodiscard]] static bool CanAccess( core::Role role, std::string_view resource, Permission permission );

        /// HTTP middleware helper: extract Bearer token, validate, check RBAC.
        [[nodiscard]] static std::string RoleToString( core::Role role );
        [[nodiscard]] static core::Role RoleFromString( std::string_view s );
    };

} // namespace iot::security

#endif // IOT_SECURITY_ACCESS_CONTROL_HPP
