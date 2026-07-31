/**
 * @file       heartbeat_monitor.hpp
 * @brief      Detects offline devices when heartbeat stops arriving
 * @standard   C++23
 */

#ifndef IOT_DEVICES_HEARTBEAT_MONITOR_HPP
#define IOT_DEVICES_HEARTBEAT_MONITOR_HPP

#include "common/types.hpp"
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <unordered_map>

namespace iot::devices
{
    using TimeoutCallback = std::function<void( const std::string& deviceId )>;

    /**
     * Polls tracked devices at a configurable interval.  When a device
     * has not sent a heartbeat within its timeout window, the registered
     * callback is fired exactly once (until the device comes back).
     *
     * Thread-safe: RecordHeartbeat() and Register() may be called from
     * any thread.
     */
    class HeartbeatMonitor
    {
        public:
        explicit HeartbeatMonitor( std::chrono::seconds pollInterval = std::chrono::seconds( 30 ) );
        ~HeartbeatMonitor();

        HeartbeatMonitor( const HeartbeatMonitor& ) = delete;
        HeartbeatMonitor& operator=( const HeartbeatMonitor& ) = delete;

        /// Register a device with its inactivity timeout.
        void Register( const std::string& deviceId, std::chrono::seconds timeout );

        /// Unregister a device (stop monitoring it).
        void Unregister( const std::string& deviceId );

        /// Update last-seen timestamp for a device.
        void RecordHeartbeat( const std::string& deviceId );

        /// Set callback invoked when a device exceeds its timeout.
        void SetTimeoutCallback( TimeoutCallback cb );

        void Start();
        void Stop();

        [[nodiscard]] bool IsRunning() const noexcept;

        /// Returns time since last heartbeat, or nullopt if not registered.
        [[nodiscard]] std::optional<std::chrono::seconds> TimeSinceLastSeen( const std::string& deviceId ) const;

        private:
        struct DeviceRecord
        {
            std::chrono::seconds m_timeout;
            Timestamp m_lastSeen;
            bool m_timedOut = false; // avoid duplicate callbacks
        };

        std::chrono::seconds m_pollInterval;
        mutable std::mutex m_mutex;
        std::unordered_map<std::string, DeviceRecord> m_devices;
        TimeoutCallback m_timeoutCallback;
        std::thread m_pollThread;
        std::atomic<bool> m_running{ false };

        void PollLoop();
    };

} // namespace iot::devices

#endif // IOT_DEVICES_HEARTBEAT_MONITOR_HPP
