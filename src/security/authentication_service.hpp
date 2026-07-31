/**
 * @file       authentication_service.hpp
 * @brief      Concrete IAuthenticationService — login, refresh, validate
 * @standard   C++23
 */

#ifndef IOT_SECURITY_AUTHENTICATION_SERVICE_HPP
#define IOT_SECURITY_AUTHENTICATION_SERVICE_HPP

#include "core/interfaces/i_authentication_service.hpp"
#include "security/jwt_handler.hpp"
#include "security/password_hasher.hpp"
#include <mutex>
#include <spdlog/spdlog.h>
#include <unordered_set>

namespace iot::security
{
    struct SecurityConfig
    {
        std::string m_jwtSecret = "change-me-in-production";
        std::chrono::seconds m_accessTokenExpiry{ 3600 };
        std::chrono::seconds m_refreshTokenExpiry{ 86400 * 30 };
        uint32_t m_pbkdf2Iterations = 600000;
    };

    /// Minimal user record used by AuthenticationService.
    struct UserRecord
    {
        std::string m_userId;
        std::string m_username;
        std::string m_passwordHash; // PBKDF2 hash string
        core::Role m_role = core::Role::Viewer;
    };

    /**
     * Login / refresh / logout / validate via JWT + PBKDF2 passwords.
     *
     * User storage is in-memory; a real deployment would inject an
     * IUserRepository.  The interface (IAuthenticationService) is
     * unchanged, making that swap transparent to callers.
     */
    class AuthenticationService final : public core::IAuthenticationService
    {
        public:
        explicit AuthenticationService( SecurityConfig config );

        // Register a user (for bootstrapping / tests).
        [[nodiscard]] Result<void> RegisterUser( const std::string& userId,
                                                 const std::string& username,
                                                 const std::string& password,
                                                 core::Role role );

        // IAuthenticationService
        [[nodiscard]] Result<core::TokenPair> Login( std::string_view username, std::string_view password ) override;

        [[nodiscard]] Result<core::TokenPair> RefreshToken( std::string_view refreshToken ) override;

        [[nodiscard]] Result<void> Logout( std::string_view accessToken ) override;

        [[nodiscard]] Result<core::UserClaims> ValidateToken( std::string_view accessToken ) override;

        private:
        SecurityConfig m_config;
        JwtHandler m_jwtHandler;
        PasswordHasher m_hasher;
        std::shared_ptr<spdlog::logger> m_logger;

        mutable std::mutex m_mutex;
        std::unordered_map<std::string, UserRecord> m_users; // keyed by username
        std::unordered_set<std::string> m_revokedTokens;     // logout blacklist
    };

} // namespace iot::security

#endif // IOT_SECURITY_AUTHENTICATION_SERVICE_HPP
