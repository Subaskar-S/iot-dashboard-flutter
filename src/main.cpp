/**
 * @file       main.cpp
 * @brief      CLI entry point for IoT Dashboard
 * @standard   C++20
 *
 * Usage:
 *   iot-dashboard --config <path> [options]
 */

#include "common/logging.hpp"
#include <iostream>
#include <string>

// NOTE: api::Server wiring will be re-introduced once the api/ module
// (composition root) is implemented. Until then, main() only parses
// arguments and initializes logging so the build stays green while
// modules are added incrementally (see CLAUDE.md).

using namespace iot;

// ---------------------------------------------------------------------------
// Argument parser
// ---------------------------------------------------------------------------
struct Args
{
    std::string m_configPath;
    uint16_t m_httpPort = 8080;
    uint16_t m_wsPort = 8081;
    std::string m_dbPath = "./iot.db";
    std::string m_mqttBroker;
    std::string m_logLevel = "info";
    bool m_serve = false;
    bool m_verbose = false;
    bool m_help = false;
};

static void PrintHelp( const char* prog )
{
    std::cout << "Usage: " << prog << " [options]\n\n"
              << "Options:\n"
              << "  --config <path>          Configuration file (JSON)\n"
              << "  --port <n>               HTTP server port (default: 8080)\n"
              << "  --ws-port <n>            WebSocket port (default: 8081)\n"
              << "  --db <path>              Database path (default: ./iot.db)\n"
              << "  --mqtt-broker <url>      MQTT broker address\n"
              << "  --log-level <level>      Log level: trace|debug|info|warn|error (default: info)\n"
              << "  --serve                  Start server (blocking)\n"
              << "  --verbose                Enable debug logging\n"
              << "  --help                   Show this help\n";
}

static Args ParseArgs( int argc, char** argv )
{
    Args args;
    
    for ( int i = 1; i < argc; ++i )
    {
        std::string arg = argv[i];
        
        if ( arg == "--help" || arg == "-h" )
        {
            args.m_help = true;
        }
        else if ( arg == "--config" && i + 1 < argc )
        {
            args.m_configPath = argv[++i];
        }
        else if ( arg == "--port" && i + 1 < argc )
        {
            args.m_httpPort = static_cast<uint16_t>( std::stoi( argv[++i] ) );
        }
        else if ( arg == "--ws-port" && i + 1 < argc )
        {
            args.m_wsPort = static_cast<uint16_t>( std::stoi( argv[++i] ) );
        }
        else if ( arg == "--db" && i + 1 < argc )
        {
            args.m_dbPath = argv[++i];
        }
        else if ( arg == "--mqtt-broker" && i + 1 < argc )
        {
            args.m_mqttBroker = argv[++i];
        }
        else if ( arg == "--log-level" && i + 1 < argc )
        {
            args.m_logLevel = argv[++i];
        }
        else if ( arg == "--serve" )
        {
            args.m_serve = true;
        }
        else if ( arg == "--verbose" || arg == "-v" )
        {
            args.m_verbose = true;
            args.m_logLevel = "debug";
        }
    }
    
    return args;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main( int argc, char** argv )
{
    Args args = ParseArgs( argc, argv );
    
    if ( args.m_help )
    {
        PrintHelp( argv[0] );
        return 0;
    }
    
    // Initialize logging
    InitializeLogging( args.m_logLevel );
    auto logger = CreateLogger( "Main" );
    
    logger->info( "IoT Dashboard starting..." );
    logger->info( "HTTP port: {}, WebSocket port: {}", args.m_httpPort, args.m_wsPort );

    if ( args.m_serve )
    {
        logger->warn( "Server composition root (api module) is not implemented yet" );
    }

    logger->info( "IoT Dashboard exiting" );
    return 0;
}
