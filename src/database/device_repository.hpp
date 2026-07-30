/**
 * @file       device_repository.hpp
 * @brief      SQLite implementation of IDeviceRepository
 * @standard   C++23
 */

#ifndef IOT_DATABASE_DEVICE_REPOSITORY_HPP
#define IOT_DATABASE_DEVICE_REPOSITORY_HPP

#include "core/interfaces/i_device_repository.hpp"
#include "database/sqlite_connection.hpp"
#include "database/sqlite_statement.hpp"
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>

namespace iot::database
{
    /**
     * Persists DeviceInfo entities in the `devices` SQLite table.
     *
     * Thread-safety: each method acquires m_mutex so the same repository
     * instance can be shared across the HTTP and MQTT thread pools.
     */
    class SqliteDeviceRepository final : public core::IDeviceRepository
    {
        public:
        explicit SqliteDeviceRepository( SqliteConnection& conn );

        [[nodiscard]] Result<DeviceInfo> GetById( const std::string& deviceId ) override;
        [[nodiscard]] Result<std::vector<DeviceInfo>> GetAll() override;
        [[nodiscard]] Result<void> Add( const DeviceInfo& device ) override;
        [[nodiscard]] Result<void> Update( const DeviceInfo& device ) override;
        [[nodiscard]] Result<void> Remove( const std::string& deviceId ) override;

        private:
        SqliteConnection& m_conn;
        std::shared_ptr<spdlog::logger> m_logger;
        mutable std::mutex m_mutex;

        [[nodiscard]] static DeviceInfo RowToDevice( SqliteStatement& stmt );
    };

} // namespace iot::database

#endif // IOT_DATABASE_DEVICE_REPOSITORY_HPP
