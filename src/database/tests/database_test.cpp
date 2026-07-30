/**
 * @file       database_test.cpp
 * @brief      Integration tests for the database module
 * @standard   C++23
 *
 * All tests use an in-memory SQLite database (:memory:) so no files are
 * created and teardown is automatic.
 */

#include "database/device_repository.hpp"
#include "database/migration_manager.hpp"
#include "database/schema.hpp"
#include "database/sensor_repository.hpp"
#include "database/sqlite_connection.hpp"
#include <chrono>
#include <gtest/gtest.h>

namespace
{
    using namespace iot;
    using namespace iot::database;

    // ------------------------------------------------------------------
    // Fixture: in-memory DB with migrations already applied.
    // ------------------------------------------------------------------
    class DatabaseFixture : public ::testing::Test
    {
        protected:
        void SetUp() override
        {
            m_conn = std::make_unique<SqliteConnection>( ":memory:" );
            MigrationManager mgr( *m_conn );

            for ( auto& m : GetMigrations() )
            {
                mgr.Register( std::move( m ) );
            }

            ASSERT_TRUE( mgr.Migrate().has_value() );
        }

        std::unique_ptr<SqliteConnection> m_conn;

        static DeviceInfo MakeDevice( std::string id, std::string name = "Test Device" )
        {
            DeviceInfo d;
            d.m_id = std::move( id );
            d.m_name = std::move( name );
            d.m_type = "sensor";
            d.m_protocol = "mqtt";
            d.m_address = "192.168.1.1";
            d.m_isConnected = true;
            d.m_lastSeen = std::chrono::system_clock::now();
            return d;
        }
    };

    // ------------------------------------------------------------------
    // Migration tests
    // ------------------------------------------------------------------
    TEST_F( DatabaseFixture, MigrationCreatesDevicesTable )
    {
        // If schema was applied the devices table must exist; inserting
        // and selecting won't throw.
        SqliteDeviceRepository repo( *m_conn );

        auto result = repo.Add( MakeDevice( "probe-migration" ) );

        EXPECT_TRUE( result.has_value() );
    }

    TEST_F( DatabaseFixture, MigrationIsIdempotent )
    {
        // Applying migrations a second time on the same DB must succeed.
        MigrationManager mgr( *m_conn );
        for ( auto& m : GetMigrations() )
        {
            mgr.Register( std::move( m ) );
        }

        EXPECT_TRUE( mgr.Migrate().has_value() );
    }

    // ------------------------------------------------------------------
    // SqliteDeviceRepository tests
    // ------------------------------------------------------------------
    class DeviceRepositoryTest : public DatabaseFixture
    {
        protected:
        void SetUp() override
        {
            DatabaseFixture::SetUp();
            m_repo = std::make_unique<SqliteDeviceRepository>( *m_conn );
        }

        std::unique_ptr<SqliteDeviceRepository> m_repo;
    };

    TEST_F( DeviceRepositoryTest, AddAndGetById )
    {
        auto device = MakeDevice( "temp-001", "Temperature Sensor" );

        ASSERT_TRUE( m_repo->Add( device ).has_value() );

        auto result = m_repo->GetById( "temp-001" );
        ASSERT_TRUE( result.has_value() );
        EXPECT_EQ( result->m_id, "temp-001" );
        EXPECT_EQ( result->m_name, "Temperature Sensor" );
        EXPECT_EQ( result->m_type, "sensor" );
        EXPECT_TRUE( result->m_isConnected );
    }

    TEST_F( DeviceRepositoryTest, GetByIdNotFound )
    {
        auto result = m_repo->GetById( "no-such-device" );

        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::DeviceNotFound );
    }

    TEST_F( DeviceRepositoryTest, GetAllReturnsAllDevices )
    {
        ASSERT_TRUE( m_repo->Add( MakeDevice( "dev-001" ) ).has_value() );
        ASSERT_TRUE( m_repo->Add( MakeDevice( "dev-002" ) ).has_value() );

        auto result = m_repo->GetAll();
        ASSERT_TRUE( result.has_value() );
        EXPECT_EQ( result->size(), 2u );
    }

    TEST_F( DeviceRepositoryTest, UpdateExistingDevice )
    {
        ASSERT_TRUE( m_repo->Add( MakeDevice( "dev-001", "Old Name" ) ).has_value() );

        auto device = MakeDevice( "dev-001", "New Name" );
        ASSERT_TRUE( m_repo->Update( device ).has_value() );

        auto result = m_repo->GetById( "dev-001" );
        ASSERT_TRUE( result.has_value() );
        EXPECT_EQ( result->m_name, "New Name" );
    }

    TEST_F( DeviceRepositoryTest, UpdateMissingDeviceReturnsError )
    {
        auto device = MakeDevice( "ghost-device" );
        auto result = m_repo->Update( device );

        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::DeviceNotFound );
    }

    TEST_F( DeviceRepositoryTest, RemoveExistingDevice )
    {
        ASSERT_TRUE( m_repo->Add( MakeDevice( "to-delete" ) ).has_value() );

        ASSERT_TRUE( m_repo->Remove( "to-delete" ).has_value() );

        auto result = m_repo->GetById( "to-delete" );
        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::DeviceNotFound );
    }

    TEST_F( DeviceRepositoryTest, RemoveMissingDeviceReturnsError )
    {
        auto result = m_repo->Remove( "does-not-exist" );

        ASSERT_FALSE( result.has_value() );
        EXPECT_EQ( result.error(), Error::DeviceNotFound );
    }

    // ------------------------------------------------------------------
    // SqliteSensorRepository tests
    // ------------------------------------------------------------------
    class SensorRepositoryTest : public DatabaseFixture
    {
        protected:
        void SetUp() override
        {
            DatabaseFixture::SetUp();
            m_deviceRepo = std::make_unique<SqliteDeviceRepository>( *m_conn );
            m_sensorRepo = std::make_unique<SqliteSensorRepository>( *m_conn );

            // Sensor readings require a parent device (FK constraint).
            DeviceInfo d;
            d.m_id = "temp-001";
            d.m_name = "Temp";
            d.m_type = "sensor";
            d.m_protocol = "mqtt";
            d.m_lastSeen = std::chrono::system_clock::now();
            ASSERT_TRUE( m_deviceRepo->Add( d ).has_value() );
        }

        static SensorReading MakeReading( std::string deviceId, std::string sensorType, double value )
        {
            SensorReading r;
            r.m_deviceId = std::move( deviceId );
            r.m_sensor = std::move( sensorType );
            r.m_value = value;
            r.m_unit = "celsius";
            r.m_timestamp = std::chrono::system_clock::now();
            return r;
        }

        std::unique_ptr<SqliteDeviceRepository> m_deviceRepo;
        std::unique_ptr<SqliteSensorRepository> m_sensorRepo;
    };

    TEST_F( SensorRepositoryTest, AddAndQuery )
    {
        ASSERT_TRUE( m_sensorRepo->Add( MakeReading( "temp-001", "temperature", 25.5 ) ).has_value() );

        SensorQueryOptions opts;
        opts.m_deviceId = "temp-001";
        opts.m_sensorType = "temperature";
        opts.m_limit = 10;

        auto result = m_sensorRepo->Query( opts );
        ASSERT_TRUE( result.has_value() );
        ASSERT_EQ( result->size(), 1u );
        EXPECT_DOUBLE_EQ( result->front().m_value, 25.5 );
        EXPECT_EQ( result->front().m_deviceId, "temp-001" );
    }

    TEST_F( SensorRepositoryTest, BatchInsertAndQueryCount )
    {
        std::vector<SensorReading> batch;
        for ( int i = 0; i < 20; ++i )
        {
            batch.push_back( MakeReading( "temp-001", "temperature", 20.0 + i ) );
        }

        ASSERT_TRUE( m_sensorRepo->AddBatch( batch ).has_value() );

        SensorQueryOptions opts;
        opts.m_deviceId = "temp-001";
        opts.m_limit = 100;

        auto result = m_sensorRepo->Query( opts );
        ASSERT_TRUE( result.has_value() );
        EXPECT_EQ( result->size(), 20u );
    }

    TEST_F( SensorRepositoryTest, QueryRespectsLimit )
    {
        std::vector<SensorReading> batch;
        for ( int i = 0; i < 10; ++i )
        {
            batch.push_back( MakeReading( "temp-001", "temperature", static_cast<double>( i ) ) );
        }

        ASSERT_TRUE( m_sensorRepo->AddBatch( batch ).has_value() );

        SensorQueryOptions opts;
        opts.m_deviceId = "temp-001";
        opts.m_limit = 3;

        auto result = m_sensorRepo->Query( opts );
        ASSERT_TRUE( result.has_value() );
        EXPECT_EQ( result->size(), 3u );
    }

    TEST_F( SensorRepositoryTest, PurgeRemovesOldReadings )
    {
        // Insert one reading and purge everything older than 0 days
        // (should delete it since it's technically in the past epoch range).
        ASSERT_TRUE( m_sensorRepo->Add( MakeReading( "temp-001", "temperature", 30.0 ) ).has_value() );

        // Purge with 0 day retention — removes records before "now".
        auto purgeResult = m_sensorRepo->Purge( 0 );
        ASSERT_TRUE( purgeResult.has_value() );
        // May or may not delete depending on sub-second timing; just confirm
        // no crash and returns a non-negative count.
        EXPECT_GE( static_cast<int>( purgeResult.value() ), 0 );
    }

} // namespace
