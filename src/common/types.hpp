/**
 * @file       types.hpp
 * @brief      Shared data types for IoT Dashboard
 * @standard   C++20
 */

#ifndef IOT_COMMON_TYPES_HPP
#define IOT_COMMON_TYPES_HPP

#include <chrono>
#include <concepts>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace iot
{
    // -----------------------------------------------------------------------
    // Type aliases
    // -----------------------------------------------------------------------
    using Timestamp = std::chrono::system_clock::time_point;
    using Duration = std::chrono::milliseconds;
    
    // -----------------------------------------------------------------------
    // Device information
    // -----------------------------------------------------------------------
    struct DeviceInfo
    {
        std::string m_id;
        std::string m_name;
        std::string m_type;          // "sensor", "actuator", "gateway"
        std::string m_protocol;      // "mqtt", "modbus", "opcua"
        std::string m_address;
        bool m_isConnected = false;
        Timestamp m_lastSeen;
    };

    // -----------------------------------------------------------------------
    // Sensor reading
    // -----------------------------------------------------------------------
    struct SensorReading
    {
        std::string m_deviceId;
        std::string m_sensor;
        double m_value;
        std::string m_unit;
        Timestamp m_timestamp;
        std::optional<std::string> m_quality;  // "good", "uncertain", "bad"
    };

    // -----------------------------------------------------------------------
    // Alert definition
    // -----------------------------------------------------------------------
    enum class AlertSeverity : uint8_t
    {
        Info = 0,
        Warning = 1,
        Error = 2,
        Critical = 3
    };

    struct Alert
    {
        std::string m_id;
        std::string m_deviceId;
        AlertSeverity m_severity;
        std::string m_message;
        Timestamp m_timestamp;
        bool m_acknowledged = false;
    };

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------
    struct ServerConfig
    {
        uint16_t m_httpPort = 8080;
        uint16_t m_wsPort = 8081;
        uint32_t m_maxConnections = 1000;
        std::string m_tlsCert;
        std::string m_tlsKey;
    };

    struct DatabaseConfig
    {
        std::string m_path = "./iot.db";
        uint32_t m_retentionDays = 90;
        uint32_t m_cacheSize = 10000;
    };

    struct MqttConfig
    {
        std::string m_broker = "mqtt://localhost:1883";
        std::string m_username;
        std::string m_password;
        std::string m_clientId = "iot-dashboard";
        uint16_t m_keepAlive = 60;
        bool m_cleanSession = true;
    };

    // -----------------------------------------------------------------------
    // Concepts for type constraints
    // -----------------------------------------------------------------------
    
    /// Device type concept
    template<typename T>
    concept Device = requires( T device )
    {
        { device.GetId() } -> std::convertible_to<std::string>;
        { device.GetType() } -> std::convertible_to<std::string>;
        { device.Connect() } -> std::same_as<bool>;
        { device.Disconnect() } -> std::same_as<void>;
        { device.IsConnected() } -> std::same_as<bool>;
    };

    /// Sensor reading producer
    template<typename T>
    concept SensorProducer = requires( T producer )
    {
        { producer.Read() } -> std::convertible_to<std::optional<SensorReading>>;
    };

} // namespace iot

#endif // IOT_COMMON_TYPES_HPP
