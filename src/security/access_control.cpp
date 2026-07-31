/**
 * @file       access_control.cpp
 * @brief      RBAC table implementation
 * @standard   C++23
 */

#include "security/access_control.hpp"

namespace iot::security
{
    bool AccessControl::CanAccess( core::Role role, std::string_view resource, Permission permission )
    {
        using R = core::Role;
        using P = Permission;

        // Admin has full access to everything.
        if ( role == R::Admin )
        {
            return true;
        }

        // Operator: read/write on operational resources; no user management.
        if ( role == R::Operator )
        {
            if ( resource == "users" )
            {
                return false;
            }
            return permission == P::Read || permission == P::Write;
        }

        // Viewer: read-only on devices, sensors, metrics.
        if ( role == R::Viewer )
        {
            if ( resource == "devices" || resource == "sensors" || resource == "metrics" )
            {
                return permission == P::Read;
            }
            return false;
        }

        return false;
    }

    std::string AccessControl::RoleToString( core::Role role )
    {
        switch ( role )
        {
            case core::Role::Admin:
                return "Admin";
            case core::Role::Operator:
                return "Operator";
            case core::Role::Viewer:
                return "Viewer";
        }
        return "Viewer";
    }

    core::Role AccessControl::RoleFromString( std::string_view s )
    {
        if ( s == "Admin" )
            return core::Role::Admin;
        if ( s == "Operator" )
            return core::Role::Operator;
        return core::Role::Viewer;
    }

} // namespace iot::security
