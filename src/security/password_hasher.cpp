/**
 * @file       password_hasher.cpp
 * @brief      PBKDF2-SHA256 implementation via OpenSSL
 * @standard   C++23
 */

#include "security/password_hasher.hpp"
#include <array>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace iot::security
{
    namespace
    {
        /// Simple base64 encode (standard alphabet, no padding stripped).
        std::string B64Encode( const std::vector<uint8_t>& data )
        {
            static const char* kTable = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

            std::string out;
            out.reserve( ( data.size() + 2 ) / 3 * 4 );

            for ( size_t i = 0; i < data.size(); i += 3 )
            {
                uint32_t val = ( static_cast<uint32_t>( data[i] ) << 16 );
                if ( i + 1 < data.size() )
                    val |= ( static_cast<uint32_t>( data[i + 1] ) << 8 );
                if ( i + 2 < data.size() )
                    val |= static_cast<uint32_t>( data[i + 2] );

                out += kTable[( val >> 18 ) & 0x3F];
                out += kTable[( val >> 12 ) & 0x3F];
                out += ( i + 1 < data.size() ) ? kTable[( val >> 6 ) & 0x3F] : '=';
                out += ( i + 2 < data.size() ) ? kTable[val & 0x3F] : '=';
            }

            return out;
        }

        std::vector<uint8_t> B64Decode( std::string_view s )
        {
            auto dec = []( char c ) -> uint8_t
            {
                if ( c >= 'A' && c <= 'Z' )
                    return static_cast<uint8_t>( c - 'A' );
                if ( c >= 'a' && c <= 'z' )
                    return static_cast<uint8_t>( c - 'a' + 26 );
                if ( c >= '0' && c <= '9' )
                    return static_cast<uint8_t>( c - '0' + 52 );
                if ( c == '+' )
                    return 62;
                if ( c == '/' )
                    return 63;
                return 0;
            };

            std::vector<uint8_t> out;
            out.reserve( s.size() / 4 * 3 );

            for ( size_t i = 0; i + 3 < s.size(); i += 4 )
            {
                uint32_t val = ( static_cast<uint32_t>( dec( s[i] ) ) << 18 ) |
                               ( static_cast<uint32_t>( dec( s[i + 1] ) ) << 12 ) |
                               ( static_cast<uint32_t>( dec( s[i + 2] ) ) << 6 ) |
                               static_cast<uint32_t>( dec( s[i + 3] ) );

                out.push_back( static_cast<uint8_t>( val >> 16 ) );
                if ( s[i + 2] != '=' )
                    out.push_back( static_cast<uint8_t>( ( val >> 8 ) & 0xFF ) );
                if ( s[i + 3] != '=' )
                    out.push_back( static_cast<uint8_t>( val & 0xFF ) );
            }

            return out;
        }
    } // namespace

    PasswordHasher::PasswordHasher( uint32_t iterations )
        : m_iterations( iterations )
    {
    }

    Result<std::string> PasswordHasher::Hash( std::string_view password ) const
    {
        // Generate random salt.
        std::vector<uint8_t> salt( kSaltBytes );
        if ( RAND_bytes( salt.data(), static_cast<int>( kSaltBytes ) ) != 1 )
        {
            return std::unexpected( Error::InternalError );
        }

        // Derive key.
        std::vector<uint8_t> hash( kHashBytes );
        int rc = PKCS5_PBKDF2_HMAC( password.data(), static_cast<int>( password.size() ), salt.data(),
                                    static_cast<int>( salt.size() ), static_cast<int>( m_iterations ), EVP_sha256(),
                                    static_cast<int>( kHashBytes ), hash.data() );

        if ( rc != 1 )
        {
            return std::unexpected( Error::InternalError );
        }

        // Format: $pbkdf2-sha256$<iter>$<b64salt>$<b64hash>
        std::string stored =
            "$pbkdf2-sha256$" + std::to_string( m_iterations ) + "$" + B64Encode( salt ) + "$" + B64Encode( hash );

        return stored;
    }

    bool PasswordHasher::Verify( std::string_view password, std::string_view storedHash ) const
    {
        // Parse: $pbkdf2-sha256$<iter>$<b64salt>$<b64hash>
        if ( !storedHash.starts_with( "$pbkdf2-sha256$" ) )
        {
            return false;
        }

        auto rest = storedHash.substr( 15 );
        auto d1 = rest.find( '$' );
        if ( d1 == std::string_view::npos )
            return false;

        uint32_t iterations = 0;
        try
        {
            iterations = static_cast<uint32_t>( std::stoul( std::string( rest.substr( 0, d1 ) ) ) );
        }
        catch ( ... )
        {
            return false;
        }

        rest = rest.substr( d1 + 1 );
        auto d2 = rest.find( '$' );
        if ( d2 == std::string_view::npos )
            return false;

        auto salt = B64Decode( rest.substr( 0, d2 ) );
        auto expected = B64Decode( rest.substr( d2 + 1 ) );

        if ( salt.empty() || expected.empty() )
            return false;

        std::vector<uint8_t> computed( expected.size() );
        int rc = PKCS5_PBKDF2_HMAC( password.data(), static_cast<int>( password.size() ), salt.data(),
                                    static_cast<int>( salt.size() ), static_cast<int>( iterations ), EVP_sha256(),
                                    static_cast<int>( computed.size() ), computed.data() );

        if ( rc != 1 )
            return false;

        // Constant-time compare to prevent timing attacks.
        return CRYPTO_memcmp( computed.data(), expected.data(), computed.size() ) == 0;
    }

} // namespace iot::security
