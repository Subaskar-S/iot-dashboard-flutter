/**
 * @file       metrics_controller.cpp
 * @standard   C++23
 */

#include "api/controllers/metrics_controller.hpp"
#include "api/middleware/auth_middleware.hpp"
#include <nlohmann/json.hpp>

namespace iot::api
{
    MetricsController::MetricsController( const network::websocket::WsServer& wsServer,
                                          security::AuthenticationService& authService )
        : m_wsServer( wsServer )
        , m_authService( authService )
        , m_startTime( std::chrono::steady_clock::now() )
    {
    }

    void MetricsController::IncrementHttpRequests() noexcept
    {
        m_httpRequests.fetch_add( 1, std::memory_order_relaxed );
    }

    void MetricsController::IncrementMqttMessages() noexcept
    {
        m_mqttMessages.fetch_add( 1, std::memory_order_relaxed );
    }

    void MetricsController::Register( network::http::HttpRouter& router )
    {
        auto readAuth = MakeAuthMiddleware( m_authService, "metrics", security::Permission::Read );

        // GET /metrics
        router.AddMiddleware( readAuth );
        router.AddRoute( network::http::HttpMethod::GET, "/metrics",
                         [this]( const network::http::HttpRequest&, network::http::HttpResponse& res )
                         {
                             auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
                                               std::chrono::steady_clock::now() - m_startTime )
                                               .count();

                             nlohmann::json j = { { "uptime_seconds", uptime },
                                                  { "active_websocket_clients", m_wsServer.ConnectionCount() },
                                                  { "http_requests_total", m_httpRequests.load() },
                                                  { "mqtt_messages_total", m_mqttMessages.load() } };

                             res.Json( 200, j.dump() );
                         } );
    }

} // namespace iot::api
