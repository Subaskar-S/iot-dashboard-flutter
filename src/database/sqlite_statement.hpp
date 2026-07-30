/**
 * @file       sqlite_statement.hpp
 * @brief      RAII prepared statement + parameter binding helpers
 * @standard   C++23
 */

#ifndef IOT_DATABASE_SQLITE_STATEMENT_HPP
#define IOT_DATABASE_SQLITE_STATEMENT_HPP

#include "common/error.hpp"
#include "database/sqlite_connection.hpp"
#include <sqlite3.h>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace iot::database
{
    /// A bound parameter value (NULL, integer, real, or text).
    using SqlParam = std::variant<std::monostate, int64_t, double, std::string>;

    /**
     * RAII wrapper around sqlite3_stmt*.
     *
     * Usage:
     *   SqliteStatement stmt( conn, "SELECT * FROM devices WHERE id = ?" );
     *   stmt.Bind( 1, "my-device" );
     *   while ( stmt.Step() ) { ... stmt.GetText(0) ... }
     */
    class SqliteStatement
    {
        public:
        SqliteStatement( SqliteConnection& conn, std::string_view sql );
        ~SqliteStatement();

        SqliteStatement( const SqliteStatement& ) = delete;
        SqliteStatement& operator=( const SqliteStatement& ) = delete;

        /// Bind helpers (1-based index).
        void BindNull( int col );
        void BindInt( int col, int64_t value );
        void BindReal( int col, double value );
        void BindText( int col, std::string_view value );

        /// Advance one row. Returns true if a data row is available.
        /// For INSERT/UPDATE/DELETE, ignore the return value.
        bool Step();

        /// Reset statement so it can be re-executed.
        void Reset();

        /// Column accessors (0-based index).
        [[nodiscard]] int64_t GetInt( int col ) const;
        [[nodiscard]] double GetReal( int col ) const;
        [[nodiscard]] std::string GetText( int col ) const;
        [[nodiscard]] bool IsNull( int col ) const;

        private:
        sqlite3_stmt* m_stmt = nullptr;
    };

} // namespace iot::database

#endif // IOT_DATABASE_SQLITE_STATEMENT_HPP
