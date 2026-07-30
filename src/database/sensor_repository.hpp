/**
 * @file       sensor_repository.hpp
 * @brief      SQLite persistence for sensor readings (time-series)
 * @standard   C++23
 */

#ifndef IOT_DATABASE_SENSOR_REPOSITORY_HPP
#define IOT_DATABASE_SENSOR_REPOSITORY_HPP

#include "common/error.hpp"
#include "common/types.hpp"
#include "database/sqlite_connection.hpp"
#include "database/sqlite_statement.hpp"
#include <chrono>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <vector>

namespace iot::database
{
    struct SensorQueryOptions
    {
        std::string m_deviceId;
        std::string m_sensorType; // empty = all types
        Timestamp m_from{};
        Timestamp m_to{};
        size_t m_limit = 1000;
    };

    class SqliteSensorRepository
    {
        public:
        explicit SqliteSensorRepository( SqliteConnection& conn );

        [[nodiscard]] Result<void> Add( const SensorReading& reading );

        /// Bulk insert — significantly faster for high-frequency sensor data.
        [[nodiscard]] Result<void> AddBatch( const std::vector<SensorReading>& readings );

        [[nodiscard]] Result<std::vector<SensorReading>> Query( const SensorQueryOptions& opts );

        /// Delete readings older than `retentionDays`.
        [[nodiscard]] Result<uint32_t> Purge( uint32_t retentionDays );

        private:
        SqliteConnection& m_conn;
        std::shared_ptr<spdlog::logger> m_logger;
        mutable std::mutex m_mutex;

        [[nodiscard]] static SensorReading RowToReading( SqliteStatement& stmt );
    };

} // namespace iot::database

#endif // IOT_DATABASE_SENSOR_REPOSITORY_HPP
