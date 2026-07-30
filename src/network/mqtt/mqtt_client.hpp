/**
 * @file       mqtt_client.hpp
 * @brief      Production MQTT client — Paho C async API behind IMqttClient
 * @standard   C++23
 *
 * Uses MQTTAsync (non-blocking callbacks) so the caller's thread is never
 * blocked waiting for broker I/O.  All public methods are thread-safe.
 */

#ifndef IOT_NETWORK_MQTT_MQTT_CLIENT_HPP
#define IOT_NETWORK_MQTT_MQTT_CLIENT_HPP

#include "common/types.hpp"
#include "core/interfaces/i_mqtt_client.hpp"
#include <MQTTAsync.h>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_set>

namespace iot::network::mqtt
{
    struct MqttClientConfig
    {
        std::string m_brokerUrl = "tcp://localhost:1883";
        std::string m_clientId = "iot-dashboard";
        std::string m_username;
        std::string m_password;
        uint16_t m_keepAliveSeconds = 60;
        bool m_cleanSession = false;
        uint32_t m_connectTimeoutSeconds = 10;
        uint32_t m_maxReconnectDelaySeconds = 30;
        bool m_enableTls = false;
        std::string m_tlsCaFile; // path to CA cert for TLS brokers
    };

    /**
     * Paho-C-backed IMqttClient implementation.
     *
     * Lifecycle:
     *   1. Construct with config.
     *   2. Call Connect() — blocks up to connectTimeoutSeconds.
     *   3. Subscribe / Publish as needed.
     *   4. Call Disconnect() or destroy (RAII disconnect in destructor).
     *
     * The Paho async callbacks run on Paho's internal thread.  The
     * message callback registered via SetMessageCallback() is also
     * invoked on that thread — the caller is responsible for any further
     * thread-safety within the callback.
     */
    class MqttClient final : public core::IMqttClient
    {
        public:
        explicit MqttClient( MqttClientConfig config );
        ~MqttClient() override;

        MqttClient( const MqttClient& ) = delete;
        MqttClient& operator=( const MqttClient& ) = delete;

        // IMqttClient
        Result<void> Connect() override;
        Result<void> Disconnect() override;
        [[nodiscard]] bool IsConnected() const override;
        Result<void> Subscribe( std::string_view topic, core::QoS qos ) override;
        Result<void> Unsubscribe( std::string_view topic ) override;
        Result<void> Publish( std::string_view topic,
                              std::span<const std::byte> payload,
                              core::QoS qos,
                              bool retain ) override;
        void SetMessageCallback( core::MqttMessageCallback callback ) override;

        private:
        MqttClientConfig m_config;
        MQTTAsync m_handle = nullptr;
        std::atomic<bool> m_connected{ false };

        // Subscriptions we need to re-subscribe after reconnect.
        std::mutex m_subsMutex;
        std::unordered_map<std::string, core::QoS> m_subscriptions;

        // Message callback (user-supplied).
        std::mutex m_callbackMutex;
        core::MqttMessageCallback m_messageCallback;

        std::shared_ptr<spdlog::logger> m_logger;

        // Synchronous connect/disconnect helpers.
        std::mutex m_cvMutex;
        std::condition_variable m_cv;
        bool m_connectDone = false;
        bool m_connectSuccess = false;

        void ResubscribeAll();

        // Paho callbacks (static, dispatch to instance via context ptr).
        static void OnConnected( void* context, char* cause );
        static void OnConnectionLost( void* context, char* cause );
        static int OnMessageArrived( void* context, char* topicName, int topicLen, MQTTAsync_message* message );
        static void OnConnectSuccess( void* context, MQTTAsync_successData* response );
        static void OnConnectFailure( void* context, MQTTAsync_failureData* response );
    };

} // namespace iot::network::mqtt

#endif // IOT_NETWORK_MQTT_MQTT_CLIENT_HPP
