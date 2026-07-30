/**
 * @file       schema.hpp
 * @brief      Embedded SQL migrations for the IoT dashboard schema
 * @standard   C++23
 */

#ifndef IOT_DATABASE_SCHEMA_HPP
#define IOT_DATABASE_SCHEMA_HPP

#include "database/migration_manager.hpp"
#include <vector>

namespace iot::database
{
    /// Returns all migrations in ascending version order.
    inline std::vector<Migration> GetMigrations()
    {
        return {
            {
                .m_version = 1,
                .m_description = "Initial schema",
                .m_sql = R"SQL(
                    CREATE TABLE IF NOT EXISTS devices (
                        id            TEXT    PRIMARY KEY NOT NULL,
                        name          TEXT    NOT NULL,
                        type          TEXT    NOT NULL,
                        protocol      TEXT    NOT NULL DEFAULT '',
                        address       TEXT    NOT NULL DEFAULT '',
                        is_connected  INTEGER NOT NULL DEFAULT 0,
                        last_seen_ms  INTEGER NOT NULL DEFAULT 0
                    );

                    CREATE TABLE IF NOT EXISTS sensor_readings (
                        id            INTEGER PRIMARY KEY AUTOINCREMENT,
                        device_id     TEXT    NOT NULL,
                        sensor_type   TEXT    NOT NULL,
                        value         REAL    NOT NULL,
                        unit          TEXT    NOT NULL DEFAULT '',
                        timestamp_ms  INTEGER NOT NULL,
                        quality       TEXT,
                        FOREIGN KEY (device_id) REFERENCES devices(id) ON DELETE CASCADE
                    );
                    CREATE INDEX IF NOT EXISTS idx_sr_device_ts
                        ON sensor_readings (device_id, timestamp_ms DESC);

                    CREATE TABLE IF NOT EXISTS users (
                        id            TEXT    PRIMARY KEY NOT NULL,
                        username      TEXT    UNIQUE NOT NULL,
                        password_hash TEXT    NOT NULL,
                        role          TEXT    NOT NULL DEFAULT 'Viewer',
                        created_at    INTEGER NOT NULL,
                        updated_at    INTEGER NOT NULL
                    );

                    CREATE TABLE IF NOT EXISTS alerts (
                        id            INTEGER PRIMARY KEY AUTOINCREMENT,
                        device_id     TEXT    NOT NULL,
                        severity      TEXT    NOT NULL,
                        message       TEXT    NOT NULL,
                        acknowledged  INTEGER NOT NULL DEFAULT 0,
                        created_at    INTEGER NOT NULL,
                        FOREIGN KEY (device_id) REFERENCES devices(id) ON DELETE CASCADE
                    );
                    CREATE INDEX IF NOT EXISTS idx_alerts_device
                        ON alerts (device_id, created_at DESC);

                    CREATE TABLE IF NOT EXISTS automation_rules (
                        id            TEXT    PRIMARY KEY NOT NULL,
                        name          TEXT    NOT NULL,
                        enabled       INTEGER NOT NULL DEFAULT 1,
                        condition_json TEXT   NOT NULL,
                        action_json   TEXT    NOT NULL,
                        created_by    TEXT    NOT NULL,
                        created_at    INTEGER NOT NULL,
                        updated_at    INTEGER NOT NULL
                    );
                )SQL",
            },
        };
    }

} // namespace iot::database

#endif // IOT_DATABASE_SCHEMA_HPP
