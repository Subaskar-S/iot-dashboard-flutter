/**
 * @file       sqlite_connection.hpp
 * @brief      RAII SQLite connection wrapper
 * @standard   C++23
 */

#ifndef IOT_DATABASE_SQLITE_CONNECTION_HPP
#define IOT_DATABASE_SQLITE_CONNECTION_HPP

#include "common/error.hpp"
#include "common/logging.hpp"
#include <memory>
#include <mutex>
#include <sqlite3.h>
#include <string>
#include <string_view>

namespace iot::database
{
    /**
     * Thin RAII wrapper around a single sqlite3* handle.
     * Intentionally non-copyable; move-only.
     */
    class SqliteConnection
    {
        public:
        explicit SqliteConnection( std::string_view path );
        ~SqliteConnection();

        SqliteConnection( const SqliteConnection& ) = delete;
        SqliteConnection& operator=( const SqliteConnection& ) = delete;
        SqliteConnection( SqliteConnection&& ) noexcept;
        SqliteConnection& operator=( SqliteConnection&& ) noexcept;

        [[nodiscard]] bool IsOpen() const noexcept;

        /// Execute a single SQL statement with no result rows.
        Result<void> Execute( std::string_view sql );

        /// Raw handle — use only in SqliteStatement.
        [[nodiscard]] sqlite3* Handle() noexcept
        {
            return m_db;
        }

        private:
        sqlite3* m_db = nullptr;
        std::string m_path;
    };

} // namespace iot::database

#endif // IOT_DATABASE_SQLITE_CONNECTION_HPP
