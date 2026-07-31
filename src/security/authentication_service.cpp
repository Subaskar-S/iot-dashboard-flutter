/**
 * @file       authentication_service.cpp
 * @brief      Login / refresh / logout / validate
 * @standard   C++23
 */

#include "security/authentication_service.hpp"
#include "common/logging.hpp"

namespace iot::security
{
    AuthenticationService::AuthenticationService( SecurityConfig config )
        : m_config( std::move( config ) )
        , m_jwtHandler( m_config.m_jwtSecret )
        , m_hasher( m_config.m_pbkdf2Iterations )
        , m_logger( CreateLogger( "AuthenticationService" ) )
    {
    }

    Result<void> AuthenticationService::RegisterUser( const std::string& userId,
                                                      const std::string& username,
                                                      const std::string& password,
                                                      core::Role role )
    {
        auto hashResult = m_hasher.Hash( password );
        if ( !hashResult )
        {
            return std::unexpected( hashResult.error() );
        }

        std::lock_guard lock( m_mutex );
        if ( m_users.contains( username ) )
        {
            return std::unexpected( Error::InvalidInput );
        }

        m_users[username] = { userId, username, hashResult.value(), role };
        return {};
    }

    Result<core::TokenPair> AuthenticationService::Login( std::string_view username, std::string_view password )
    {
        std::lock_guard lock( m_mutex );

        auto it = m_users.find( std::string( username ) );
        if ( it == m_users.end() )
        {
            m_logger->warn( "Login failed: unknown user '{}'", username );
            return std::unexpected( Error::AuthenticationFailed );
        }

        if ( !m_hasher.Verify( password, it->second.m_passwordHash ) )
        {
            m_logger->warn( "Login failed: wrong password for '{}'", username );
            return std::unexpected( Error::AuthenticationFailed );
        }

        core::UserClaims claims{ it->second.m_userId, it->second.m_username, it->second.m_role };

        auto accessResult = m_jwtHandler.Generate( claims, m_config.m_accessTokenExpiry );
        if ( !accessResult )
        {
            return std::unexpected( accessResult.error() );
        }

        auto refreshResult = m_jwtHandler.Generate( claims, m_config.m_refreshTokenExpiry );
        if ( !refreshResult )
        {
            return std::unexpected( refreshResult.error() );
        }

        m_logger->info( "User '{}' logged in", username );

        return core::TokenPair{ .m_accessToken = accessResult.value(),
                                .m_refreshToken = refreshResult.value(),
                                .m_expiresInSeconds = static_cast<uint32_t>( m_config.m_accessTokenExpiry.count() ) };
    }

    Result<core::TokenPair> AuthenticationService::RefreshToken( std::string_view refreshToken )
    {
        auto claimsResult = m_jwtHandler.Verify( refreshToken );
        if ( !claimsResult )
        {
            return std::unexpected( claimsResult.error() );
        }

        {
            std::lock_guard lock( m_mutex );
            if ( m_revokedTokens.contains( std::string( refreshToken ) ) )
            {
                return std::unexpected( Error::TokenExpired );
            }
        }

        auto accessResult = m_jwtHandler.Generate( claimsResult.value(), m_config.m_accessTokenExpiry );
        if ( !accessResult )
        {
            return std::unexpected( accessResult.error() );
        }

        return core::TokenPair{ .m_accessToken = accessResult.value(),
                                .m_expiresInSeconds = static_cast<uint32_t>( m_config.m_accessTokenExpiry.count() ) };
    }

    Result<void> AuthenticationService::Logout( std::string_view accessToken )
    {
        std::lock_guard lock( m_mutex );
        m_revokedTokens.insert( std::string( accessToken ) );
        return {};
    }

    Result<core::UserClaims> AuthenticationService::ValidateToken( std::string_view accessToken )
    {
        {
            std::lock_guard lock( m_mutex );
            if ( m_revokedTokens.contains( std::string( accessToken ) ) )
            {
                return std::unexpected( Error::TokenExpired );
            }
        }

        return m_jwtHandler.Verify( accessToken );
    }

} // namespace iot::security
