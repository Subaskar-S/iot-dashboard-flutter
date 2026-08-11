/**
 * @file       main.cpp
 * @brief      CLI entry point for IoT Dashboard
 * @standard   C++23
 */

#include "api/application.hpp"
#include "common/logging.hpp"
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

using namespace iot;
using namespace iot::api;

static std::function<void()> g_stopHandler;

/// Returns environment variable @p name, or @p fallback when it is unset or empty.
/// Secrets are read from the environment rather than argv, which is world-readable via ps.
static std::string EnvOr( const char* name, const std::string& fallback )
{
    const char* value = std::getenv( name );
    if ( value == nullptr || *value == '\0' )
    {
        return fallback;
    }
    return value;
}

static void SignalHandler( int )
{
    if ( g_stopHandler )
        g_stopHandler();
}

struct Args 
{
    std::string m_configPath;
    uint16_t m_httpPort = 8080;
    uint16_t m_wsPort = 8081;
    std::string m_dbPath = "./data/iot.db";
    std::string m_mqttBroker = "tcp://localhost:1883";
    std::string m_logLevel = "info";
    bool m_serve = false;
    bool m_verbose = false;
    bool m_help = false;
};

static void PrintHelp( const char* prog )
{
    std::cout << "Usage: " << prog << " [options]\n\n"
              << "  --port <n>          HTTP port (default: 8080)\n"
              << "  --ws-port <n>       WebSocket port (default: 8081)\n"
              << "  --db <path>         Database path (default: ./data/iot.db)\n"
              << "  --mqtt-broker <url> MQTT broker\n"
              << "  --log-level <lvl>   trace|debug|info|warn|error\n"
              << "  --serve             Start server (blocking)\n"
              << "  --verbose           Debug logging\n"
              << "  --help              Show this help\n";
}

static Args ParseArgs( int argc, char** argv )
{
    Args args;
    for ( int i = 1; i < argc; ++i )
    {
        std::string arg = argv[i];
        if ( arg == "--help" || arg == "-h" )
            args.m_help = true;
        else if ( arg == "--port" && i + 1 < argc )
            args.m_httpPort = static_cast<uint16_t>( std::stoi( argv[++i] ) );
        else if ( arg == "--ws-port" && i + 1 < argc )
            args.m_wsPort = static_cast<uint16_t>( std::stoi( argv[++i] ) );
        else if ( arg == "--db" && i + 1 < argc )
            args.m_dbPath = argv[++i];
        else if ( arg == "--mqtt-broker" && i + 1 < argc )
            args.m_mqttBroker = argv[++i];
        else if ( arg == "--log-level" && i + 1 < argc )
            args.m_logLevel = argv[++i];
        else if ( arg == "--serve" )
            args.m_serve = true;
        else if ( arg == "--verbose" || arg == "-v" )
        {
            args.m_verbose = true;
            args.m_logLevel = "debug";
        }
    }
    return args;
}

int main( int argc, char** argv )
{
    Args args = ParseArgs( argc, argv );

    if ( args.m_help )
    {
        PrintHelp( argv[0] );
        return 0;
    }

    InitializeLogging( args.m_logLevel );
    auto logger = CreateLogger( "Main" );

    const AppConfig defaults{};

    AppConfig config;
    config.m_httpPort = args.m_httpPort;
    config.m_wsPort = args.m_wsPort;
    config.m_dbPath = args.m_dbPath;
    config.m_mqttBroker = args.m_mqttBroker;
    config.m_logLevel = args.m_logLevel;
    config.m_jwtSecret = EnvOr( "IOT_JWT_SECRET", config.m_jwtSecret );
    config.m_adminUsername = EnvOr( "IOT_ADMIN_USER", config.m_adminUsername );
    config.m_adminPassword = EnvOr( "IOT_ADMIN_PASSWORD", config.m_adminPassword );

    if ( args.m_serve &&
         ( config.m_jwtSecret == defaults.m_jwtSecret || config.m_adminPassword == defaults.m_adminPassword ) )
    {
        logger->warn(
            "Insecure defaults in use - set IOT_JWT_SECRET and IOT_ADMIN_PASSWORD before exposing this server" );
    }

    Application app( config );

    auto initResult = app.Initialize();
    if ( !initResult )
    {
        logger->error( "Initialization failed" );
        return 1;
    }

    if ( !args.m_serve )
    {
        logger->info( "Initialized OK. Use --serve to start the server." );
        return 0;
    }

    auto startResult = app.Start();
    if ( !startResult )
    {
        logger->error( "Failed to start server" );
        return 1;
    }

    g_stopHandler = [&app]
    {
        app.Stop();
    };
    std::signal( SIGINT, SignalHandler );
    std::signal( SIGTERM, SignalHandler );

    while ( app.IsRunning() )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
    }

    logger->info( "IoT Dashboard exiting" );
    return 0;
}
