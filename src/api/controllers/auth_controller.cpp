/**
 * @file       auth_controller.cpp
 * @standard   C++23
 */

#include "api/controllers/auth_controller.hpp"
#include <nlohmann/json.hpp>

namespace iot::api
{
    AuthController::AuthController( security::AuthenticationService& authService )
        : m_authService( authService )
    {
    }

    void AuthController::Register( network::http::HttpRouter& router )
    {
        // POST /auth/login
        router.AddRoute( network::http::HttpMethod::POST, "/auth/login",
                         [this]( const network::http::HttpRequest& req, network::http::HttpResponse& res )
                         {
                             nlohmann::json body;
                             try
                             {
                                 body = nlohmann::json::parse( req.m_body );
                             }
                             catch ( ... )
                             {
                                 res.Error( 400, "BadRequest", "Invalid JSON body" );
                                 return;
                             }

                             auto username = body.value( "username", std::string{} );
                             auto password = body.value( "password", std::string{} );

                             if ( username.empty() || password.empty() )
                             {
                                 res.Error( 400, "BadRequest", "username and password required" );
                                 return;
                             }

                             auto result = m_authService.Login( username, password );
                             if ( !result )
                             {
                                 res.Error( 401, "Unauthorized", "Invalid credentials" );
                                 return;
                             }

                             nlohmann::json j = { { "access_token", result->m_accessToken },
                                                  { "refresh_token", result->m_refreshToken },
                                                  { "expires_in", result->m_expiresInSeconds },
                                                  { "token_type", "Bearer" } };

                             res.Json( 200, j.dump() );
                         } );

        // POST /auth/refresh
        router.AddRoute( network::http::HttpMethod::POST, "/auth/refresh",
                         [this]( const network::http::HttpRequest& req, network::http::HttpResponse& res )
                         {
                             nlohmann::json body;
                             try
                             {
                                 body = nlohmann::json::parse( req.m_body );
                             }
                             catch ( ... )
                             {
                                 res.Error( 400, "BadRequest", "Invalid JSON body" );
                                 return;
                             }

                             auto refreshToken = body.value( "refresh_token", std::string{} );
                             if ( refreshToken.empty() )
                             {
                                 res.Error( 400, "BadRequest", "refresh_token required" );
                                 return;
                             }

                             auto result = m_authService.RefreshToken( refreshToken );
                             if ( !result )
                             {
                                 res.Error( 401, "Unauthorized", "Invalid or expired refresh token" );
                                 return;
                             }

                             nlohmann::json j = { { "access_token", result->m_accessToken },
                                                  { "expires_in", result->m_expiresInSeconds },
                                                  { "token_type", "Bearer" } };

                             res.Json( 200, j.dump() );
                         } );

        // POST /auth/logout
        router.AddRoute( network::http::HttpMethod::POST, "/auth/logout",
                         [this]( const network::http::HttpRequest& req, network::http::HttpResponse& res )
                         {
                             std::string auth = req.GetHeader( "authorization" );
                             if ( auth.starts_with( "Bearer " ) )
                             {
                                 std::ignore = m_authService.Logout( auth.substr( 7 ) );
                             }
                             res.NoContent();
                         } );
    }

} // namespace iot::api
