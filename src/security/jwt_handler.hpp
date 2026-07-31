/**
 * @file       jwt_handler.hpp
 * @brief      HS256 JWT generation and verification
 * @standard   C++23
 *
 * Produces compact JWTs:
 *   base64url(header) . base64url(payload) . base64url(signature)
 *
 * Algorithm: HMAC-SHA256 (HS256)
 * No third-party JWT library — implemented directly over OpenSSL HMAC.
 */

#ifndef IOT_SECURITY_JWT_HANDLER_HPP
#define IOT_SECURITY_JWT_HANDLER_HPP

#include "common/error.hpp"
#include "core/interfaces/i_authentication_service.hpp"
#include <chrono>
#include <string>
#include <string_view>

namespace iot::security
{
    class JwtHandler
    {
        public:
        explicit JwtHandler( std::string secret );

        /// Generate a signed JWT with standard claims.
        [[nodiscard]] Result<std::string> Generate( const core::UserClaims& claims,
                                                    std::chrono::seconds expirySeconds ) const;

        /// Verify signature and expiry; returns parsed claims on success.
        [[nodiscard]] Result<core::UserClaims> Verify( std::string_view token ) const;

        private:
        std::string m_secret;

        [[nodiscard]] std::string Sign( std::string_view data ) const;
    };

} // namespace iot::security

#endif // IOT_SECURITY_JWT_HANDLER_HPP
