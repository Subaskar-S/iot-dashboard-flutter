/**
 * @file       sensor_repository.cpp
 * @brief      SQLite time-series sensor reading persistence
 * @standard   C++23
 */

#include "database/sensor_repository.hpp"
#include "database/sqlite_statement.hpp"
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

    SqliteSensorRepository::SqliteSensorRepository( SqliteConnection& conn )
        : m_conn( conn )
        , m_logger( CreateLogger( "SqliteSensorRepository" ) )
    {
    }

    SensorReading SqliteSensorRepository::RowToReading( SqliteStatement& stmt )
    {
        SensorReading r;
        r.m_deviceId = stmt.GetText( 0 );
        r.m_sensor = stmt.GetText( 1 );
        r.m_value = stmt.GetReal( 2 );
        r.m_unit = stmt.GetText( 3 );
        r.m_timestamp = FromUnixMs( stmt.GetInt( 4 ) );
        if ( !stmt.IsNull( 5 ) )
        {
            r.m_quality = stmt.GetText( 5 );
        }
        return r;
    }

    Result<void> SqliteSensorRepository::Add( const SensorReading& reading )
    {
        std::lock_guard lock( m_mutex );

        static constexpr std::string_view kSql =
            "INSERT INTO sensor_readings (device_id, sensor_type, value, unit, timestamp_ms, quality) "
            "VALUES (?, ?, ?, ?, ?, ?);";

        SqliteStatement stmt( m_conn, kSql );
        stmt.BindText( 1, reading.m_deviceId );
        stmt.BindText( 2, reading.m_sensor );
        stmt.BindReal( 3, reading.m_value );
        stmt.BindText( 4, reading.m_unit );
        stmt.BindInt( 5, ToUnixMs( reading.m_timestamp ) );

        if ( reading.m_quality )
        {
            stmt.BindText( 6, *reading.m_quality );
        }
        else
        {
            stmt.BindNull( 6 );
        }

        stmt.Step();
        return {};
    }

    Result<void> SqliteSensorRepository::AddBatch( const std::vector<SensorReading>& readings )
    {
        if ( readings.empty() )
        {
            return {};
        }

        std::lock_guard lock( m_mutex );

        auto beginResult = m_conn.Execute( "BEGIN TRANSACTION;" );
        if ( !beginResult )
        {
            return beginResult;
        }

        static constexpr std::string_view kSql =
            "INSERT INTO sensor_readings (device_id, sensor_type, value, unit, timestamp_ms, quality) "
            "VALUES (?, ?, ?, ?, ?, ?);";

        SqliteStatement stmt( m_conn, kSql );

        for ( const auto& reading : readings )
        {
            stmt.BindText( 1, reading.m_deviceId );
            stmt.BindText( 2, reading.m_sensor );
            stmt.BindReal( 3, reading.m_value );
            stmt.BindText( 4, reading.m_unit );
            stmt.BindInt( 5, ToUnixMs( reading.m_timestamp ) );

            if ( reading.m_quality )
            {
                stmt.BindText( 6, *reading.m_quality );
            }
            else
            {
                stmt.BindNull( 6 );
            }

            stmt.Step();
            stmt.Reset();
        }

        auto commitResult = m_conn.Execute( "COMMIT;" );
        if ( !commitResult )
        {
            m_conn.Execute( "ROLLBACK;" );
            return commitResult;
        }

        return {};
    }

    Result<std::vector<SensorReading>> SqliteSensorRepository::Query( const SensorQueryOptions& opts )
    {
        std::lock_guard lock( m_mutex );

        std::string sql = "SELECT device_id, sensor_type, value, unit, timestamp_ms, quality "
                          "FROM sensor_readings WHERE device_id = ?";

        if ( !opts.m_sensorType.empty() )
        {
            sql += " AND sensor_type = ?";
        }

        auto fromMs = ToUnixMs( opts.m_from );
        auto toMs = ToUnixMs( opts.m_to );

        if ( fromMs > 0 )
        {
            sql += " AND timestamp_ms >= ?";
        }
        if ( toMs > 0 )
        {
            sql += " AND timestamp_ms <= ?";
        }

        sql += " ORDER BY timestamp_ms DESC LIMIT ?;";

        SqliteStatement stmt( m_conn, sql );

        int paramIdx = 1;
        stmt.BindText( paramIdx++, opts.m_deviceId );

        if ( !opts.m_sensorType.empty() )
        {
            stmt.BindText( paramIdx++, opts.m_sensorType );
        }
        if ( fromMs > 0 )
        {
            stmt.BindInt( paramIdx++, fromMs );
        }
        if ( toMs > 0 )
        {
            stmt.BindInt( paramIdx++, toMs );
        }

        stmt.BindInt( paramIdx, static_cast<int64_t>( opts.m_limit ) );

        std::vector<SensorReading> result;
        while ( stmt.Step() )
        {
            result.push_back( RowToReading( stmt ) );
        }

        return result;
    }

    Result<uint32_t> SqliteSensorRepository::Purge( uint32_t retentionDays )
    {
        std::lock_guard lock( m_mutex );

        auto cutoffMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                ( std::chrono::system_clock::now() - std::chrono::hours( 24 * retentionDays ) ).time_since_epoch() )
                .count();

        static constexpr std::string_view kSql = "DELETE FROM sensor_readings WHERE timestamp_ms < ?;";

        SqliteStatement stmt( m_conn, kSql );
        stmt.BindInt( 1, cutoffMs );
        stmt.Step();

        return static_cast<uint32_t>( sqlite3_changes( m_conn.Handle() ) );
    }

} // namespace iot::database
