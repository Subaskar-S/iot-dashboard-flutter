/**
 * @file       application.hpp
 * @brief      Composition root — wires all modules into a running server
 * @standard   C++23
 */

#ifndef IOT_API_APPLICATION_HPP
#define IOT_API_APPLICATION_HPP

#include "common/error.hpp"
#include "common/types.hpp"

// Infrastructure
#include "database/device_repository.hpp"
#include "database/migration_manager.hpp"
#include "database/schema.hpp"
#include "database/sensor_repository.hpp"
#include "database/sqlite_connection.hpp"

// Network
#include "network/http/http_server.hpp"
#include "network/mqtt/mqtt_client.hpp"
#include "network/websocket/ws_server.hpp"

// Domain
#include "automation/rule_engine.hpp"
#include "automation/rule_repository.hpp"
#include "devices/device_manager.hpp"
#include "devices/heartbeat_monitor.hpp"
#include "security/authentication_service.hpp"

// API
#include "api/controllers/auth_controller.hpp"
#include "api/controllers/automation_controller.hpp"
#include "api/controllers/device_controller.hpp"
#include "api/controllers/health_controller.hpp"
#include "api/controllers/metrics_controller.hpp"
#include "api/controllers/sensor_controller.hpp"
#include "api/middleware/auth_middleware.hpp"

#include <memory>
#include <spdlog/spdlog.h>

namespace iot::api
{
    struct AppConfig
    {
        // HTTP
        std::string m_httpHost = "0.0.0.0";
        uint16_t m_httpPort = 8080;
        uint32_t m_httpThreads = 4;

        // WebSocket
        uint16_t m_wsPort = 8081;
        uint32_t m_wsThreads = 2;

        // Database
        std::string m_dbPath = "./data/iot.db";

        // MQTT
        std::string m_mqttBroker = "tcp://localhost:1883";
        std::string m_mqttClientId = "iot-dashboard";

        // Security
        std::string m_jwtSecret = "change-in-production";
        uint32_t m_pbkdf2Iterations = 600000;

        // Heartbeat
        uint32_t m_heartbeatTimeoutSeconds = 300;
        uint32_t m_heartbeatPollSeconds = 30;

        // Logging
        std::string m_logLevel = "info";
    };

    /**
     * Composition root.  Owns all module instances and wires:
     *   MQTT messages → DeviceManager → WebSocket broadcast
     *   HTTP routes   → Controllers   → DeviceManager / RuleEngine
     *   HeartbeatMonitor → DeviceManager status callback
     *
     * Lifecycle: Initialize() → Start() → (blocking or event-driven) → Stop()
     */
    class Application
    {
        public:
        explicit Application( AppConfig config );
        ~Application() = default;

        Application( const Application& ) = delete;
        Application& operator=( const Application& ) = delete;

        [[nodiscard]] Result<void> Initialize();
        [[nodiscard]] Result<void> Start();
        void Stop();

        [[nodiscard]] bool IsRunning() const noexcept;

        // Expose servers for testing / embedding.
        [[nodiscard]] network::http::HttpServer& HttpServer() noexcept
        {
            return *m_httpServer;
        }

        [[nodiscard]] network::websocket::WsServer& WsServer() noexcept
        {
            return *m_wsServer;
        }

        private:
        AppConfig m_config;
        std::shared_ptr<spdlog::logger> m_logger;

        // Infrastructure
        std::unique_ptr<database::SqliteConnection> m_db;
        std::unique_ptr<database::SqliteDeviceRepository> m_deviceRepo;
        std::unique_ptr<database::SqliteSensorRepository> m_sensorRepo;
        std::unique_ptr<network::mqtt::MqttClient> m_mqttClient;

        // Domain
        std::unique_ptr<devices::DeviceManager> m_deviceManager;
        std::unique_ptr<devices::HeartbeatMonitor> m_heartbeatMonitor;
        std::unique_ptr<automation::RuleRepository> m_ruleRepository;
        std::unique_ptr<automation::RuleEngine> m_ruleEngine;
        std::unique_ptr<security::AuthenticationService> m_authService;

        // Network servers
        std::unique_ptr<network::http::HttpServer> m_httpServer;
        std::unique_ptr<network::websocket::WsServer> m_wsServer;

        // HTTP Controllers (must outlive the HTTP server)
        std::unique_ptr<HealthController> m_healthController;
        std::unique_ptr<AuthController> m_authController;
        std::unique_ptr<DeviceController> m_deviceController;
        std::unique_ptr<SensorController> m_sensorController;
        std::unique_ptr<AutomationController> m_automationController;
        std::unique_ptr<MetricsController> m_metricsController;

        std::atomic<bool> m_running{ false };

        void RegisterHttpRoutes();
        void WireDeviceCallbacks();
        void WireWebSocketHandlers();
        void SeedAdminUser();
    };

} // namespace iot::api

#endif // IOT_API_APPLICATION_HPP
