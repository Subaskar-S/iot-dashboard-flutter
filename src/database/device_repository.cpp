/**
 * @file       device_repository.cpp
 * @brief      SQLite DeviceInfo persistence
 * @standard   C++23
 */

#include "database/device_repository.hpp"
#include "database/sqlite_statement.hpp"
#include <chrono>
#include <mutex>

namespace iot::database
{
    namespace
    {
        int64_t ToUnixMs( const Timestamp& ts )
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>( ts.time_since_epoch() ).count();
        }

        Timestamp FromUnixMs( int64_t ms )
        {
            return Timestamp( std::chrono::milliseconds( ms ) );
        }
    } // namespace

    SqliteDeviceRepository::SqliteDeviceRepository( SqliteConnection& conn )
        : m_conn( conn )
        , m_logger( CreateLogger( "SqliteDeviceRepository" ) )
    {
    }

    DeviceInfo SqliteDeviceRepository::RowToDevice( SqliteStatement& stmt )
    {
        DeviceInfo d;
        d.m_id = stmt.GetText( 0 );
        d.m_name = stmt.GetText( 1 );
        d.m_type = stmt.GetText( 2 );
        d.m_protocol = stmt.GetText( 3 );
        d.m_address = stmt.GetText( 4 );
        d.m_isConnected = stmt.GetInt( 5 ) != 0;
        d.m_lastSeen = FromUnixMs( stmt.GetInt( 6 ) );
        return d;
    }

    Result<DeviceInfo> SqliteDeviceRepository::GetById( const std::string& deviceId )
    {
        std::lock_guard lock( m_mutex );

        static constexpr std::string_view kSql = "SELECT id, name, type, protocol, address, is_connected, last_seen_ms "
                                                 "FROM devices WHERE id = ? LIMIT 1;";

        SqliteStatement stmt( m_conn, kSql );
        stmt.BindText( 1, deviceId );

        if ( !stmt.Step() )
        {
            return std::unexpected( Error::DeviceNotFound );
        }

        return RowToDevice( stmt );
    }

    Result<std::vector<DeviceInfo>> SqliteDeviceRepository::GetAll()
    {
        std::lock_guard lock( m_mutex );

        static constexpr std::string_view kSql = "SELECT id, name, type, protocol, address, is_connected, last_seen_ms "
                                                 "FROM devices ORDER BY name;";

        SqliteStatement stmt( m_conn, kSql );
        std::vector<DeviceInfo> result;

        while ( stmt.Step() )
        {
            result.push_back( RowToDevice( stmt ) );
        }

        return result;
    }

    Result<void> SqliteDeviceRepository::Add( const DeviceInfo& device )
    {
        std::lock_guard lock( m_mutex );

        static constexpr std::string_view kSql =
            "INSERT INTO devices (id, name, type, protocol, address, is_connected, last_seen_ms) "
            "VALUES (?, ?, ?, ?, ?, ?, ?);";

        SqliteStatement stmt( m_conn, kSql );
        stmt.BindText( 1, device.m_id );
        stmt.BindText( 2, device.m_name );
        stmt.BindText( 3, device.m_type );
        stmt.BindText( 4, device.m_protocol );
        stmt.BindText( 5, device.m_address );
        stmt.BindInt( 6, device.m_isConnected ? 1 : 0 );
        stmt.BindInt( 7, ToUnixMs( device.m_lastSeen ) );

        if ( stmt.Step() )
        {
            return {};
        }

        // Step() returns false on DONE (not an error for INSERT).
        return {};
    }

    Result<void> SqliteDeviceRepository::Update( const DeviceInfo& device )
    {
        std::lock_guard lock( m_mutex );

        static constexpr std::string_view kSql = "UPDATE devices SET name = ?, type = ?, protocol = ?, address = ?, "
                                                 "is_connected = ?, last_seen_ms = ? WHERE id = ?;";

        SqliteStatement stmt( m_conn, kSql );
        stmt.BindText( 1, device.m_name );
        stmt.BindText( 2, device.m_type );
        stmt.BindText( 3, device.m_protocol );
        stmt.BindText( 4, device.m_address );
        stmt.BindInt( 5, device.m_isConnected ? 1 : 0 );
        stmt.BindInt( 6, ToUnixMs( device.m_lastSeen ) );
        stmt.BindText( 7, device.m_id );

        stmt.Step();

        if ( sqlite3_changes( m_conn.Handle() ) == 0 )
        {
            return std::unexpected( Error::DeviceNotFound );
        }

        return {};
    }

    Result<void> SqliteDeviceRepository::Remove( const std::string& deviceId )
    {
        std::lock_guard lock( m_mutex );

        static constexpr std::string_view kSql = "DELETE FROM devices WHERE id = ?;";

        SqliteStatement stmt( m_conn, kSql );
        stmt.BindText( 1, deviceId );
        stmt.Step();

        if ( sqlite3_changes( m_conn.Handle() ) == 0 )
        {
            return std::unexpected( Error::DeviceNotFound );
        }

        return {};
    }

} // namespace iot::database
