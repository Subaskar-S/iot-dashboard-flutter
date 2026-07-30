/**
 * @file       sqlite_connection.cpp
 * @brief      RAII SQLite connection wrapper implementation
 * @standard   C++23
 */

#include "database/sqlite_connection.hpp"
#include <stdexcept>

namespace iot::database
{
    SqliteConnection::SqliteConnection( std::string_view path )
        : m_path( path )
    {
        int rc = sqlite3_open_v2( m_path.c_str(), &m_db,
                                  SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr );

        if ( rc != SQLITE_OK )
        {
            const char* msg = m_db ? sqlite3_errmsg( m_db ) : "unknown error";
            sqlite3_close( m_db );
            m_db = nullptr;
            throw std::runtime_error( std::string( "SQLite open failed: " ) + msg );
        }

        // Enable WAL mode for better concurrent read performance.
        sqlite3_exec( m_db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr );

        // Enforce foreign key constraints.
        sqlite3_exec( m_db, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr );
    }

    SqliteConnection::~SqliteConnection()
    {
        if ( m_db )
        {
            sqlite3_close( m_db );
            m_db = nullptr;
        }
    }

    SqliteConnection::SqliteConnection( SqliteConnection&& other ) noexcept
        : m_db( other.m_db )
        , m_path( std::move( other.m_path ) )
    {
        other.m_db = nullptr;
    }

    SqliteConnection& SqliteConnection::operator=( SqliteConnection&& other ) noexcept
    {
        if ( this != &other )
        {
            if ( m_db )
            {
                sqlite3_close( m_db );
            }
            m_db = other.m_db;
            m_path = std::move( other.m_path );
            other.m_db = nullptr;
        }
        return *this;
    }

    bool SqliteConnection::IsOpen() const noexcept
    {
        return m_db != nullptr;
    }

    Result<void> SqliteConnection::Execute( std::string_view sql )
    {
        char* errMsg = nullptr;
        int rc = sqlite3_exec( m_db, sql.data(), nullptr, nullptr, &errMsg );

        if ( rc != SQLITE_OK )
        {
            std::string msg = errMsg ? errMsg : "unknown";
            sqlite3_free( errMsg );
            return std::unexpected( Error::DatabaseError );
        }

        return {};
    }

} // namespace iot::database
