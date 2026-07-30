/**
 * @file       heartbeat_monitor.cpp
 * @brief      Heartbeat monitor poll loop
 * @standard   C++23
 */

#include "devices/heartbeat_monitor.hpp"
#include "common/logging.hpp"

namespace iot::devices
{
    HeartbeatMonitor::HeartbeatMonitor( std::chrono::seconds pollInterval )
        : m_pollInterval( pollInterval )
    {
    }

    HeartbeatMonitor::~HeartbeatMonitor()
    {
        Stop();
    }

    void HeartbeatMonitor::Register( const std::string& deviceId, std::chrono::seconds timeout )
    {
        std::lock_guard lock( m_mutex );
        m_devices[deviceId] =
            DeviceRecord{ .m_timeout = timeout, .m_lastSeen = std::chrono::system_clock::now(), .m_timedOut = false };
    }

    void HeartbeatMonitor::Unregister( const std::string& deviceId )
    {
        std::lock_guard lock( m_mutex );
        m_devices.erase( deviceId );
    }

    void HeartbeatMonitor::RecordHeartbeat( const std::string& deviceId )
    {
        std::lock_guard lock( m_mutex );
        auto it = m_devices.find( deviceId );
        if ( it != m_devices.end() )
        {
            it->second.m_lastSeen = std::chrono::system_clock::now();
            it->second.m_timedOut = false; // back online
        }
    }

    void HeartbeatMonitor::SetTimeoutCallback( TimeoutCallback cb )
    {
        std::lock_guard lock( m_mutex );
        m_timeoutCallback = std::move( cb );
    }

    void HeartbeatMonitor::Start()
    {
        if ( m_running.exchange( true ) )
        {
            return;
        }
        m_pollThread = std::thread( &HeartbeatMonitor::PollLoop, this );
    }

    void HeartbeatMonitor::Stop()
    {
        if ( !m_running.exchange( false ) )
        {
            return;
        }
        if ( m_pollThread.joinable() )
        {
            m_pollThread.join();
        }
    }

    bool HeartbeatMonitor::IsRunning() const noexcept
    {
        return m_running.load();
    }

    std::optional<std::chrono::seconds> HeartbeatMonitor::TimeSinceLastSeen( const std::string& deviceId ) const
    {
        std::lock_guard lock( m_mutex );
        auto it = m_devices.find( deviceId );
        if ( it == m_devices.end() )
        {
            return std::nullopt;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>( std::chrono::system_clock::now() -
                                                                         it->second.m_lastSeen );
        return elapsed;
    }

    void HeartbeatMonitor::PollLoop()
    {
        while ( m_running )
        {
            // Sleep in short increments so Stop() is responsive.
            auto remaining = m_pollInterval;
            while ( remaining > std::chrono::seconds( 0 ) && m_running )
            {
                auto slice = std::min( remaining, std::chrono::seconds( 1 ) );
                std::this_thread::sleep_for( slice );
                remaining -= slice;
            }

            if ( !m_running )
            {
                break;
            }

            auto now = std::chrono::system_clock::now();
            TimeoutCallback cb;

            std::vector<std::string> timedOut;
            {
                std::lock_guard lock( m_mutex );
                cb = m_timeoutCallback;
                for ( auto& [id, record] : m_devices )
                {
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>( now - record.m_lastSeen );

                    if ( elapsed >= record.m_timeout && !record.m_timedOut )
                    {
                        record.m_timedOut = true;
                        timedOut.push_back( id );
                    }
                }
            }

            if ( cb )
            {
                for ( const auto& id : timedOut )
                {
                    cb( id );
                }
            }
        }
    }

} // namespace iot::devices
