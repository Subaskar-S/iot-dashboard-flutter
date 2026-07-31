/**
 * @file       health_controller.cpp
 * @standard   C++23
 */

#include "api/controllers/health_controller.hpp"
#include <nlohmann/json.hpp>

namespace iot::api
{
    HealthController::HealthController( const std::string& version )
        : m_version( version )
        , m_startTime( std::chrono::steady_clock::now() )
    {
    }

    void HealthController::Register( network::http::HttpRouter& router )
    {
        router.AddRoute(
            network::http::HttpMethod::GET, "/health",
            [this]( const network::http::HttpRequest&, network::http::HttpResponse& res )
            {
                auto uptime =
                    std::chrono::duration_cast<std::chrono::seconds>( std::chrono::steady_clock::now() - m_startTime )
                        .count();

                nlohmann::json j = { { "status", "healthy" }, { "version", m_version }, { "uptime_seconds", uptime } };

                res.Json( 200, j.dump() );
            } );
    }

} // namespace iot::api
