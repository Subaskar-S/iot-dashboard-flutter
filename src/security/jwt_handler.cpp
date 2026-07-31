/**
 * @file       jwt_handler.cpp
 * @brief      HS256 JWT implementation via OpenSSL HMAC
 * @standard   C++23
 */

#include "security/jwt_handler.hpp"
#include <nlohmann/json.hpp>
#include <openssl/hmac.h>
#include <sstream>
#include <stdexcept>

namespace iot::security
{
    namespace
    {
        std::string Base64UrlEncode( const std::string& input )
        {
            static const char* kTable = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

            std::string out;
            out.reserve( ( input.size() + 2 ) / 3 * 4 );

            for ( size_t i = 0; i < input.size(); i += 3 )
            {
                uint32_t val = ( static_cast<uint8_t>( input[i] ) << 16 );
                if ( i + 1 < input.size() )
                    val |= ( static_cast<uint8_t>( input[i + 1] ) << 8 );
                if ( i + 2 < input.size() )
                    val |= static_cast<uint8_t>( input[i + 2] );

                out += kTable[( val >> 18 ) & 0x3F];
                out += kTable[( val >> 12 ) & 0x3F];
                out += ( i + 1 < input.size() ) ? kTable[( val >> 6 ) & 0x3F] : '=';
                out += ( i + 2 < input.size() ) ? kTable[val & 0x3F] : '=';
            }

            // Base64URL: replace + with -, / with _, strip padding
            for ( auto& c : out )
            {
                if ( c == '+' )
                    c = '-';
                else if ( c == '/' )
                    c = '_';
            }
            while ( !out.empty() && out.back() == '=' )
                out.pop_back();

            return out;
        }

        std::string Base64UrlDecode( std::string_view s )
        {
            std::string padded( s );
            for ( auto& c : padded )
            {
                if ( c == '-' )
                    c = '+';
                else if ( c == '_' )
                    c = '/';
            }

            while ( padded.size() % 4 != 0 )
                padded += '=';

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

            std::string out;
            out.reserve( padded.size() / 4 * 3 );

            for ( size_t i = 0; i + 3 < padded.size(); i += 4 )
            {
                uint32_t val = ( static_cast<uint32_t>( dec( padded[i] ) ) << 18 ) |
                               ( static_cast<uint32_t>( dec( padded[i + 1] ) ) << 12 ) |
                               ( static_cast<uint32_t>( dec( padded[i + 2] ) ) << 6 ) |
                               static_cast<uint32_t>( dec( padded[i + 3] ) );

                out += static_cast<char>( val >> 16 );
                if ( padded[i + 2] != '=' )
                    out += static_cast<char>( ( val >> 8 ) & 0xFF );
                if ( padded[i + 3] != '=' )
                    out += static_cast<char>( val & 0xFF );
            }

            return out;
        }

        std::string RoleToString( core::Role role )
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

        core::Role RoleFromString( std::string_view s )
        {
            if ( s == "Admin" )
                return core::Role::Admin;
            if ( s == "Operator" )
                return core::Role::Operator;
            return core::Role::Viewer;
        }
    } // namespace

    JwtHandler::JwtHandler( std::string secret )
        : m_secret( std::move( secret ) )
    {
    }

    std::string JwtHandler::Sign( std::string_view data ) const
    {
        unsigned int len = 0;
        unsigned char sig[EVP_MAX_MD_SIZE];

        HMAC( EVP_sha256(), m_secret.data(), static_cast<int>( m_secret.size() ),
              reinterpret_cast<const unsigned char*>( data.data() ), data.size(), sig, &len );

        return std::string( reinterpret_cast<char*>( sig ), len );
    }

    Result<std::string> JwtHandler::Generate( const core::UserClaims& claims, std::chrono::seconds expirySeconds ) const
    {
        // Header
        nlohmann::json header = { { "alg", "HS256" }, { "typ", "JWT" } };
        std::string hEncoded = Base64UrlEncode( header.dump() );

        // Payload
        auto now = std::chrono::system_clock::now();
        int64_t iat = std::chrono::duration_cast<std::chrono::seconds>( now.time_since_epoch() ).count();
        int64_t exp = iat + expirySeconds.count();

        nlohmann::json payload = { { "sub", claims.m_userId },
                                   { "username", claims.m_username },
                                   { "role", RoleToString( claims.m_role ) },
                                   { "iat", iat },
                                   { "exp", exp } };

        std::string pEncoded = Base64UrlEncode( payload.dump() );

        // Signature
        std::string signingInput = hEncoded + "." + pEncoded;
        std::string sig = Sign( signingInput );
        std::string sEncoded = Base64UrlEncode( sig );

        return signingInput + "." + sEncoded;
    }

    Result<core::UserClaims> JwtHandler::Verify( std::string_view token ) const
    {
        // Split into header.payload.signature
        auto d1 = token.find( '.' );
        if ( d1 == std::string_view::npos )
        {
            return std::unexpected( Error::AuthorizationFailed );
        }

        auto d2 = token.find( '.', d1 + 1 );
        if ( d2 == std::string_view::npos )
        {
            return std::unexpected( Error::AuthorizationFailed );
        }

        std::string_view signingInput = token.substr( 0, d2 );
        std::string_view sigEncoded = token.substr( d2 + 1 );

        // Verify signature
        std::string expectedSig = Base64UrlEncode( Sign( signingInput ) );
        if ( CRYPTO_memcmp( expectedSig.data(), sigEncoded.data(),
                            std::min( expectedSig.size(), sigEncoded.size() ) ) != 0 ||
             expectedSig.size() != sigEncoded.size() )
        {
            return std::unexpected( Error::AuthorizationFailed );
        }

        // Decode payload
        std::string payloadJson = Base64UrlDecode( token.substr( d1 + 1, d2 - d1 - 1 ) );

        nlohmann::json payload;
        try
        {
            payload = nlohmann::json::parse( payloadJson );
        }
        catch ( ... )
        {
            return std::unexpected( Error::AuthorizationFailed );
        }

        // Check expiry
        int64_t exp = payload.value( "exp", int64_t{ 0 } );
        auto now =
            std::chrono::duration_cast<std::chrono::seconds>( std::chrono::system_clock::now().time_since_epoch() )
                .count();

        if ( now > exp )
        {
            return std::unexpected( Error::TokenExpired );
        }

        core::UserClaims claims;
        claims.m_userId = payload.value( "sub", std::string{} );
        claims.m_username = payload.value( "username", std::string{} );
        claims.m_role = RoleFromString( payload.value( "role", std::string{ "Viewer" } ) );

        return claims;
    }

} // namespace iot::security
