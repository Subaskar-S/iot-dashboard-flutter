/**
 * @file       password_hasher.hpp
 * @brief      PBKDF2-SHA256 password hashing (bcrypt-equivalent strength)
 * @standard   C++23
 *
 * Format of stored hash:
 *   $pbkdf2-sha256$<iterations>$<base64-salt>$<base64-hash>
 *
 * Default: 600,000 iterations (NIST SP 800-132 recommendation for SHA-256).
 */

#ifndef IOT_SECURITY_PASSWORD_HASHER_HPP
#define IOT_SECURITY_PASSWORD_HASHER_HPP

#include "common/error.hpp"
#include <string>
#include <string_view>

namespace iot::security
{
    class PasswordHasher
    {
        public:
        explicit PasswordHasher( uint32_t iterations = 600000 );

        /// Returns a self-contained hash string (includes salt + iterations).
        [[nodiscard]] Result<std::string> Hash( std::string_view password ) const;

        /// Returns true if password matches the stored hash.
        [[nodiscard]] bool Verify( std::string_view password, std::string_view storedHash ) const;

        private:
        uint32_t m_iterations;

        static constexpr size_t kSaltBytes = 16;
        static constexpr size_t kHashBytes = 32;
    };

} // namespace iot::security

#endif // IOT_SECURITY_PASSWORD_HASHER_HPP
