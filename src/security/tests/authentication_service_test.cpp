/**
 * @file       authentication_service_test.cpp
 * @standard   C++23
 */

#include "security/authentication_service.hpp"
#include <gtest/gtest.h>

namespace
{
    using namespace iot;
    using namespace iot::security;

    static SecurityConfig FastConfig()
    {
        SecurityConfig cfg;
        cfg.m_jwtSecret = "test-secret";
        cfg.m_accessTokenExpiry = std::chrono::seconds( 3600 );
        cfg.m_refreshTokenExpiry = std::chrono::seconds( 7200 );
        cfg.m_pbkdf2Iterations = 1000; // fast for tests
        return cfg;
    }

    class AuthServiceTest : public ::testing::Test
    {
        protected:
        void SetUp() override
        {
            m_service = std::make_unique<AuthenticationService>( FastConfig() );
            ASSERT_TRUE(
                m_service->RegisterUser( "user-001", "admin", "correct-password", core::Role::Admin ).has_value() );
        }

        std::unique_ptr<AuthenticationService> m_service;
    };

    TEST_F( AuthServiceTest, LoginWithValidCredentialsReturnsTokens )
    {
        auto result = m_service->Login( "admin", "correct-password" );
        ASSERT_TRUE( result.has_value() );
        EXPECT_FALSE( result->m_accessToken.empty() );
        EXPECT_FALSE( result->m_refreshToken.empty() );
        EXPECT_GT( result->m_expiresInSeconds, 0u );
    }

    TEST_F( AuthServiceTest, LoginWithWrongPasswordFails )
    {
        auto result = m_service->Login( "admin", "wrong" );
        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::AuthenticationFailed );
    }

    TEST_F( AuthServiceTest, LoginWithUnknownUserFails )
    {
        auto result = m_service->Login( "nobody", "password" );
        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::AuthenticationFailed );
    }

    TEST_F( AuthServiceTest, ValidateTokenAfterLoginSucceeds )
    {
        auto login = m_service->Login( "admin", "correct-password" );
        ASSERT_TRUE( login.has_value() );

        auto claims = m_service->ValidateToken( login->m_accessToken );
        ASSERT_TRUE( claims.has_value() );
        EXPECT_EQ( claims->m_username, "admin" );
        EXPECT_EQ( claims->m_role, core::Role::Admin );
    }

    TEST_F( AuthServiceTest, ValidateInvalidTokenFails )
    {
        auto result = m_service->ValidateToken( "not.a.token" );
        ASSERT_FALSE( result.has_value() );
    }

    TEST_F( AuthServiceTest, LogoutRevokesToken )
    {
        auto login = m_service->Login( "admin", "correct-password" );
        ASSERT_TRUE( login.has_value() );

        ASSERT_TRUE( m_service->Logout( login->m_accessToken ).has_value() );

        auto validate = m_service->ValidateToken( login->m_accessToken );
        ASSERT_FALSE( validate.has_value() );
        EXPECT_EQ( validate.error(), Error::TokenExpired );
    }

    TEST_F( AuthServiceTest, RefreshTokenGrantsNewAccessToken )
    {
        auto login = m_service->Login( "admin", "correct-password" );
        ASSERT_TRUE( login.has_value() );

        auto refresh = m_service->RefreshToken( login->m_refreshToken );
        ASSERT_TRUE( refresh.has_value() );
        EXPECT_FALSE( refresh->m_accessToken.empty() );

        // Validate the new token is usable.
        auto claims = m_service->ValidateToken( refresh->m_accessToken );
        ASSERT_TRUE( claims.has_value() );
        EXPECT_EQ( claims->m_username, "admin" );
    }

    TEST_F( AuthServiceTest, RefreshWithRevokedTokenFails )
    {
        auto login = m_service->Login( "admin", "correct-password" );
        ASSERT_TRUE( login.has_value() );

        ASSERT_TRUE( m_service->Logout( login->m_refreshToken ).has_value() );

        auto refresh = m_service->RefreshToken( login->m_refreshToken );
        ASSERT_FALSE( refresh.has_value() );
        EXPECT_EQ( refresh.error(), Error::TokenExpired );
    }

    TEST_F( AuthServiceTest, RegisterDuplicateUserFails )
    {
        auto result = m_service->RegisterUser( "user-002", "admin", "other-pass", core::Role::Viewer );
        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::InvalidInput );
    }

} // namespace
