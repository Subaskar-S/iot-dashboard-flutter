/**
 * @file       i_authentication_service.hpp
 * @brief      Port for user authentication and token management
 * @standard   C++23
 */

#ifndef IOT_CORE_I_AUTHENTICATION_SERVICE_HPP
#define IOT_CORE_I_AUTHENTICATION_SERVICE_HPP

#include "common/error.hpp"
#include <cstdint>
#include <string>
#include <string_view>

namespace iot::core
{
    /// Role assigned to an authenticated user, used for access control.
    enum class Role : uint8_t
    {
        Admin,
        Operator,
        Viewer
    };

    struct UserClaims
    {
        std::string m_userId;
        std::string m_username;
        Role m_role;
    };

    struct TokenPair
    {
        std::string m_accessToken;
        std::string m_refreshToken;
        uint32_t m_expiresInSeconds = 0;
    };

    /**
     * Abstract authentication boundary.
     *
     * The concrete implementation (JWT + bcrypt) lives in the security
     * module. Controllers and other use cases depend only on this
     * interface.
     */
    class IAuthenticationService
    {
        public:
        virtual ~IAuthenticationService() = default;

        [[nodiscard]] virtual Result<TokenPair> Login( std::string_view username, std::string_view password ) = 0;

        [[nodiscard]] virtual Result<TokenPair> RefreshToken( std::string_view refreshToken ) = 0;

        [[nodiscard]] virtual Result<void> Logout( std::string_view accessToken ) = 0;

        [[nodiscard]] virtual Result<UserClaims> ValidateToken( std::string_view accessToken ) = 0;
    };

} // namespace iot::core

#endif // IOT_CORE_I_AUTHENTICATION_SERVICE_HPP
