/**
 * @file       migration_manager.cpp
 * @brief      Schema migration runner
 * @standard   C++23
 */

#include "database/migration_manager.hpp"
#include "database/sqlite_statement.hpp"

namespace iot::database
{
    MigrationManager::MigrationManager( SqliteConnection& conn )
        : m_conn( conn )
    {
    }

    void MigrationManager::Register( Migration migration )
    {
        m_migrations.push_back( std::move( migration ) );
    }

    void MigrationManager::EnsureVersionTable()
    {
        std::ignore = m_conn.Execute( R"(
            CREATE TABLE IF NOT EXISTS schema_version (
                version     INTEGER NOT NULL,
                applied_at  INTEGER NOT NULL
            );
        )" );
    }

    int MigrationManager::CurrentVersion()
    {
        EnsureVersionTable();
        SqliteStatement stmt( m_conn, "SELECT MAX(version) FROM schema_version;" );
        if ( stmt.Step() && !stmt.IsNull( 0 ) )
        {
            return static_cast<int>( stmt.GetInt( 0 ) );
        }
        return 0;
    }

    Result<void> MigrationManager::Migrate()
    {
        EnsureVersionTable();
        int current = CurrentVersion();

        for ( const auto& migration : m_migrations )
        {
            if ( migration.m_version <= current )
            {
                continue;
            }

            // Run the migration SQL inside a transaction.
            auto beginResult = m_conn.Execute( "BEGIN TRANSACTION;" );
            if ( !beginResult )
            {
                return beginResult;
            }

            auto sqlResult = m_conn.Execute( migration.m_sql );
            if ( !sqlResult )
            {
                m_conn.Execute( "ROLLBACK;" );
                return sqlResult;
            }

            // Record the version inside the same transaction.
            SqliteStatement insert( m_conn, "INSERT INTO schema_version (version, applied_at) VALUES (?, ?);" );
            insert.BindInt( 1, migration.m_version );
            insert.BindInt( 2, std::chrono::duration_cast<std::chrono::seconds>(
                                   std::chrono::system_clock::now().time_since_epoch() )
                                   .count() );
            insert.Step();

            auto commitResult = m_conn.Execute( "COMMIT;" );
            if ( !commitResult )
            {
                m_conn.Execute( "ROLLBACK;" );
                return commitResult;
            }
        }

        return {};
    }

} // namespace iot::database
