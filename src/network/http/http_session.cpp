/**
 * @file       http_session.cpp
 * @brief      Beast async HTTP/1.1 session implementation
 * @standard   C++23
 */

#include "network/http/http_session.hpp"
#include <algorithm>

namespace iot::network::http
{
    HttpSession::HttpSession( asio::ip::tcp::socket socket,
                              const HttpRouter& router,
                              bool enableCors,
                              std::shared_ptr<spdlog::logger> logger )
        : m_stream( std::move( socket ) )
        , m_router( router )
        , m_enableCors( enableCors )
        , m_logger( std::move( logger ) )
    {
    }

    void HttpSession::Run()
    {
        asio::dispatch( m_stream.get_executor(),
                        beast::bind_front_handler( &HttpSession::DoRead, shared_from_this() ) );
    }

    void HttpSession::DoRead()
    {
        m_beastReq = {};
        m_buffer.consume( m_buffer.size() );

        bhttp::async_read( m_stream, m_buffer, m_beastReq,
                           beast::bind_front_handler( &HttpSession::OnRead, shared_from_this() ) );
    }

    void HttpSession::OnRead( beast::error_code ec, std::size_t /* bytes */ )
    {
        if ( ec == bhttp::error::end_of_stream )
        {
            beast::error_code shutEc;
            m_stream.socket().shutdown( asio::ip::tcp::socket::shutdown_send, shutEc );
            return;
        }

        if ( ec )
        {
            m_logger->debug( "HTTP read error: {}", ec.message() );
            return;
        }

        // Wrap in try/catch so async handler never throws (would terminate).
        try
        {
            HttpRequest appReq = BeastToApp( m_beastReq );

            // Handle pre-flight CORS immediately.
            if ( m_enableCors && appReq.m_method == HttpMethod::OPTIONS )
            {
                HttpResponse appRes;
                appRes.m_status = 204;
                appRes.m_headers["access-control-allow-origin"] = "*";
                appRes.m_headers["access-control-allow-methods"] =
                    "GET,POST,PUT,DELETE,PATCH,OPTIONS";
                appRes.m_headers["access-control-allow-headers"] = "Authorization,Content-Type";
                SendResponse( std::move( appRes ) );
                return;
            }

            HttpResponse appRes;
            m_router.Dispatch( appReq, appRes );

            if ( m_enableCors )
            {
                appRes.m_headers["access-control-allow-origin"] = "*";
            }

            m_logger->info( "{} {} -> {}", static_cast<int>( appReq.m_method ),
                             appReq.m_path, appRes.m_status );

            SendResponse( std::move( appRes ) );
        }
        catch ( const std::exception& e )
        {
            m_logger->error( "HTTP handler exception [{}]: {}", typeid( e ).name(), e.what() );
            HttpResponse errRes;
            errRes.Error( 500, "InternalError", "Internal server error" );
            SendResponse( std::move( errRes ) );
        }
    }

    void HttpSession::SendResponse( HttpResponse appRes )
    {
        bool keepAlive = m_beastReq.keep_alive();
        auto bRes = AppToBeast( appRes, m_beastReq.version(), keepAlive );

        auto spRes = std::make_shared<bhttp::response<bhttp::string_body>>( std::move( bRes ) );

        bhttp::async_write( m_stream, *spRes,
                            [self = shared_from_this(), spRes, keepAlive]( beast::error_code ec, std::size_t bytes )
                            {
                                self->OnWrite( !keepAlive, ec, bytes );
                            } );
    }

    void HttpSession::OnWrite( bool close, beast::error_code ec, std::size_t /* bytes */ )
    {
        if ( ec )
        {
            m_logger->debug( "HTTP write error: {}", ec.message() );
            return;
        }

        if ( close )
        {
            beast::error_code shutEc;
            m_stream.socket().shutdown( asio::ip::tcp::socket::shutdown_send, shutEc );
            return;
        }

        DoRead(); // HTTP/1.1 keep-alive — read next request
    }

    // -----------------------------------------------------------------------
    // Beast ↔ App type conversions
    // -----------------------------------------------------------------------

    HttpRequest HttpSession::BeastToApp( const bhttp::request<bhttp::string_body>& bReq )
    {
        HttpRequest req;
        req.m_httpVersion = bReq.version();

        // Map Beast verb to our enum.
        switch ( bReq.method() )
        {
            case bhttp::verb::get:
                req.m_method = HttpMethod::GET;
                break;
            case bhttp::verb::post:
                req.m_method = HttpMethod::POST;
                break;
            case bhttp::verb::put:
                req.m_method = HttpMethod::PUT;
                break;
            case bhttp::verb::delete_:
                req.m_method = HttpMethod::DELETE;
                break;
            case bhttp::verb::patch:
                req.m_method = HttpMethod::PATCH;
                break;
            case bhttp::verb::options:
                req.m_method = HttpMethod::OPTIONS;
                break;
            case bhttp::verb::head:
                req.m_method = HttpMethod::HEAD;
                break;
            default:
                req.m_method = HttpMethod::Unknown;
                break;
        }

        req.m_target = std::string( bReq.target() );

        auto qPos = req.m_target.find( '?' );
        if ( qPos != std::string::npos )
        {
            req.m_path = req.m_target.substr( 0, qPos );
            req.m_query = req.m_target.substr( qPos + 1 );
        }
        else
        {
            req.m_path = req.m_target;
        }

        req.m_body = bReq.body();

        for ( const auto& field : bReq )
        {
            std::string key( field.name_string() );
            std::ranges::transform( key, key.begin(), ::tolower );
            req.m_headers[key] = std::string( field.value() );
        }

        return req;
    }

    bhttp::response<bhttp::string_body> HttpSession::AppToBeast( const HttpResponse& appRes,
                                                                 unsigned httpVersion,
                                                                 bool keepAlive )
    {
        // Clamp httpVersion to valid HTTP/1.x values
        if ( httpVersion != 10 && httpVersion != 11 )
        {
            httpVersion = 11;
        }

        bhttp::response<bhttp::string_body> res{
            static_cast<bhttp::status>( appRes.m_status ), httpVersion
        };

        for ( const auto& [key, value] : appRes.m_headers )
        {
            // Skip empty keys — Beast throws on empty field name
            if ( !key.empty() )
            {
                res.set( key, value );
            }
        }

        res.keep_alive( keepAlive );
        res.body() = appRes.m_body;
        res.prepare_payload();
        return res;
    }

} // namespace iot::network::http
