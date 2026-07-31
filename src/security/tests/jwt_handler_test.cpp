/**
 * @file       jwt_handler_test.cpp
 * @standard   C++23
 */

#include "security/jwt_handler.hpp"
#include <algorithm>
#include <gtest/gtest.h>
#include <thread>

namespace
{
    using namespace iot;
    using namespace iot::security;

    static core::UserClaims AdminClaims()
    {
        return { "user-001", "admin", core::Role::Admin };
    }

    TEST( JwtHandlerTest, GenerateProducesThreePartToken )
    {
        JwtHandler handler( "secret" );
        auto result = handler.Generate( AdminClaims(), std::chrono::seconds( 3600 ) );
        ASSERT_TRUE( result.has_value() );
        auto& tok = result.value();
        EXPECT_EQ( std::count( tok.begin(), tok.end(), '.' ), 2 );
    }

    TEST( JwtHandlerTest, VerifyValidTokenReturnsClaims )
    {
        JwtHandler handler( "secret" );
        auto token = handler.Generate( AdminClaims(), std::chrono::seconds( 3600 ) );
        ASSERT_TRUE( token.has_value() );

        auto claims = handler.Verify( token.value() );
        ASSERT_TRUE( claims.has_value() );
        EXPECT_EQ( claims->m_userId, "user-001" );
        EXPECT_EQ( claims->m_username, "admin" );
        EXPECT_EQ( claims->m_role, core::Role::Admin );
    }

    TEST( JwtHandlerTest, VerifyWrongSecretFails )
    {
        JwtHandler signer( "correct-secret" );
        JwtHandler verifier( "wrong-secret" );

        auto token = signer.Generate( AdminClaims(), std::chrono::seconds( 3600 ) );
        ASSERT_TRUE( token.has_value() );

        auto result = verifier.Verify( token.value() );
        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::AuthorizationFailed );
    }

    TEST( JwtHandlerTest, VerifyExpiredTokenFails )
    {
        JwtHandler handler( "secret" );
        auto token = handler.Generate( AdminClaims(), std::chrono::seconds( 1 ) );
        ASSERT_TRUE( token.has_value() );

        std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

        auto result = handler.Verify( token.value() );
        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::TokenExpired );
    }

    TEST( JwtHandlerTest, VerifyMalformedTokenFails )
    {
        JwtHandler handler( "secret" );
        auto result = handler.Verify( "not.a.valid.jwt.token" );
        ASSERT_FALSE( result.has_value() );
    }

    TEST( JwtHandlerTest, VerifyEmptyTokenFails )
    {
        JwtHandler handler( "secret" );
        auto result = handler.Verify( "" );
        ASSERT_FALSE( result.has_value() );
    }

    TEST( JwtHandlerTest, AllRolesPreservedInToken )
    {
        JwtHandler handler( "secret" );

        for ( auto role : { core::Role::Admin, core::Role::Operator, core::Role::Viewer } )
        {
            core::UserClaims claims{ "u-001", "user", role };
            auto token = handler.Generate( claims, std::chrono::seconds( 3600 ) );
            ASSERT_TRUE( token.has_value() );

            auto verified = handler.Verify( token.value() );
            ASSERT_TRUE( verified.has_value() );
            EXPECT_EQ( verified->m_role, role );
        }
    }

} // namespace
