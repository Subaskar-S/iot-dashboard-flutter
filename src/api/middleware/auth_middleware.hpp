/**
 * @file       auth_middleware.hpp
 * @brief      HTTP middleware — validates JWT and enforces RBAC
 * @standard   C++23
 */

#ifndef IOT_API_MIDDLEWARE_AUTH_MIDDLEWARE_HPP
#define IOT_API_MIDDLEWARE_AUTH_MIDDLEWARE_HPP

#include "network/http/http_types.hpp"
#include "security/access_control.hpp"
#include "security/authentication_service.hpp"

namespace iot::api
{
    /**
     * Returns a Middleware lambda that:
     *   1. Skips if the request path is in the excluded list (e.g. /health).
     *   2. Extracts the Bearer token from the Authorization header.
     *   3. Validates the token via AuthenticationService.
     *   4. Checks RBAC via AccessControl::CanAccess(role, resource, permission).
     *   5. Returns false (aborting the handler) with 401/403 on failure.
     *
     * The validated UserClaims are injected into the request headers as
     * "x-user-id", "x-username", and "x-role" for downstream handlers.
     *
     * Public paths that never need authentication: /health, /auth/login, /auth/refresh
     */
    network::http::Middleware MakeAuthMiddleware( security::AuthenticationService& authService,
                                                  std::string_view resource,
                                                  security::Permission requiredPermission );

    /// Middleware that only validates the token (no RBAC check).
    network::http::Middleware MakeTokenMiddleware( security::AuthenticationService& authService );

    /// Paths that are exempt from authentication checks.
    inline bool IsPublicPath( std::string_view path )
    {
        return path == "/health" || path == "/auth/login" || path == "/auth/refresh";
    }

} // namespace iot::api

#endif // IOT_API_MIDDLEWARE_AUTH_MIDDLEWARE_HPP
