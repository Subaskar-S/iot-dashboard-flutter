/**
 * @file       http_router.cpp
 * @brief      HTTP router implementation
 * @standard   C++23
 */

#include "network/http/http_router.hpp"
#include "common/logging.hpp"
#include <algorithm>

namespace iot::network::http
{
    HttpRouter::HttpRouter()
        : m_logger( CreateLogger( "HttpRouter" ) )
    {
    }

    void HttpRouter::AddRoute( HttpMethod method, std::string path, RouteHandler handler )
    {
        std::lock_guard lock( m_mutex );
        m_routes.push_back( { method, std::move( path ), std::move( handler ) } );
    }

    void HttpRouter::AddMiddleware( Middleware mw )
    {
        std::lock_guard lock( m_mutex );
        m_middlewares.push_back( std::move( mw ) );
    }

    void HttpRouter::Dispatch( const HttpRequest& req, HttpResponse& res ) const
    {
        std::lock_guard lock( m_mutex );

        // Run middleware chain first.
        for ( const auto& mw : m_middlewares )
        {
            if ( !mw( req, res ) )
            {
                return; // Middleware short-circuited the request.
            }
        }

        // Find a matching route.
        bool pathMatched = false;

        for ( const auto& route : m_routes )
        {
            if ( route.m_path != req.m_path )
            {
                continue;
            }

            pathMatched = true;

            if ( route.m_method != req.m_method )
            {
                continue;
            }

            m_logger->debug( "Dispatching {} {}", static_cast<int>( req.m_method ), req.m_path );
            route.m_handler( req, res );
            return;
        }

        if ( pathMatched )
        {
            res.Error( 405, "MethodNotAllowed", "Method not allowed for this endpoint" );
        }
        else
        {
            res.Error( 404, "NotFound", "Endpoint not found" );
        }
    }

} // namespace iot::network::http
