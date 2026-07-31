/**
 * @file       application.cpp
 * @brief      Composition root — wires all modules together
 * @standard   C++23
 */

#include "api/application.hpp"
#include "common/logging.hpp"
#include <nlohmann/json.hpp>

namespace iot::api
{
    Application::Application( AppConfig config )
        : m_config( std::move( config ) )
        , m_logger( CreateLogger( "Application" ) )
    {
    }

    Result<void> Application::Initialize()
    {
        m_logger->info( "Initializing IoT Dashboard..." );

        // ── Database ──────────────────────────────────────────────────────
        try
        {
            m_db = std::make_unique<database::SqliteConnection>( m_config.m_dbPath );
        }
        catch ( const std::exception& e )
        {
            m_logger->error( "Failed to open database '{}': {}", m_config.m_dbPath, e.what() );
            return std::unexpected( Error::DatabaseError );
        }

        database::MigrationManager migrations( *m_db );
        for ( auto& m : database::GetMigrations() )
        {
            migrations.Register( std::move( m ) );
        }

        auto migrateResult = migrations.Migrate();
        if ( !migrateResult )
        {
            m_logger->error( "Database migration failed" );
            return migrateResult;
        }

        m_deviceRepo = std::make_unique<database::SqliteDeviceRepository>( *m_db );
        m_sensorRepo = std::make_unique<database::SqliteSensorRepository>( *m_db );
        m_logger->info( "Database ready: {}", m_config.m_dbPath );

        // ── Security ──────────────────────────────────────────────────────
        security::SecurityConfig secCfg;
        secCfg.m_jwtSecret = m_config.m_jwtSecret;
        secCfg.m_pbkdf2Iterations = m_config.m_pbkdf2Iterations;
        m_authService = std::make_unique<security::AuthenticationService>( secCfg );
        SeedAdminUser();

        // ── MQTT ──────────────────────────────────────────────────────────
        network::mqtt::MqttClientConfig mqttCfg;
        mqttCfg.m_brokerUrl = m_config.m_mqttBroker;
        mqttCfg.m_clientId = m_config.m_mqttClientId;
        m_mqttClient = std::make_unique<network::mqtt::MqttClient>( mqttCfg );

        // ── Domain ────────────────────────────────────────────────────────
        m_deviceManager =
            std::make_unique<devices::DeviceManager>( *m_deviceRepo, *m_mqttClient, CreateLogger( "DeviceManager" ) );

        m_heartbeatMonitor =
            std::make_unique<devices::HeartbeatMonitor>( std::chrono::seconds( m_config.m_heartbeatPollSeconds ) );

        m_heartbeatMonitor->SetTimeoutCallback(
            [this]( const std::string& deviceId )
            {
                m_logger->warn( "Heartbeat timeout: device={}", deviceId );
                m_wsServer->Broadcast( nlohmann::json{
                    { "type", "device_status" }, { "data", { { "device_id", deviceId }, { "status", "offline" } } } }
                                           .dump() );
            } );

        m_ruleRepository = std::make_unique<automation::RuleRepository>();
        m_ruleEngine = std::make_unique<automation::RuleEngine>( *m_ruleRepository );

        // ── HTTP server ───────────────────────────────────────────────────
        network::http::HttpServerConfig httpCfg;
        httpCfg.m_host = m_config.m_httpHost;
        httpCfg.m_port = m_config.m_httpPort;
        httpCfg.m_threadCount = m_config.m_httpThreads;
        m_httpServer = std::make_unique<network::http::HttpServer>( httpCfg );

        // ── WebSocket server ──────────────────────────────────────────────
        network::websocket::WsServerConfig wsCfg;
        wsCfg.m_port = m_config.m_wsPort;
        wsCfg.m_threadCount = m_config.m_wsThreads;
        m_wsServer = std::make_unique<network::websocket::WsServer>( wsCfg );

        RegisterHttpRoutes();
        WireDeviceCallbacks();
        WireWebSocketHandlers();

        m_logger->info( "Initialization complete" );
        return {};
    }

    Result<void> Application::Start()
    {
        // Connect MQTT (best-effort — system runs without broker).
        auto mqttResult = m_mqttClient->Connect();
        if ( mqttResult )
        {
            m_logger->info( "MQTT connected to {}", m_config.m_mqttBroker );
            std::ignore = m_deviceManager->SubscribeAll();
        }
        else
        {
            m_logger->warn( "MQTT broker not reachable — running without MQTT" );
        }

        // Start heartbeat monitor.
        m_heartbeatMonitor->Start();

        // Start WebSocket server.
        auto wsResult = m_wsServer->Start();
        if ( !wsResult )
        {
            m_logger->error( "Failed to start WebSocket server on port {}", m_config.m_wsPort );
            return wsResult;
        }

        // Start HTTP server.
        auto httpResult = m_httpServer->Start();
        if ( !httpResult )
        {
            m_logger->error( "Failed to start HTTP server on port {}", m_config.m_httpPort );
            return httpResult;
        }

        m_running = true;
        m_logger->info( "IoT Dashboard running — HTTP:{} WS:{}", m_config.m_httpPort, m_config.m_wsPort );
        return {};
    }

    void Application::Stop()
    {
        if ( !m_running.exchange( false ) )
        {
            return;
        }

        m_heartbeatMonitor->Stop();
        m_httpServer->Stop();
        m_wsServer->Stop();
        std::ignore = m_mqttClient->Disconnect();
        m_logger->info( "IoT Dashboard stopped" );
    }

    bool Application::IsRunning() const noexcept
    {
        return m_running.load();
    }

    void Application::RegisterHttpRoutes()
    {
        auto& router = m_httpServer->Router();

        HealthController health( "1.0.0" );
        health.Register( router );

        AuthController auth( *m_authService );
        auth.Register( router );

        DeviceController devices( *m_deviceManager, *m_authService );
        devices.Register( router );

        SensorController sensors( *m_sensorRepo, *m_authService );
        sensors.Register( router );

        AutomationController automation( *m_ruleEngine, *m_authService );
        automation.Register( router );

        MetricsController metrics( *m_wsServer, *m_authService );
        metrics.Register( router );
    }

    void Application::WireDeviceCallbacks()
    {
        // Forward device status changes to all WebSocket clients.
        m_deviceManager->OnDeviceStatus(
            [this]( const std::string& deviceId, bool online )
            {
                nlohmann::json msg = { { "type", "device_status" },
                                       { "data", { { "device_id", deviceId }, { "online", online } } } };
                m_wsServer->Broadcast( msg.dump() );
                m_wsServer->BroadcastToTopic( "devices", msg.dump() );
            } );

        // Forward sensor readings to subscribers + run automation.
        m_deviceManager->OnSensorReading(
            [this]( const SensorReading& reading )
            {
                nlohmann::json msg = { { "type", "sensor_data" },
                                       { "data",
                                         { { "device_id", reading.m_deviceId },
                                           { "sensor_type", reading.m_sensor },
                                           { "value", reading.m_value },
                                           { "unit", reading.m_unit } } } };
                m_wsServer->BroadcastToTopic( "sensors", msg.dump() );
                m_wsServer->BroadcastToTopic( "sensors/" + reading.m_deviceId, msg.dump() );

                // Evaluate automation rules.
                auto actions = m_ruleEngine->Evaluate( reading );
                if ( actions && !actions->empty() )
                {
                    for ( const auto& action : actions.value() )
                    {
                        std::ignore = m_deviceManager->SendCommand( action.m_deviceId, action.m_command,
                                                                    action.m_parameters.dump() );
                    }
                }
            } );
    }

    void Application::WireWebSocketHandlers()
    {
        // Parse subscribe/unsubscribe messages from WS clients.
        m_wsServer->OnMessage(
            [this]( const network::websocket::WsMessage& msg )
            {
                try
                {
                    auto j = nlohmann::json::parse( msg.m_payload );
                    std::string type = j.value( "type", std::string{} );

                    if ( type == "subscribe" )
                    {
                        std::string topic = j.value( "topic", std::string{} );
                        if ( !topic.empty() )
                        {
                            m_wsServer->Subscribe( msg.m_connectionId, topic );
                        }
                    }
                    else if ( type == "unsubscribe" )
                    {
                        std::string topic = j.value( "topic", std::string{} );
                        if ( !topic.empty() )
                        {
                            m_wsServer->Unsubscribe( msg.m_connectionId, topic );
                        }
                    }
                    else if ( type == "ping" )
                    {
                        m_wsServer->Send( msg.m_connectionId, R"({"type":"pong"})" );
                    }
                }
                catch ( ... )
                {
                    // Ignore malformed messages.
                }
            } );
    }

    void Application::SeedAdminUser()
    {
        // Seed a default admin if none registered yet.
        // In production, admin credentials should be set via env/config.
        std::ignore = m_authService->RegisterUser( "admin-001", "admin", "admin123", core::Role::Admin );
    }

} // namespace iot::api
