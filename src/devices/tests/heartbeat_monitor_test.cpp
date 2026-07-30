/**
 * @file       heartbeat_monitor_test.cpp
 * @brief      Unit tests for HeartbeatMonitor
 * @standard   C++23
 */

#include "devices/heartbeat_monitor.hpp"
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

namespace
{
    using namespace iot::devices;
    using namespace std::chrono_literals;

    TEST( HeartbeatMonitorTest, NotRunningBeforeStart )
    {
        HeartbeatMonitor monitor( 1s );
        EXPECT_FALSE( monitor.IsRunning() );
    }

    TEST( HeartbeatMonitorTest, StartAndStopCycle )
    {
        HeartbeatMonitor monitor( 1s );
        monitor.Start();
        EXPECT_TRUE( monitor.IsRunning() );
        monitor.Stop();
        EXPECT_FALSE( monitor.IsRunning() );
    }

    TEST( HeartbeatMonitorTest, RegisteredDeviceHasKnownLastSeen )
    {
        HeartbeatMonitor monitor( 60s );
        monitor.Register( "dev-001", 10s );

        auto elapsed = monitor.TimeSinceLastSeen( "dev-001" );
        ASSERT_TRUE( elapsed.has_value() );
        EXPECT_LT( elapsed->count(), 2 ); // registered just now
    }

    TEST( HeartbeatMonitorTest, UnregisteredDeviceReturnsNullopt )
    {
        HeartbeatMonitor monitor( 60s );
        EXPECT_FALSE( monitor.TimeSinceLastSeen( "ghost" ).has_value() );
    }

    TEST( HeartbeatMonitorTest, RecordHeartbeatResetsLastSeen )
    {
        HeartbeatMonitor monitor( 60s );
        monitor.Register( "dev-001", 10s );

        std::this_thread::sleep_for( 20ms );
        monitor.RecordHeartbeat( "dev-001" );

        auto elapsed = monitor.TimeSinceLastSeen( "dev-001" );
        ASSERT_TRUE( elapsed.has_value() );
        EXPECT_LT( elapsed->count(), 1 );
    }

    TEST( HeartbeatMonitorTest, TimeoutCallbackFiredForLateDevice )
    {
        // Use a very short timeout and poll interval for fast test.
        HeartbeatMonitor monitor( 1s ); // poll every 1s

        std::atomic<int> callbackCount{ 0 };
        std::string timedOutId;

        monitor.SetTimeoutCallback(
            [&]( const std::string& id )
            {
                timedOutId = id;
                callbackCount++;
            } );

        // Register device with 1s timeout.
        monitor.Register( "dev-late", 1s );
        monitor.Start();

        // Wait long enough for poll + timeout to fire.
        std::this_thread::sleep_for( 2500ms );

        monitor.Stop();

        EXPECT_EQ( callbackCount.load(), 1 );
        EXPECT_EQ( timedOutId, "dev-late" );
    }

    TEST( HeartbeatMonitorTest, CallbackNotFiredAgainAfterFirstTimeout )
    {
        HeartbeatMonitor monitor( 1s );
        std::atomic<int> callbackCount{ 0 };

        monitor.SetTimeoutCallback(
            [&]( const std::string& )
            {
                callbackCount++;
            } );
        monitor.Register( "dev-late", 1s );
        monitor.Start();

        // Wait for 2 poll cycles — callback should still fire only once.
        std::this_thread::sleep_for( 3500ms );

        monitor.Stop();
        EXPECT_EQ( callbackCount.load(), 1 );
    }

    TEST( HeartbeatMonitorTest, HeartbeatResetsTimeoutFlag )
    {
        HeartbeatMonitor monitor( 1s );
        std::atomic<int> callbackCount{ 0 };

        monitor.SetTimeoutCallback(
            [&]( const std::string& )
            {
                callbackCount++;
            } );
        monitor.Register( "dev-recover", 1s );
        monitor.Start();

        // First timeout fires.
        std::this_thread::sleep_for( 2500ms );
        EXPECT_EQ( callbackCount.load(), 1 );

        // Send heartbeat — device comes back.
        monitor.RecordHeartbeat( "dev-recover" );

        // Wait another poll cycle — should timeout again (new cycle).
        std::this_thread::sleep_for( 2500ms );

        monitor.Stop();
        EXPECT_EQ( callbackCount.load(), 2 );
    }

    TEST( HeartbeatMonitorTest, UnregisterStopsMonitoring )
    {
        HeartbeatMonitor monitor( 1s );
        std::atomic<int> callbackCount{ 0 };

        monitor.SetTimeoutCallback(
            [&]( const std::string& )
            {
                callbackCount++;
            } );
        monitor.Register( "dev-removed", 1s );
        monitor.Unregister( "dev-removed" );
        monitor.Start();

        std::this_thread::sleep_for( 2500ms );
        monitor.Stop();

        EXPECT_EQ( callbackCount.load(), 0 );
    }

} // namespace
