/**
 * @file       mqtt_client_test.cpp
 * @brief      Unit/integration tests for MqttClient
 * @standard   C++23
 *
 * Tests are split into two tiers:
 *
 *  1. UNIT (no broker) — exercises the guard logic and state machine
 *     of MqttClient without a live broker.  Always run.
 *
 *  2. INTEGRATION (requires local broker) — marked with DISABLED_ prefix
 *     so they are skipped in CI unless explicitly enabled with
 *       --gtest_also_run_disabled_tests
 *     Start a local broker with: mosquitto -p 1883
 */

#include "network/mqtt/mqtt_client.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

namespace
{
    using namespace iot;
    using namespace iot::network::mqtt;

    MqttClientConfig TestConfig()
    {
        MqttClientConfig cfg;
        cfg.m_brokerUrl = "tcp://localhost:1883";
        cfg.m_clientId = "test-client-" + std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() );
        cfg.m_cleanSession = true;
        cfg.m_connectTimeoutSeconds = 5;
        return cfg;
    }

    // -----------------------------------------------------------------------
    // Unit tests — no broker required
    // -----------------------------------------------------------------------

    TEST( MqttClientUnitTest, PublishWithoutConnectReturnsError )
    {
        MqttClient client( TestConfig() );

        std::array<std::byte, 4> payload{};
        auto result = client.Publish( "test/topic", payload, core::QoS::AtLeastOnce, false );

        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::ConnectionFailed );
    }

    TEST( MqttClientUnitTest, SubscribeWithoutConnectReturnsError )
    {
        MqttClient client( TestConfig() );

        auto result = client.Subscribe( "test/topic", core::QoS::AtLeastOnce );

        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::ConnectionFailed );
    }

    TEST( MqttClientUnitTest, IsConnectedFalseBeforeConnect )
    {
        MqttClient client( TestConfig() );
        EXPECT_FALSE( client.IsConnected() );
    }

    TEST( MqttClientUnitTest, SetMessageCallbackDoesNotThrow )
    {
        MqttClient client( TestConfig() );
        EXPECT_NO_THROW( client.SetMessageCallback( []( std::string_view, std::span<const std::byte> ) {} ) );
    }

    TEST( MqttClientUnitTest, ConnectToNonExistentBrokerReturnsError )
    {
        MqttClientConfig cfg = TestConfig();
        cfg.m_brokerUrl = "tcp://127.0.0.1:19999"; // nothing listening
        cfg.m_connectTimeoutSeconds = 2;

        MqttClient client( cfg );
        auto result = client.Connect();

        // Should fail with timeout or connection refused.
        ASSERT_FALSE( result.has_value() );
        EXPECT_TRUE( result.error() == Error::ConnectionFailed || result.error() == Error::Timeout );
    }

    // -----------------------------------------------------------------------
    // Integration tests — require mosquitto running on localhost:1883
    // Run with: --gtest_also_run_disabled_tests
    // -----------------------------------------------------------------------

    TEST( MqttClientIntegrationTest, DISABLED_ConnectAndDisconnect )
    {
        MqttClient client( TestConfig() );

        auto connectResult = client.Connect();
        ASSERT_TRUE( connectResult.has_value() ) << "Broker not running?";
        EXPECT_TRUE( client.IsConnected() );

        auto disconnectResult = client.Disconnect();
        ASSERT_TRUE( disconnectResult.has_value() );
        EXPECT_FALSE( client.IsConnected() );
    }

    TEST( MqttClientIntegrationTest, DISABLED_SubscribeAndReceiveMessage )
    {
        // Publisher
        MqttClient publisher( TestConfig() );
        ASSERT_TRUE( publisher.Connect().has_value() );

        // Subscriber
        MqttClientConfig subCfg = TestConfig();
        subCfg.m_clientId = subCfg.m_clientId + "-sub";
        MqttClient subscriber( subCfg );
        ASSERT_TRUE( subscriber.Connect().has_value() );

        std::vector<std::string> received;
        std::mutex rxMutex;
        subscriber.SetMessageCallback(
            [&]( std::string_view, std::span<const std::byte> payload )
            {
                std::string msg( reinterpret_cast<const char*>( payload.data() ), payload.size() );
                std::lock_guard lock( rxMutex );
                received.push_back( msg );
            } );

        ASSERT_TRUE( subscriber.Subscribe( "iot/test/integration", core::QoS::AtLeastOnce ).has_value() );

        // Give the broker time to register the subscription.
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

        std::string msg = "hello-iot";
        std::span<const std::byte> payload( reinterpret_cast<const std::byte*>( msg.data() ), msg.size() );
        ASSERT_TRUE( publisher.Publish( "iot/test/integration", payload, core::QoS::AtLeastOnce, false ).has_value() );

        // Wait for delivery.
        std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );

        std::lock_guard lock( rxMutex );
        ASSERT_EQ( received.size(), 1u );
        EXPECT_EQ( received[0], "hello-iot" );
    }

} // namespace
