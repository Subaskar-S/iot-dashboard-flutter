/**
 * @file       http_types.cpp
 * @brief      HttpRequest / HttpResponse method implementations
 * @standard   C++23
 */

#include "network/http/http_types.hpp"
#include <algorithm>
#include <sstream>

namespace iot::network::http
{
    // -----------------------------------------------------------------------
    // HttpRequest helpers
    // -----------------------------------------------------------------------

    std::string HttpRequest::GetHeader( std::string_view key ) const
    {
        // Headers are stored with lower-cased keys.
        std::string lower( key );
        std::ranges::transform( lower, lower.begin(), ::tolower );

        auto it = m_headers.find( lower );
        return it != m_headers.end() ? it->second : "";
    }

    std::string HttpRequest::GetQueryParam( std::string_view name ) const
    {
        if ( m_query.empty() )
        {
            return "";
        }

        std::istringstream ss( m_query );
        std::string token;

        while ( std::getline( ss, token, '&' ) )
        {
            auto eq = token.find( '=' );
            if ( eq == std::string::npos )
            {
                continue;
            }

            std::string_view paramName( token.data(), eq );
            if ( paramName == name )
            {
                return token.substr( eq + 1 );
            }
        }

        return "";
    }

    // -----------------------------------------------------------------------
    // HttpResponse helpers
    // -----------------------------------------------------------------------

    void HttpResponse::Json( uint16_t status, std::string body )
    {
        m_status = status;
        m_headers["content-type"] = "application/json";
        m_body = std::move( body );
    }

    void HttpResponse::Text( uint16_t status, std::string body )
    {
        m_status = status;
        m_headers["content-type"] = "text/plain";
        m_body = std::move( body );
    }

    void HttpResponse::NoContent()
    {
        m_status = 204;
        m_body.clear();
    }

    void HttpResponse::Error( uint16_t status, std::string_view code, std::string_view message )
    {
        Json( status, R"({"error":")" + std::string( code ) + R"(","message":")" + std::string( message ) + R"("})" );
    }

} // namespace iot::network::http
