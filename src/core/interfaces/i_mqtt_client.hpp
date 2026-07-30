/**
 * @file       i_mqtt_client.hpp
 * @brief      Port for MQTT publish/subscribe operations
 * @standard   C++23
 */

#ifndef IOT_CORE_I_MQTT_CLIENT_HPP
#define IOT_CORE_I_MQTT_CLIENT_HPP

#include "common/error.hpp"
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>
#include <vector>

namespace iot::core
{
    /// MQTT Quality of Service level.
    enum class QoS : uint8_t
    {
        AtMostOnce = 0,
        AtLeastOnce = 1,
        ExactlyOnce = 2
    };

    /// Callback invoked when a message arrives on a subscribed topic.
    using MqttMessageCallback = std::function<void( std::string_view topic, std::span<const std::byte> payload )>;

    /**
     * Abstract MQTT client boundary.
     *
     * Implementations (e.g. a Paho-based client) live in network/mqtt.
     * Business logic (device manager, automation engine) depends only on
     * this interface so it can be tested with a mock broker.
     */
    class IMqttClient
    {
        public:
        virtual ~IMqttClient() = default;

        [[nodiscard]] virtual Result<void> Connect() = 0;

        [[nodiscard]] virtual Result<void> Disconnect() = 0;

        [[nodiscard]] virtual bool IsConnected() const = 0;

        [[nodiscard]] virtual Result<void> Subscribe( std::string_view topic, QoS qos ) = 0;

        [[nodiscard]] virtual Result<void> Unsubscribe( std::string_view topic ) = 0;

        [[nodiscard]] virtual Result<void> Publish( std::string_view topic,
                                                    std::span<const std::byte> payload,
                                                    QoS qos,
                                                    bool retain ) = 0;

        virtual void SetMessageCallback( MqttMessageCallback callback ) = 0;
    };

} // namespace iot::core

#endif // IOT_CORE_I_MQTT_CLIENT_HPP
