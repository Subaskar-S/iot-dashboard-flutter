/**
 * @file       migration_manager.hpp
 * @brief      Schema versioning and incremental migration runner
 * @standard   C++23
 */

#ifndef IOT_DATABASE_MIGRATION_MANAGER_HPP
#define IOT_DATABASE_MIGRATION_MANAGER_HPP

#include "common/error.hpp"
#include "database/sqlite_connection.hpp"
#include <string>
#include <vector>

namespace iot::database
{
    struct Migration
    {
        int m_version;
        std::string m_description;
        std::string m_sql; // Full DDL (may contain multiple statements)
    };

    /**
     * Runs schema migrations in version order.
     *
     * Tracks the current schema version in a `schema_version` table and
     * applies only migrations whose version exceeds the stored value.
     * All migrations run inside a transaction — any failure rolls back and
     * returns an error.
     */
    class MigrationManager
    {
        public:
        explicit MigrationManager( SqliteConnection& conn );

        /// Register a migration.  Must be called in ascending version order.
        void Register( Migration migration );

        /// Apply all pending migrations. Safe to call on every startup.
        [[nodiscard]] Result<void> Migrate();

        [[nodiscard]] int CurrentVersion();

        private:
        SqliteConnection& m_conn;
        std::vector<Migration> m_migrations;

        void EnsureVersionTable();
    };

} // namespace iot::database

#endif // IOT_DATABASE_MIGRATION_MANAGER_HPP
