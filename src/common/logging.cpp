/**
 * @file       logging.cpp
 * @brief      Logging system initialization
 * @standard   C++20
 */

#include "common/logging.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace iot
{
    void InitializeLogging( const std::string& level )
    {
        spdlog::level::level_enum logLevel = spdlog::level::info;

        if ( level == "trace" )      logLevel = spdlog::level::trace;
        else if ( level == "debug" ) logLevel = spdlog::level::debug;
        else if ( level == "info" )  logLevel = spdlog::level::info;
        else if ( level == "warn" )  logLevel = spdlog::level::warn;
        else if ( level == "error" ) logLevel = spdlog::level::err;

        spdlog::set_level( logLevel );
        spdlog::set_pattern( "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v" );
    }

} // namespace iot
