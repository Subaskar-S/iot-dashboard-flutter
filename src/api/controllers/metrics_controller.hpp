/**
 * @file       metrics_controller.hpp
 * @standard   C++23
 */

#ifndef IOT_API_CONTROLLERS_METRICS_CONTROLLER_HPP
#define IOT_API_CONTROLLERS_METRICS_CONTROLLER_HPP

#include "network/http/http_router.hpp"
#include "network/websocket/ws_server.hpp"
#include "security/authentication_service.hpp"
#include <atomic>

namespace iot::api
{
    class MetricsController
    {
        public:
        MetricsController( const network::websocket::WsServer& wsServer, security::AuthenticationService& authService );

        void Register( network::http::HttpRouter& router );

        // Counters updated by application layer.
        void IncrementHttpRequests() noexcept;
        void IncrementMqttMessages() noexcept;

        private:
        const network::websocket::WsServer& m_wsServer;
        security::AuthenticationService& m_authService;
        std::atomic<uint64_t> m_httpRequests{ 0 };
        std::atomic<uint64_t> m_mqttMessages{ 0 };
        std::chrono::steady_clock::time_point m_startTime;
    };

} // namespace iot::api

#endif // IOT_API_CONTROLLERS_METRICS_CONTROLLER_HPP
