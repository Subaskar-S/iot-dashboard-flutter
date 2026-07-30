/**
 * @file       http_router.hpp
 * @brief      Path-based HTTP router with middleware support
 * @standard   C++23
 */

#ifndef IOT_NETWORK_HTTP_HTTP_ROUTER_HPP
#define IOT_NETWORK_HTTP_HTTP_ROUTER_HPP

#include "network/http/http_types.hpp"
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

namespace iot::network::http
{
    /**
     * Stores method + path → handler mappings and evaluates them in
     * registration order.  Path parameters are not supported in this
     * iteration; patterns are exact-match strings.
     *
     * Middleware runs before every handler.  If any middleware returns
     * false the handler is not called and the response (as set by the
     * middleware) is returned immediately.
     *
     * Thread-safe: AddRoute / AddMiddleware lock m_mutex; Dispatch is
     * read-only after setup so also safe.
     */
    class HttpRouter
    {
        public:
        HttpRouter();

        void AddRoute( HttpMethod method, std::string path, RouteHandler handler );
        void AddMiddleware( Middleware mw );

        /// Find and invoke the matching handler.
        /// Returns 404 JSON if no route matches, 405 if method wrong.
        void Dispatch( const HttpRequest& req, HttpResponse& res ) const;

        private:
        struct Route
        {
            HttpMethod m_method;
            std::string m_path;
            RouteHandler m_handler;
        };

        std::vector<Route> m_routes;
        std::vector<Middleware> m_middlewares;
        mutable std::mutex m_mutex;
        std::shared_ptr<spdlog::logger> m_logger;
    };

} // namespace iot::network::http

#endif // IOT_NETWORK_HTTP_HTTP_ROUTER_HPP
