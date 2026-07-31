/**
 * @file       sensor_controller.cpp
 * @standard   C++23
 */

#include "api/controllers/sensor_controller.hpp"
#include "api/middleware/auth_middleware.hpp"
#include <nlohmann/json.hpp>

namespace iot::api
{
    namespace
    {
        nlohmann::json ReadingToJson( const SensorReading& r )
        {
            nlohmann::json j = {
                { "device_id", r.m_deviceId },
                { "sensor_type", r.m_sensor },
                { "value", r.m_value },
                { "unit", r.m_unit },
                { "timestamp",
                  std::chrono::duration_cast<std::chrono::seconds>( r.m_timestamp.time_since_epoch() ).count() } };
            if ( r.m_quality )
            {
                j["quality"] = r.m_quality.value();
            }
            return j;
        }
    } // namespace

    SensorController::SensorController( database::SqliteSensorRepository& sensorRepo,
                                        security::AuthenticationService& authService )
        : m_sensorRepo( sensorRepo )
        , m_authService( authService )
    {
    }

    void SensorController::Register( network::http::HttpRouter& router )
    {
        auto readAuth = MakeAuthMiddleware( m_authService, "sensors", security::Permission::Read );

        // GET /sensors
        router.AddMiddleware( readAuth );
        router.AddRoute( network::http::HttpMethod::GET, "/sensors",
                         [this]( const network::http::HttpRequest& req, network::http::HttpResponse& res )
                         {
                             database::SensorQueryOptions opts;
                             opts.m_deviceId = req.GetQueryParam( "device_id" );
                             opts.m_sensorType = req.GetQueryParam( "sensor_type" );

                             std::string limitStr = req.GetQueryParam( "limit" );
                             if ( !limitStr.empty() )
                             {
                                 try
                                 {
                                     opts.m_limit = static_cast<size_t>( std::stoul( limitStr ) );
                                 }
                                 catch ( ... )
                                 {
                                 }
                             }

                             if ( opts.m_deviceId.empty() )
                             {
                                 res.Error( 400, "BadRequest", "device_id is required" );
                                 return;
                             }

                             auto result = m_sensorRepo.Query( opts );
                             if ( !result )
                             {
                                 res.Error( 500, "InternalError", "Failed to query sensors" );
                                 return;
                             }

                             nlohmann::json arr = nlohmann::json::array();
                             for ( const auto& r : result.value() )
                             {
                                 arr.push_back( ReadingToJson( r ) );
                             }

                             nlohmann::json j = { { "readings", arr }, { "total", arr.size() } };
                             res.Json( 200, j.dump() );
                         } );
    }

} // namespace iot::api
