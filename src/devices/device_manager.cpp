/**
 * @file       device_manager.cpp
 * @brief      DeviceManager implementation
 * @standard   C++23
 */

#include "devices/device_manager.hpp"
#include <algorithm>
#include <chrono>
#include <nlohmann/json.hpp>

namespace iot::devices
{
    DeviceManager::DeviceManager( core::IDeviceRepository& repository,
                                  core::IMqttClient& mqtt,
                                  std::shared_ptr<spdlog::logger> logger )
        : m_repository( repository )
        , m_mqtt( mqtt )
        , m_logger( std::move( logger ) )
    {
        // Install MQTT message handler.
        m_mqtt.SetMessageCallback(
            [this]( std::string_view topic, std::span<const std::byte> payload )
            {
                HandleMqttMessage( topic, payload );
            } );
    }

    // ── Device CRUD ───────────────────────────────────────────────────────

    Result<DeviceInfo> DeviceManager::Register( const RegisterDeviceRequest& req )
    {
        DeviceInfo device;
        device.m_id = req.m_id;
        device.m_name = req.m_name;
        device.m_type = req.m_type;
        device.m_protocol = req.m_protocol;
        device.m_address = req.m_address;
        device.m_isConnected = false;
        device.m_lastSeen = std::chrono::system_clock::now();

        auto result = m_repository.Add( device );
        if ( !result )
        {
            return std::unexpected( result.error() );
        }

        // Subscribe to the device's MQTT topics.
        if ( m_mqtt.IsConnected() )
        {
            std::ignore = m_mqtt.Subscribe( "devices/" + req.m_id + "/status", core::QoS::AtLeastOnce );
            std::ignore = m_mqtt.Subscribe( "devices/" + req.m_id + "/sensors", core::QoS::AtLeastOnce );
            std::ignore = m_mqtt.Subscribe( "devices/" + req.m_id + "/heartbeat", core::QoS::AtMostOnce );
        }

        m_logger->info( "Device registered: id={} name={}", req.m_id, req.m_name );
        return device;
    }

    Result<void> DeviceManager::Unregister( const std::string& deviceId )
    {
        if ( m_mqtt.IsConnected() )
        {
            std::ignore = m_mqtt.Unsubscribe( "devices/" + deviceId + "/status" );
            std::ignore = m_mqtt.Unsubscribe( "devices/" + deviceId + "/sensors" );
            std::ignore = m_mqtt.Unsubscribe( "devices/" + deviceId + "/heartbeat" );
        }

        auto result = m_repository.Remove( deviceId );
        if ( result )
        {
            m_logger->info( "Device unregistered: id={}", deviceId );
        }
        return result;
    }

    Result<DeviceInfo> DeviceManager::Get( const std::string& deviceId )
    {
        return m_repository.GetById( deviceId );
    }

    Result<std::vector<DeviceInfo>> DeviceManager::List( const DeviceFilter& filter )
    {
        auto result = m_repository.GetAll();
        if ( !result )
        {
            return result;
        }

        auto& devices = result.value();

        if ( !filter.m_type.empty() )
        {
            std::erase_if( devices,
                           [&filter]( const DeviceInfo& d )
                           {
                               return d.m_type != filter.m_type;
                           } );
        }

        if ( filter.m_onlineOnly )
        {
            std::erase_if( devices,
                           []( const DeviceInfo& d )
                           {
                               return !d.m_isConnected;
                           } );
        }

        return devices;
    }

    // ── Command dispatch ──────────────────────────────────────────────────

    Result<void> DeviceManager::SendCommand( const std::string& deviceId,
                                             const std::string& command,
                                             const std::string& parametersJson )
    {
        auto deviceResult = m_repository.GetById( deviceId );
        if ( !deviceResult )
        {
            return std::unexpected( deviceResult.error() );
        }

        if ( !deviceResult->m_isConnected )
        {
            return std::unexpected( Error::DeviceOffline );
        }

        std::string json = BuildCommandJson( command, parametersJson );
        std::string topic = BuildCommandTopic( deviceId );

        auto bytes = std::as_bytes( std::span( json ) );
        auto pubResult = m_mqtt.Publish( topic, bytes, core::QoS::AtLeastOnce, false );

        if ( pubResult )
        {
            m_logger->info( "Command sent: device={} cmd={}", deviceId, command );
        }

        return pubResult;
    }

    // ── Callbacks ─────────────────────────────────────────────────────────

    void DeviceManager::OnDeviceStatus( DeviceStatusCallback cb )
    {
        std::lock_guard lock( m_mutex );
        m_statusCallback = std::move( cb );
    }

    void DeviceManager::OnSensorReading( SensorReadingCallback cb )
    {
        std::lock_guard lock( m_mutex );
        m_sensorCallback = std::move( cb );
    }

    // ── MQTT subscription ─────────────────────────────────────────────────

    Result<void> DeviceManager::SubscribeAll()
    {
        auto result = m_repository.GetAll();
        if ( !result )
        {
            return std::unexpected( result.error() );
        }

        for ( const auto& device : result.value() )
        {
            std::ignore = m_mqtt.Subscribe( "devices/" + device.m_id + "/status", core::QoS::AtLeastOnce );
            std::ignore = m_mqtt.Subscribe( "devices/" + device.m_id + "/sensors", core::QoS::AtLeastOnce );
            std::ignore = m_mqtt.Subscribe( "devices/" + device.m_id + "/heartbeat", core::QoS::AtMostOnce );
        }

        return {};
    }

    // ── MQTT message routing ──────────────────────────────────────────────

    void DeviceManager::HandleMqttMessage( std::string_view topic, std::span<const std::byte> payload )
    {
        std::string topicStr( topic );
        std::string payloadStr( reinterpret_cast<const char*>( payload.data() ), payload.size() );

        // Topic format: devices/{id}/{subtopic}
        static constexpr std::string_view kPrefix = "devices/";
        if ( !topicStr.starts_with( kPrefix ) )
        {
            return;
        }

        auto rest = topicStr.substr( kPrefix.size() );
        auto slash = rest.rfind( '/' );
        if ( slash == std::string::npos )
        {
            return;
        }

        std::string deviceId = rest.substr( 0, slash );
        std::string subtopic = rest.substr( slash + 1 );

        if ( subtopic == "status" )
        {
            HandleStatusMessage( deviceId, payloadStr );
        }
        else if ( subtopic == "sensors" )
        {
            HandleSensorMessage( deviceId, payloadStr );
        }
        else if ( subtopic == "heartbeat" )
        {
            // Heartbeat: just update last-seen in the DB.
            auto deviceResult = m_repository.GetById( deviceId );
            if ( deviceResult )
            {
                auto& d = deviceResult.value();
                d.m_lastSeen = std::chrono::system_clock::now();
                std::ignore = m_repository.Update( d );
            }
        }
    }

    void DeviceManager::HandleStatusMessage( const std::string& deviceId, const std::string& json )
    {
        try
        {
            auto j = nlohmann::json::parse( json );
            bool online = j.value( "status", std::string( "offline" ) ) == "online";

            auto deviceResult = m_repository.GetById( deviceId );
            if ( deviceResult )
            {
                auto& d = deviceResult.value();
                d.m_isConnected = online;
                d.m_lastSeen = std::chrono::system_clock::now();

                if ( j.contains( "firmware_version" ) )
                {
                    // firmware_version stored in address field for now;
                    // will move to dedicated column when schema v2 lands.
                }

                std::ignore = m_repository.Update( d );
            }

            DeviceStatusCallback cb;
            {
                std::lock_guard lock( m_mutex );
                cb = m_statusCallback;
            }

            if ( cb )
            {
                cb( deviceId, online );
            }

            m_logger->info( "Device {} is {}", deviceId, online ? "online" : "offline" );
        }
        catch ( const nlohmann::json::exception& e )
        {
            m_logger->warn( "Bad status JSON from {}: {}", deviceId, e.what() );
        }
    }

    void DeviceManager::HandleSensorMessage( const std::string& deviceId, const std::string& json )
    {
        try
        {
            auto j = nlohmann::json::parse( json );

            SensorReading reading;
            reading.m_deviceId = deviceId;
            reading.m_sensor = j.value( "sensor_type", "unknown" );
            reading.m_value = j.value( "value", 0.0 );
            reading.m_unit = j.value( "unit", "" );
            reading.m_timestamp = std::chrono::system_clock::now();

            if ( j.contains( "quality" ) )
            {
                reading.m_quality = j["quality"].get<std::string>();
            }

            SensorReadingCallback cb;
            {
                std::lock_guard lock( m_mutex );
                cb = m_sensorCallback;
            }

            if ( cb )
            {
                cb( reading );
            }
        }
        catch ( const nlohmann::json::exception& e )
        {
            m_logger->warn( "Bad sensor JSON from {}: {}", deviceId, e.what() );
        }
    }

    // ── Static helpers ────────────────────────────────────────────────────

    std::string DeviceManager::BuildCommandTopic( const std::string& deviceId )
    {
        return "devices/" + deviceId + "/commands";
    }

    std::string DeviceManager::BuildCommandJson( const std::string& command, const std::string& paramsJson )
    {
        nlohmann::json j;
        j["command"] = command;
        try
        {
            j["parameters"] = nlohmann::json::parse( paramsJson );
        }
        catch ( ... )
        {
            j["parameters"] = nlohmann::json::object();
        }
        return j.dump();
    }

} // namespace iot::devices
