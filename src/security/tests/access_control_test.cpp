/**
 * @file       access_control_test.cpp
 * @standard   C++23
 */

#include "security/access_control.hpp"
#include <gtest/gtest.h>

namespace
{
    using namespace iot::security;
    using namespace iot::core;

    // ── Admin ───────────────────────────────────────────────────────────

    TEST( AccessControlTest, AdminCanDoEverything )
    {
        for ( auto resource : { "devices", "sensors", "commands", "automation", "users", "metrics" } )
        {
            for ( auto perm : { Permission::Read, Permission::Write, Permission::Delete, Permission::Admin } )
            {
                EXPECT_TRUE( AccessControl::CanAccess( Role::Admin, resource, perm ) ) << "Admin denied " << resource;
            }
        }
    }

    // ── Operator ─────────────────────────────────────────────────────────

    TEST( AccessControlTest, OperatorCanReadWriteDevices )
    {
        EXPECT_TRUE( AccessControl::CanAccess( Role::Operator, "devices", Permission::Read ) );
        EXPECT_TRUE( AccessControl::CanAccess( Role::Operator, "devices", Permission::Write ) );
    }

    TEST( AccessControlTest, OperatorCannotDeleteOrAdmin )
    {
        EXPECT_FALSE( AccessControl::CanAccess( Role::Operator, "devices", Permission::Delete ) );
        EXPECT_FALSE( AccessControl::CanAccess( Role::Operator, "devices", Permission::Admin ) );
    }

    TEST( AccessControlTest, OperatorCannotAccessUsers )
    {
        EXPECT_FALSE( AccessControl::CanAccess( Role::Operator, "users", Permission::Read ) );
        EXPECT_FALSE( AccessControl::CanAccess( Role::Operator, "users", Permission::Write ) );
    }

    TEST( AccessControlTest, OperatorCanAccessMetrics )
    {
        EXPECT_TRUE( AccessControl::CanAccess( Role::Operator, "metrics", Permission::Read ) );
        EXPECT_TRUE( AccessControl::CanAccess( Role::Operator, "metrics", Permission::Write ) );
    }

    // ── Viewer ───────────────────────────────────────────────────────────

    TEST( AccessControlTest, ViewerCanReadAllowedResources )
    {
        EXPECT_TRUE( AccessControl::CanAccess( Role::Viewer, "devices", Permission::Read ) );
        EXPECT_TRUE( AccessControl::CanAccess( Role::Viewer, "sensors", Permission::Read ) );
        EXPECT_TRUE( AccessControl::CanAccess( Role::Viewer, "metrics", Permission::Read ) );
    }

    TEST( AccessControlTest, ViewerCannotWrite )
    {
        EXPECT_FALSE( AccessControl::CanAccess( Role::Viewer, "devices", Permission::Write ) );
        EXPECT_FALSE( AccessControl::CanAccess( Role::Viewer, "sensors", Permission::Write ) );
    }

    TEST( AccessControlTest, ViewerCannotAccessCommandsOrAutomation )
    {
        EXPECT_FALSE( AccessControl::CanAccess( Role::Viewer, "commands", Permission::Read ) );
        EXPECT_FALSE( AccessControl::CanAccess( Role::Viewer, "automation", Permission::Read ) );
    }

    // ── Round-trip helpers ───────────────────────────────────────────────

    TEST( AccessControlTest, RoleStringRoundTrip )
    {
        for ( auto role : { Role::Admin, Role::Operator, Role::Viewer } )
        {
            EXPECT_EQ( AccessControl::RoleFromString( AccessControl::RoleToString( role ) ), role );
        }
    }

    TEST( AccessControlTest, UnknownRoleDefaultsToViewer )
    {
        EXPECT_EQ( AccessControl::RoleFromString( "Unknown" ), Role::Viewer );
    }

} // namespace
