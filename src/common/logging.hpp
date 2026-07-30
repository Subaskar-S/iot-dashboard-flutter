/**
 * @file       logging.hpp
 * @brief      Structured logging interface
 * @standard   C++20
 */

#ifndef IOT_COMMON_LOGGING_HPP
#define IOT_COMMON_LOGGING_HPP

#include <memory>
#include <spdlog/spdlog.h>
#include <string_view>

namespace iot
{
    /// Create a named logger
    inline std::shared_ptr<spdlog::logger> CreateLogger( std::string_view name )
    {
        auto logger = spdlog::get( std::string( name ) );
        if ( !logger )
        {
            logger = spdlog::stdout_color_mt( std::string( name ) );
        }
        return logger;
    }

    /// Initialize logging system
    void InitializeLogging( const std::string& level = "info" );

} // namespace iot

#endif // IOT_COMMON_LOGGING_HPP
