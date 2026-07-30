/**
 * @file       sqlite_statement.cpp
 * @brief      RAII prepared statement implementation
 * @standard   C++23
 */

#include "database/sqlite_statement.hpp"
#include <stdexcept>

namespace iot::database
{
    SqliteStatement::SqliteStatement( SqliteConnection& conn, std::string_view sql )
    {
        int rc = sqlite3_prepare_v2( conn.Handle(), sql.data(), static_cast<int>( sql.size() ), &m_stmt, nullptr );

        if ( rc != SQLITE_OK )
        {
            throw std::runtime_error( std::string( "SQLite prepare failed: " ) + sqlite3_errmsg( conn.Handle() ) );
        }
    }

    SqliteStatement::~SqliteStatement()
    {
        if ( m_stmt )
        {
            sqlite3_finalize( m_stmt );
            m_stmt = nullptr;
        }
    }

    void SqliteStatement::BindNull( int col )
    {
        sqlite3_bind_null( m_stmt, col );
    }

    void SqliteStatement::BindInt( int col, int64_t value )
    {
        sqlite3_bind_int64( m_stmt, col, value );
    }

    void SqliteStatement::BindReal( int col, double value )
    {
        sqlite3_bind_double( m_stmt, col, value );
    }

    void SqliteStatement::BindText( int col, std::string_view value )
    {
        sqlite3_bind_text( m_stmt, col, value.data(), static_cast<int>( value.size() ), SQLITE_TRANSIENT );
    }

    bool SqliteStatement::Step()
    {
        int rc = sqlite3_step( m_stmt );
        return rc == SQLITE_ROW;
    }

    void SqliteStatement::Reset()
    {
        sqlite3_reset( m_stmt );
        sqlite3_clear_bindings( m_stmt );
    }

    int64_t SqliteStatement::GetInt( int col ) const
    {
        return sqlite3_column_int64( m_stmt, col );
    }

    double SqliteStatement::GetReal( int col ) const
    {
        return sqlite3_column_double( m_stmt, col );
    }

    std::string SqliteStatement::GetText( int col ) const
    {
        const auto* text = reinterpret_cast<const char*>( sqlite3_column_text( m_stmt, col ) );
        return text ? text : "";
    }

    bool SqliteStatement::IsNull( int col ) const
    {
        return sqlite3_column_type( m_stmt, col ) == SQLITE_NULL;
    }

} // namespace iot::database
