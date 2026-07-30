/**
 * @file       http_types.hpp
 * @brief      HTTP request/response value types
 * @standard   C++23
 */

#ifndef IOT_NETWORK_HTTP_HTTP_TYPES_HPP
#define IOT_NETWORK_HTTP_HTTP_TYPES_HPP

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

namespace iot::network::http
{
    enum class HttpMethod : uint8_t
    {
        GET,
        POST,
        PUT,
        DELETE,
        PATCH,
        OPTIONS,
        HEAD,
        Unknown
    };

    inline HttpMethod MethodFromString( std::string_view s ) noexcept
    {
        if ( s == "GET" )
            return HttpMethod::GET;
        if ( s == "POST" )
            return HttpMethod::POST;
        if ( s == "PUT" )
            return HttpMethod::PUT;
        if ( s == "DELETE" )
            return HttpMethod::DELETE;
        if ( s == "PATCH" )
            return HttpMethod::PATCH;
        if ( s == "OPTIONS" )
            return HttpMethod::OPTIONS;
        if ( s == "HEAD" )
            return HttpMethod::HEAD;
        return HttpMethod::Unknown;
    }

    using Headers = std::unordered_map<std::string, std::string>;

    struct HttpRequest
    {
        HttpMethod m_method = HttpMethod::Unknown;
        std::string m_target; // path + query, e.g. "/devices?limit=10"
        std::string m_path;   // path only,  e.g. "/devices"
        std::string m_query;  // query string, e.g. "limit=10"
        Headers m_headers;
        std::string m_body;
        uint32_t m_httpVersion = 11; // 10 = HTTP/1.0, 11 = HTTP/1.1

        /// Convenience: get a header value (case-insensitive key lookup).
        [[nodiscard]] std::string GetHeader( std::string_view key ) const;

        /// Convenience: get a query parameter value by name.
        [[nodiscard]] std::string GetQueryParam( std::string_view name ) const;
    };

    struct HttpResponse
    {
        uint16_t m_status = 200;
        Headers m_headers;
        std::string m_body;

        /// Set JSON body and Content-Type header in one call.
        void Json( uint16_t status, std::string body );

        /// Set plain-text body.
        void Text( uint16_t status, std::string body );

        /// Empty response (e.g. 204 No Content).
        void NoContent();

        /// Standard error response with JSON body.
        void Error( uint16_t status, std::string_view code, std::string_view message );
    };

    /// Route handler: receives a request, fills a response.
    using RouteHandler = std::function<void( const HttpRequest&, HttpResponse& )>;

    /// Middleware: called before route handler; return false to abort.
    using Middleware = std::function<bool( const HttpRequest&, HttpResponse& )>;

} // namespace iot::network::http

#endif // IOT_NETWORK_HTTP_HTTP_TYPES_HPP
