/**
 * @file       mqtt_client.cpp
 * @brief      Paho C async MQTT client implementation
 * @standard   C++23
 */

#include "network/mqtt/mqtt_client.hpp"
#include "common/logging.hpp"
#include <chrono>
#include <cstring>
#include <stdexcept>

namespace iot::network::mqtt
{
    // -----------------------------------------------------------------------
    // Construction / destruction
    // -----------------------------------------------------------------------

    MqttClient::MqttClient( MqttClientConfig config )
        : m_config( std::move( config ) )
        , m_logger( CreateLogger( "MqttClient" ) )
    {
        int rc = MQTTAsync_create( &m_handle, m_config.m_brokerUrl.c_str(), m_config.m_clientId.c_str(),
                                   MQTTCLIENT_PERSISTENCE_NONE, nullptr );

        if ( rc != MQTTASYNC_SUCCESS )
        {
            throw std::runtime_error( "MQTTAsync_create failed: " + std::to_string( rc ) );
        }

        MQTTAsync_setCallbacks( m_handle,
                                this, // context pointer — passed back in every callback
                                OnConnectionLost, OnMessageArrived,
                                nullptr ); // delivery complete (we don't use QoS 2 here)

        // Automatic reconnection built into Paho C.
        MQTTAsync_setConnected( m_handle, this, OnConnected );
    }

    MqttClient::~MqttClient()
    {
        if ( m_connected )
        {
            Disconnect();
        }

        if ( m_handle )
        {
            MQTTAsync_destroy( &m_handle );
        }
    }

    // -----------------------------------------------------------------------
    // Connect / Disconnect
    // -----------------------------------------------------------------------

    Result<void> MqttClient::Connect()
    {
        MQTTAsync_connectOptions opts = MQTTAsync_connectOptions_initializer;
        opts.keepAliveInterval = m_config.m_keepAliveSeconds;
        opts.cleansession = m_config.m_cleanSession ? 1 : 0;
        opts.automaticReconnect = 1;
        opts.minRetryInterval = 1;
        opts.maxRetryInterval = static_cast<int>( m_config.m_maxReconnectDelaySeconds );
        opts.onSuccess = OnConnectSuccess;
        opts.onFailure = OnConnectFailure;
        opts.context = this;

        if ( !m_config.m_username.empty() )
        {
            opts.username = m_config.m_username.c_str();
            opts.password = m_config.m_password.c_str();
        }

        MQTTAsync_SSLOptions sslOpts = MQTTAsync_SSLOptions_initializer;
        if ( m_config.m_enableTls )
        {
            if ( !m_config.m_tlsCaFile.empty() )
            {
                sslOpts.trustStore = m_config.m_tlsCaFile.c_str();
            }
            opts.ssl = &sslOpts;
        }

        {
            std::unique_lock lock( m_cvMutex );
            m_connectDone = false;
            m_connectSuccess = false;
        }

        int rc = MQTTAsync_connect( m_handle, &opts );
        if ( rc != MQTTASYNC_SUCCESS )
        {
            m_logger->error( "MQTTAsync_connect returned {}", rc );
            return std::unexpected( Error::ConnectionFailed );
        }

        // Block until Paho fires OnConnectSuccess/Failure or timeout.
        std::unique_lock lock( m_cvMutex );
        bool timedOut = !m_cv.wait_for( lock, std::chrono::seconds( m_config.m_connectTimeoutSeconds ),
                                        [this]
                                        {
                                            return m_connectDone;
                                        } );

        if ( timedOut )
        {
            m_logger->error( "MQTT connect timed out after {}s", m_config.m_connectTimeoutSeconds );
            return std::unexpected( Error::Timeout );
        }

        if ( !m_connectSuccess )
        {
            return std::unexpected( Error::ConnectionFailed );
        }

        return {};
    }

    Result<void> MqttClient::Disconnect()
    {
        if ( !m_connected )
        {
            return {};
        }

        MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
        opts.timeout = 3000; // ms

        MQTTAsync_disconnect( m_handle, &opts );
        m_connected = false;
        m_logger->info( "MQTT disconnected" );
        return {};
    }

    bool MqttClient::IsConnected() const
    {
        return m_connected.load();
    }

    // -----------------------------------------------------------------------
    // Subscribe / Unsubscribe
    // -----------------------------------------------------------------------

    Result<void> MqttClient::Subscribe( std::string_view topic, core::QoS qos )
    {
        if ( !m_connected )
        {
            return std::unexpected( Error::ConnectionFailed );
        }

        MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

        int rc = MQTTAsync_subscribe( m_handle, std::string( topic ).c_str(), static_cast<int>( qos ), &opts );

        if ( rc != MQTTASYNC_SUCCESS )
        {
            m_logger->error( "Subscribe failed for topic '{}': {}", topic, rc );
            return std::unexpected( Error::MqttError );
        }

        std::lock_guard lock( m_subsMutex );
        m_subscriptions[std::string( topic )] = qos;

        m_logger->debug( "Subscribed to '{}'", topic );
        return {};
    }

    Result<void> MqttClient::Unsubscribe( std::string_view topic )
    {
        MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
        MQTTAsync_unsubscribe( m_handle, std::string( topic ).c_str(), &opts );

        std::lock_guard lock( m_subsMutex );
        m_subscriptions.erase( std::string( topic ) );

        return {};
    }

    // -----------------------------------------------------------------------
    // Publish
    // -----------------------------------------------------------------------

    Result<void> MqttClient::Publish( std::string_view topic,
                                      std::span<const std::byte> payload,
                                      core::QoS qos,
                                      bool retain )
    {
        if ( !m_connected )
        {
            return std::unexpected( Error::ConnectionFailed );
        }

        MQTTAsync_message msg = MQTTAsync_message_initializer;
        msg.payload = const_cast<void*>( static_cast<const void*>( payload.data() ) );
        msg.payloadlen = static_cast<int>( payload.size() );
        msg.qos = static_cast<int>( qos );
        msg.retained = retain ? 1 : 0;

        MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

        int rc = MQTTAsync_sendMessage( m_handle, std::string( topic ).c_str(), &msg, &opts );

        if ( rc != MQTTASYNC_SUCCESS )
        {
            m_logger->error( "Publish failed for topic '{}': {}", topic, rc );
            return std::unexpected( Error::MqttError );
        }

        return {};
    }

    // -----------------------------------------------------------------------
    // Callback registration
    // -----------------------------------------------------------------------

    void MqttClient::SetMessageCallback( core::MqttMessageCallback callback )
    {
        std::lock_guard lock( m_callbackMutex );
        m_messageCallback = std::move( callback );
    }

    // -----------------------------------------------------------------------
    // Reconnect helper
    // -----------------------------------------------------------------------

    void MqttClient::ResubscribeAll()
    {
        std::lock_guard lock( m_subsMutex );
        for ( const auto& [topic, qos] : m_subscriptions )
        {
            MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
            MQTTAsync_subscribe( m_handle, topic.c_str(), static_cast<int>( qos ), &opts );
            m_logger->debug( "Re-subscribed to '{}'", topic );
        }
    }

    // -----------------------------------------------------------------------
    // Static Paho callbacks
    // -----------------------------------------------------------------------

    void MqttClient::OnConnected( void* context, char* /* cause */ )
    {
        auto* self = static_cast<MqttClient*>( context );
        self->m_connected = true;
        self->m_logger->info( "MQTT connected to {}", self->m_config.m_brokerUrl );
        self->ResubscribeAll();
    }

    void MqttClient::OnConnectionLost( void* context, char* cause )
    {
        auto* self = static_cast<MqttClient*>( context );
        self->m_connected = false;
        self->m_logger->warn( "MQTT connection lost: {}", cause ? cause : "unknown" );
        // Paho automaticReconnect will attempt to reconnect.
    }

    int MqttClient::OnMessageArrived( void* context, char* topicName, int topicLen, MQTTAsync_message* message )
    {
        auto* self = static_cast<MqttClient*>( context );

        std::string_view topic( topicName, topicLen > 0 ? static_cast<size_t>( topicLen ) : std::strlen( topicName ) );

        const auto* bytes = static_cast<const std::byte*>( message->payload );
        std::span<const std::byte> payload( bytes, static_cast<size_t>( message->payloadlen ) );

        {
            std::lock_guard lock( self->m_callbackMutex );
            if ( self->m_messageCallback )
            {
                self->m_messageCallback( topic, payload );
            }
        }

        MQTTAsync_freeMessage( &message );
        MQTTAsync_free( topicName );
        return 1; // 1 = message ownership taken
    }

    void MqttClient::OnConnectSuccess( void* context, MQTTAsync_successData* /* response */ )
    {
        auto* self = static_cast<MqttClient*>( context );
        {
            std::lock_guard lock( self->m_cvMutex );
            self->m_connectDone = true;
            self->m_connectSuccess = true;
        }
        self->m_cv.notify_all();
    }

    void MqttClient::OnConnectFailure( void* context, MQTTAsync_failureData* response )
    {
        auto* self = static_cast<MqttClient*>( context );
        self->m_logger->error( "MQTT connect failed: code={}", response ? response->code : -1 );
        {
            std::lock_guard lock( self->m_cvMutex );
            self->m_connectDone = true;
            self->m_connectSuccess = false;
        }
        self->m_cv.notify_all();
    }

} // namespace iot::network::mqtt
