/**
 * @file       device_manager_test.cpp
 * @brief      Unit tests for DeviceManager using GMock stubs
 * @standard   C++23
 */

#include "common/logging.hpp"
#include "devices/device_manager.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace
{
    using namespace iot;
    using namespace iot::devices;
    using namespace iot::core;
    using ::testing::_;
    using ::testing::Return;

    // ── GMock stubs ─────────────────────────────────────────────────────

    class MockDeviceRepository : public IDeviceRepository
    {
        public:
        MOCK_METHOD( Result<DeviceInfo>, GetById, (const std::string&), ( override ) );
        MOCK_METHOD( Result<std::vector<DeviceInfo>>, GetAll, (), ( override ) );
        MOCK_METHOD( Result<void>, Add, (const DeviceInfo&), ( override ) );
        MOCK_METHOD( Result<void>, Update, (const DeviceInfo&), ( override ) );
        MOCK_METHOD( Result<void>, Remove, (const std::string&), ( override ) );
    };

    class MockMqttClient : public IMqttClient
    {
        public:
        MOCK_METHOD( Result<void>, Connect, (), ( override ) );
        MOCK_METHOD( Result<void>, Disconnect, (), ( override ) );
        MOCK_METHOD( bool, IsConnected, (), ( const, override ) );
        MOCK_METHOD( Result<void>, Subscribe, ( std::string_view, QoS ), ( override ) );
        MOCK_METHOD( Result<void>, Unsubscribe, ( std::string_view ), ( override ) );
        MOCK_METHOD( Result<void>, Publish, (std::string_view, std::span<const std::byte>, QoS, bool), ( override ) );
        MOCK_METHOD( void, SetMessageCallback, ( MqttMessageCallback ), ( override ) );
    };

    // ── Fixture ──────────────────────────────────────────────────────────

    class DeviceManagerTest : public ::testing::Test
    {
        protected:
        void SetUp() override
        {
            ON_CALL( m_mqtt, IsConnected() ).WillByDefault( Return( false ) );
            ON_CALL( m_mqtt, SetMessageCallback( _ ) ).WillByDefault( Return() );
            EXPECT_CALL( m_mqtt, SetMessageCallback( _ ) ).Times( 1 );

            m_manager = std::make_unique<DeviceManager>( m_repo, m_mqtt, CreateLogger( "DeviceManagerTest" ) );
        }

        MockDeviceRepository m_repo;
        MockMqttClient m_mqtt;
        std::unique_ptr<DeviceManager> m_manager;

        static RegisterDeviceRequest MakeRequest( std::string id = "sensor-001" )
        {
            return { .m_id = std::move( id ),
                     .m_name = "Test Sensor",
                     .m_type = "sensor",
                     .m_protocol = "mqtt",
                     .m_address = "192.168.1.100" };
        }

        static DeviceInfo MakeDevice( std::string id = "sensor-001" )
        {
            DeviceInfo d;
            d.m_id = std::move( id );
            d.m_name = "Test Sensor";
            d.m_type = "sensor";
            d.m_protocol = "mqtt";
            d.m_isConnected = false;
            return d;
        }
    };

    // ── Register ─────────────────────────────────────────────────────────

    TEST_F( DeviceManagerTest, RegisterSuccessWhenRepoAddsOk )
    {
        EXPECT_CALL( m_repo, Add( _ ) ).WillOnce( Return( Result<void>{} ) );

        auto result = m_manager->Register( MakeRequest() );

        ASSERT_TRUE( result.has_value() );
        EXPECT_EQ( result->m_id, "sensor-001" );
    }

    TEST_F( DeviceManagerTest, RegisterFailsWhenRepoFails )
    {
        EXPECT_CALL( m_repo, Add( _ ) ).WillOnce( Return( std::unexpected( Error::InvalidInput ) ) );

        auto result = m_manager->Register( MakeRequest() );

        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::InvalidInput );
    }

    TEST_F( DeviceManagerTest, RegisterSubscribesMqttWhenConnected )
    {
        ON_CALL( m_mqtt, IsConnected() ).WillByDefault( Return( true ) );
        EXPECT_CALL( m_repo, Add( _ ) ).WillOnce( Return( Result<void>{} ) );
        EXPECT_CALL( m_mqtt, Subscribe( _, _ ) ).Times( 3 ).WillRepeatedly( Return( Result<void>{} ) );

        auto result = m_manager->Register( MakeRequest() );
        EXPECT_TRUE( result.has_value() );
    }

    // ── Unregister ───────────────────────────────────────────────────────

    TEST_F( DeviceManagerTest, UnregisterCallsRepoRemove )
    {
        EXPECT_CALL( m_repo, Remove( "sensor-001" ) ).WillOnce( Return( Result<void>{} ) );

        auto result = m_manager->Unregister( "sensor-001" );
        EXPECT_TRUE( result.has_value() );
    }

    TEST_F( DeviceManagerTest, UnregisterPropagatesRepoError )
    {
        EXPECT_CALL( m_repo, Remove( _ ) ).WillOnce( Return( std::unexpected( Error::DeviceNotFound ) ) );

        auto result = m_manager->Unregister( "ghost" );
        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::DeviceNotFound );
    }

    // ── Get / List ───────────────────────────────────────────────────────

    TEST_F( DeviceManagerTest, GetDelegatesToRepository )
    {
        EXPECT_CALL( m_repo, GetById( "sensor-001" ) ).WillOnce( Return( MakeDevice() ) );

        auto result = m_manager->Get( "sensor-001" );
        ASSERT_TRUE( result.has_value() );
        EXPECT_EQ( result->m_id, "sensor-001" );
    }

    TEST_F( DeviceManagerTest, ListFiltersOnlineOnly )
    {
        DeviceInfo online = MakeDevice( "dev-online" );
        online.m_isConnected = true;
        DeviceInfo offline = MakeDevice( "dev-offline" );
        offline.m_isConnected = false;

        EXPECT_CALL( m_repo, GetAll() ).WillOnce( Return( std::vector<DeviceInfo>{ online, offline } ) );

        DeviceFilter filter;
        filter.m_onlineOnly = true;

        auto result = m_manager->List( filter );
        ASSERT_TRUE( result.has_value() );
        ASSERT_EQ( result->size(), 1u );
        EXPECT_EQ( result->front().m_id, "dev-online" );
    }

    TEST_F( DeviceManagerTest, ListFiltersByType )
    {
        DeviceInfo sensor = MakeDevice( "s-001" );
        sensor.m_type = "sensor";
        DeviceInfo actuator = MakeDevice( "a-001" );
        actuator.m_type = "actuator";

        EXPECT_CALL( m_repo, GetAll() ).WillOnce( Return( std::vector<DeviceInfo>{ sensor, actuator } ) );

        DeviceFilter filter;
        filter.m_type = "actuator";

        auto result = m_manager->List( filter );
        ASSERT_TRUE( result.has_value() );
        ASSERT_EQ( result->size(), 1u );
        EXPECT_EQ( result->front().m_id, "a-001" );
    }

    // ── SendCommand ──────────────────────────────────────────────────────

    TEST_F( DeviceManagerTest, SendCommandFailsWhenDeviceOffline )
    {
        DeviceInfo offline = MakeDevice();
        offline.m_isConnected = false;

        EXPECT_CALL( m_repo, GetById( _ ) ).WillOnce( Return( offline ) );

        auto result = m_manager->SendCommand( "sensor-001", "reboot" );
        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::DeviceOffline );
    }

    TEST_F( DeviceManagerTest, SendCommandPublishesWhenDeviceOnline )
    {
        DeviceInfo online = MakeDevice();
        online.m_isConnected = true;

        EXPECT_CALL( m_repo, GetById( _ ) ).WillOnce( Return( online ) );
        EXPECT_CALL( m_mqtt, Publish( _, _, _, _ ) ).WillOnce( Return( Result<void>{} ) );

        auto result = m_manager->SendCommand( "sensor-001", "reboot", "{}" );
        EXPECT_TRUE( result.has_value() );
    }

    TEST_F( DeviceManagerTest, SendCommandFailsWhenDeviceNotFound )
    {
        EXPECT_CALL( m_repo, GetById( _ ) ).WillOnce( Return( std::unexpected( Error::DeviceNotFound ) ) );

        auto result = m_manager->SendCommand( "ghost", "reboot" );
        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::DeviceNotFound );
    }

} // namespace
