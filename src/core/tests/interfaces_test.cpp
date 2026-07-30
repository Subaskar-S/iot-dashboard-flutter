/**
 * @file       interfaces_test.cpp
 * @brief      Unit tests verifying core interfaces are usable polymorphically
 * @standard   C++23
 *
 * These tests instantiate small mock implementations of each core
 * interface (I*Repository, I*Client, I*Service) to confirm:
 *   1. The interfaces compile as proper abstract base classes.
 *   2. They can be used through a base pointer/reference (polymorphism).
 *   3. Result<T> propagation works end-to-end through the interface.
 *
 * Full behavioral tests of real implementations belong to the modules
 * that implement these interfaces (database, network/mqtt, security,
 * automation).
 */

#include "core/interfaces/i_authentication_service.hpp"
#include "core/interfaces/i_device_repository.hpp"
#include "core/interfaces/i_mqtt_client.hpp"
#include "core/interfaces/i_rule_evaluator.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <unordered_map>

namespace
{
    using namespace iot;
    using namespace iot::core;

    // -------------------------------------------------------------------
    // IDeviceRepository mock
    // -------------------------------------------------------------------
    class InMemoryDeviceRepository final : public IDeviceRepository
    {
        public:
        [[nodiscard]] Result<DeviceInfo> GetById( const std::string& deviceId ) override
        {
            auto it = m_devices.find( deviceId );
            if ( it == m_devices.end() )
            {
                return std::unexpected( Error::DeviceNotFound );
            }
            return it->second;
        }

        [[nodiscard]] Result<std::vector<DeviceInfo>> GetAll() override
        {
            std::vector<DeviceInfo> result;
            result.reserve( m_devices.size() );
            for ( const auto& [id, device] : m_devices )
            {
                result.push_back( device );
            }
            return result;
        }

        [[nodiscard]] Result<void> Add( const DeviceInfo& device ) override
        {
            if ( m_devices.contains( device.m_id ) )
            {
                return std::unexpected( Error::InvalidInput );
            }
            m_devices[device.m_id] = device;
            return {};
        }

        [[nodiscard]] Result<void> Update( const DeviceInfo& device ) override
        {
            if ( !m_devices.contains( device.m_id ) )
            {
                return std::unexpected( Error::DeviceNotFound );
            }
            m_devices[device.m_id] = device;
            return {};
        }

        [[nodiscard]] Result<void> Remove( const std::string& deviceId ) override
        {
            if ( m_devices.erase( deviceId ) == 0 )
            {
                return std::unexpected( Error::DeviceNotFound );
            }
            return {};
        }

        private:
        std::unordered_map<std::string, DeviceInfo> m_devices;
    };

    TEST( DeviceRepositoryInterfaceTest, AddThenGetByIdReturnsDevice )
    {
        std::unique_ptr<IDeviceRepository> repo = std::make_unique<InMemoryDeviceRepository>();

        DeviceInfo device;
        device.m_id = "temp-001";
        device.m_name = "Temperature Sensor 1";
        device.m_type = "sensor";

        ASSERT_TRUE( repo->Add( device ).has_value() );

        auto result = repo->GetById( "temp-001" );
        ASSERT_TRUE( result.has_value() );
        EXPECT_EQ( result->m_id, "temp-001" );
        EXPECT_EQ( result->m_name, "Temperature Sensor 1" );
    }

    TEST( DeviceRepositoryInterfaceTest, GetByIdOnMissingDeviceReturnsError )
    {
        std::unique_ptr<IDeviceRepository> repo = std::make_unique<InMemoryDeviceRepository>();

        auto result = repo->GetById( "does-not-exist" );

        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::DeviceNotFound );
    }

    TEST( DeviceRepositoryInterfaceTest, RemoveOnMissingDeviceReturnsError )
    {
        std::unique_ptr<IDeviceRepository> repo = std::make_unique<InMemoryDeviceRepository>();

        auto result = repo->Remove( "does-not-exist" );

        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::DeviceNotFound );
    }

    // -------------------------------------------------------------------
    // IMqttClient mock
    // -------------------------------------------------------------------
    class FakeMqttClient final : public IMqttClient
    {
        public:
        [[nodiscard]] Result<void> Connect() override
        {
            m_connected = true;
            return {};
        }

        [[nodiscard]] Result<void> Disconnect() override
        {
            m_connected = false;
            return {};
        }

        [[nodiscard]] bool IsConnected() const override
        {
            return m_connected;
        }

        [[nodiscard]] Result<void> Subscribe( std::string_view topic, QoS ) override
        {
            if ( !m_connected )
            {
                return std::unexpected( Error::ConnectionFailed );
            }
            m_subscriptions.emplace_back( topic );
            return {};
        }

        [[nodiscard]] Result<void> Unsubscribe( std::string_view topic ) override
        {
            std::erase( m_subscriptions, std::string( topic ) );
            return {};
        }

        [[nodiscard]] Result<void> Publish( std::string_view topic, std::span<const std::byte>, QoS, bool ) override
        {
            if ( !m_connected )
            {
                return std::unexpected( Error::ConnectionFailed );
            }
            m_lastPublishedTopic = topic;
            return {};
        }

        void SetMessageCallback( MqttMessageCallback callback ) override
        {
            m_callback = std::move( callback );
        }

        [[nodiscard]] const std::vector<std::string>& Subscriptions() const
        {
            return m_subscriptions;
        }
        [[nodiscard]] const std::string& LastPublishedTopic() const
        {
            return m_lastPublishedTopic;
        }

        private:
        bool m_connected = false;
        std::vector<std::string> m_subscriptions;
        std::string m_lastPublishedTopic;
        MqttMessageCallback m_callback;
    };

    TEST( MqttClientInterfaceTest, PublishBeforeConnectFails )
    {
        std::unique_ptr<IMqttClient> client = std::make_unique<FakeMqttClient>();

        std::array<std::byte, 1> payload{};
        auto result = client->Publish( "devices/temp-001/sensors", payload, QoS::AtLeastOnce, false );

        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::ConnectionFailed );
    }

    TEST( MqttClientInterfaceTest, SubscribeAfterConnectSucceeds )
    {
        std::unique_ptr<IMqttClient> client = std::make_unique<FakeMqttClient>();

        ASSERT_TRUE( client->Connect().has_value() );
        EXPECT_TRUE( client->IsConnected() );

        auto result = client->Subscribe( "devices/+/sensors", QoS::AtLeastOnce );
        ASSERT_TRUE( result.has_value() );
    }

    // -------------------------------------------------------------------
    // IAuthenticationService mock
    // -------------------------------------------------------------------
    class FakeAuthenticationService final : public IAuthenticationService
    {
        public:
        [[nodiscard]] Result<TokenPair> Login( std::string_view username, std::string_view password ) override
        {
            if ( username != "admin" || password != "correct-password" )
            {
                return std::unexpected( Error::AuthenticationFailed );
            }
            return TokenPair{
                .m_accessToken = "access-token", .m_refreshToken = "refresh-token", .m_expiresInSeconds = 3600 };
        }

        [[nodiscard]] Result<TokenPair> RefreshToken( std::string_view refreshToken ) override
        {
            if ( refreshToken != "refresh-token" )
            {
                return std::unexpected( Error::TokenExpired );
            }
            return TokenPair{ .m_accessToken = "new-access-token", .m_expiresInSeconds = 3600 };
        }

        [[nodiscard]] Result<void> Logout( std::string_view ) override
        {
            return {};
        }

        [[nodiscard]] Result<UserClaims> ValidateToken( std::string_view accessToken ) override
        {
            if ( accessToken != "access-token" )
            {
                return std::unexpected( Error::AuthorizationFailed );
            }
            return UserClaims{ .m_userId = "user-001", .m_username = "admin", .m_role = Role::Admin };
        }
    };

    TEST( AuthenticationServiceInterfaceTest, LoginWithValidCredentialsSucceeds )
    {
        std::unique_ptr<IAuthenticationService> auth = std::make_unique<FakeAuthenticationService>();

        auto result = auth->Login( "admin", "correct-password" );

        ASSERT_TRUE( result.has_value() );
        EXPECT_EQ( result->m_accessToken, "access-token" );
    }

    TEST( AuthenticationServiceInterfaceTest, LoginWithInvalidCredentialsFails )
    {
        std::unique_ptr<IAuthenticationService> auth = std::make_unique<FakeAuthenticationService>();

        auto result = auth->Login( "admin", "wrong-password" );

        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::AuthenticationFailed );
    }

    TEST( AuthenticationServiceInterfaceTest, ValidateTokenRoundTrip )
    {
        std::unique_ptr<IAuthenticationService> auth = std::make_unique<FakeAuthenticationService>();

        auto login = auth->Login( "admin", "correct-password" );
        ASSERT_TRUE( login.has_value() );

        auto claims = auth->ValidateToken( login->m_accessToken );
        ASSERT_TRUE( claims.has_value() );
        EXPECT_EQ( claims->m_role, Role::Admin );
    }

    // -------------------------------------------------------------------
    // IRuleEvaluator mock
    // -------------------------------------------------------------------
    class ThresholdRuleEvaluator final : public IRuleEvaluator
    {
        public:
        [[nodiscard]] Result<std::vector<Action>> Evaluate( const SensorReading& reading ) override
        {
            std::vector<Action> actions;
            if ( reading.m_sensor == "temperature" && reading.m_value > 30.0 )
            {
                actions.push_back( Action{
                    .m_deviceId = "fan-001", .m_command = "turn_on", .m_parameters = { { "speed", "high" } } } );
            }
            return actions;
        }
    };

    TEST( RuleEvaluatorInterfaceTest, HighTemperatureTriggersAction )
    {
        std::unique_ptr<IRuleEvaluator> evaluator = std::make_unique<ThresholdRuleEvaluator>();

        SensorReading reading;
        reading.m_deviceId = "temp-001";
        reading.m_sensor = "temperature";
        reading.m_value = 32.0;
        reading.m_unit = "celsius";

        auto result = evaluator->Evaluate( reading );

        ASSERT_TRUE( result.has_value() );
        ASSERT_EQ( result->size(), 1u );
        EXPECT_EQ( result->front().m_deviceId, "fan-001" );
        EXPECT_EQ( result->front().m_command, "turn_on" );
    }

    TEST( RuleEvaluatorInterfaceTest, NormalTemperatureTriggersNoAction )
    {
        std::unique_ptr<IRuleEvaluator> evaluator = std::make_unique<ThresholdRuleEvaluator>();

        SensorReading reading;
        reading.m_deviceId = "temp-001";
        reading.m_sensor = "temperature";
        reading.m_value = 22.0;
        reading.m_unit = "celsius";

        auto result = evaluator->Evaluate( reading );

        ASSERT_TRUE( result.has_value() );
        EXPECT_TRUE( result->empty() );
    }

} // namespace
