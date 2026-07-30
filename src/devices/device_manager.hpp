/**
 * @file       device_manager.hpp
 * @brief      Device lifecycle management — register, track, command
 * @standard   C++23
 */

#ifndef IOT_DEVICES_DEVICE_MANAGER_HPP
#define IOT_DEVICES_DEVICE_MANAGER_HPP

#include "common/error.hpp"
#include "common/types.hpp"
#include "core/interfaces/i_device_repository.hpp"
#include "core/interfaces/i_mqtt_client.hpp"
#include <functional>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

namespace iot::devices
{
    struct RegisterDeviceRequest
    {
        std::string m_id;
        std::string m_name;
        std::string m_type;     // "sensor" | "actuator" | "gateway"
        std::string m_protocol; // "mqtt" | "modbus"
        std::string m_address;  // IP or MQTT topic base
    };

    struct DeviceFilter
    {
        std::string m_type; // empty = all types
        bool m_onlineOnly = false;
    };

    /// Fired when a device's online/offline status changes.
    using DeviceStatusCallback = std::function<void( const std::string& deviceId, bool online )>;

    /// Fired when a sensor reading arrives for a device.
    using SensorReadingCallback = std::function<void( const SensorReading& reading )>;

    /**
     * Coordinates device registration, status tracking, and command
     * dispatch.  Sits between the network layer (MQTT) and the
     * persistence layer (IDeviceRepository).
     *
     * Thread-safe: all public methods lock m_mutex.
     */
    class DeviceManager
    {
        public:
        DeviceManager( core::IDeviceRepository& repository,
                       core::IMqttClient& mqtt,
                       std::shared_ptr<spdlog::logger> logger );

        // ── Device CRUD ──────────────────────────────────────────────────

        [[nodiscard]] Result<DeviceInfo> Register( const RegisterDeviceRequest& req );

        [[nodiscard]] Result<void> Unregister( const std::string& deviceId );

        [[nodiscard]] Result<DeviceInfo> Get( const std::string& deviceId );

        [[nodiscard]] Result<std::vector<DeviceInfo>> List( const DeviceFilter& filter = {} );

        // ── Command dispatch ─────────────────────────────────────────────

        /// Publish a JSON command to devices/{id}/commands via MQTT.
        [[nodiscard]] Result<void> SendCommand( const std::string& deviceId,
                                                const std::string& command,
                                                const std::string& parametersJson = "{}" );

        // ── Status / sensor callbacks ─────────────────────────────────────

        void OnDeviceStatus( DeviceStatusCallback cb );
        void OnSensorReading( SensorReadingCallback cb );

        // ── MQTT subscription setup ──────────────────────────────────────

        /// Subscribe to all device MQTT topics.  Call after Connect().
        [[nodiscard]] Result<void> SubscribeAll();

        private:
        core::IDeviceRepository& m_repository;
        core::IMqttClient& m_mqtt;
        std::shared_ptr<spdlog::logger> m_logger;

        mutable std::mutex m_mutex;
        DeviceStatusCallback m_statusCallback;
        SensorReadingCallback m_sensorCallback;

        void HandleMqttMessage( std::string_view topic, std::span<const std::byte> payload );

        void HandleStatusMessage( const std::string& deviceId, const std::string& json );

        void HandleSensorMessage( const std::string& deviceId, const std::string& json );

        static std::string BuildCommandTopic( const std::string& deviceId );
        static std::string BuildCommandJson( const std::string& command, const std::string& paramsJson );
    };

} // namespace iot::devices

#endif // IOT_DEVICES_DEVICE_MANAGER_HPP
