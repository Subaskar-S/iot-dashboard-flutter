/**
 * @file       auth_middleware.cpp
 * @standard   C++23
 */

#include "api/middleware/auth_middleware.hpp"

namespace iot::api
{
    network::http::Middleware MakeTokenMiddleware( security::AuthenticationService& authService )
    {
        return [&authService]( const network::http::HttpRequest& req, network::http::HttpResponse& res ) -> bool
        {
            if ( IsPublicPath( req.m_path ) )
                return true;

            std::string auth = req.GetHeader( "authorization" );

            if ( auth.empty() || !auth.starts_with( "Bearer " ) )
            {
                res.Error( 401, "Unauthorized", "Missing Bearer token" );
                return false;
            }

            std::string token = auth.substr( 7 );
            auto claimsResult = authService.ValidateToken( token );

            if ( !claimsResult )
            {
                res.Error( 401, "Unauthorized", "Invalid or expired token" );
                return false;
            }

            // Inject claims into headers for downstream handlers.
            const_cast<network::http::HttpRequest&>( req ).m_headers["x-user-id"] = claimsResult->m_userId;
            const_cast<network::http::HttpRequest&>( req ).m_headers["x-username"] = claimsResult->m_username;
            const_cast<network::http::HttpRequest&>( req ).m_headers["x-role"] =
                security::AccessControl::RoleToString( claimsResult->m_role );

            return true;
        };
    }

    network::http::Middleware MakeAuthMiddleware( security::AuthenticationService& authService,
                                                  std::string_view resource,
                                                  security::Permission requiredPermission )
    {
        return [&authService, res = std::string( resource ), requiredPermission](
                   const network::http::HttpRequest& req, network::http::HttpResponse& httpRes ) -> bool
        {
            if ( IsPublicPath( req.m_path ) )
                return true;

            std::string auth = req.GetHeader( "authorization" );

            if ( auth.empty() || !auth.starts_with( "Bearer " ) )
            {
                httpRes.Error( 401, "Unauthorized", "Missing Bearer token" );
                return false;
            }

            std::string token = auth.substr( 7 );
            auto claimsResult = authService.ValidateToken( token );

            if ( !claimsResult )
            {
                httpRes.Error( 401, "Unauthorized", "Invalid or expired token" );
                return false;
            }

            if ( !security::AccessControl::CanAccess( claimsResult->m_role, res, requiredPermission ) )
            {
                httpRes.Error( 403, "Forbidden", "Insufficient permissions for this resource" );
                return false;
            }

            const_cast<network::http::HttpRequest&>( req ).m_headers["x-user-id"] = claimsResult->m_userId;
            const_cast<network::http::HttpRequest&>( req ).m_headers["x-username"] = claimsResult->m_username;
            const_cast<network::http::HttpRequest&>( req ).m_headers["x-role"] =
                security::AccessControl::RoleToString( claimsResult->m_role );

            return true;
        };
    }

} // namespace iot::api
