/**
 * @file       device_controller.cpp
 * @standard   C++23
 */

#include "api/controllers/device_controller.hpp"
#include "api/middleware/auth_middleware.hpp"
#include <nlohmann/json.hpp>

namespace iot::api
{
    namespace
    {
        nlohmann::json DeviceToJson( const DeviceInfo& d )
        {
            return { { "id", d.m_id },           { "name", d.m_name },
                     { "type", d.m_type },       { "protocol", d.m_protocol },
                     { "address", d.m_address }, { "online", d.m_isConnected } };
        }
    } // namespace

    DeviceController::DeviceController( devices::DeviceManager& deviceManager,
                                        security::AuthenticationService& authService )
        : m_deviceManager( deviceManager )
        , m_authService( authService )
    {
    }

    void DeviceController::Register( network::http::HttpRouter& router )
    {
        auto readAuth = MakeAuthMiddleware( m_authService, "devices", security::Permission::Read );
        auto writeAuth = MakeAuthMiddleware( m_authService, "devices", security::Permission::Write );

        // GET /devices
        router.AddMiddleware( readAuth );
        router.AddRoute( network::http::HttpMethod::GET, "/devices",
                         [this]( const network::http::HttpRequest& req, network::http::HttpResponse& res )
                         {
                             devices::DeviceFilter filter;
                             filter.m_type = req.GetQueryParam( "type" );
                             filter.m_onlineOnly = req.GetQueryParam( "online" ) == "true";

                             auto result = m_deviceManager.List( filter );
                             if ( !result )
                             {
                                 res.Error( 500, "InternalError", "Failed to list devices" );
                                 return;
                             }

                             nlohmann::json arr = nlohmann::json::array();
                             for ( const auto& d : result.value() )
                             {
                                 arr.push_back( DeviceToJson( d ) );
                             }

                             nlohmann::json j = { { "devices", arr }, { "total", arr.size() } };
                             res.Json( 200, j.dump() );
                         } );

        // POST /devices
        router.AddMiddleware( writeAuth );
        router.AddRoute( network::http::HttpMethod::POST, "/devices",
                         [this]( const network::http::HttpRequest& req, network::http::HttpResponse& res )
                         {
                             nlohmann::json body;
                             try
                             {
                                 body = nlohmann::json::parse( req.m_body );
                             }
                             catch ( ... )
                             {
                                 res.Error( 400, "BadRequest", "Invalid JSON" );
                                 return;
                             }

                             devices::RegisterDeviceRequest request{
                                 .m_id = body.value( "id", std::string{} ),
                                 .m_name = body.value( "name", std::string{} ),
                                 .m_type = body.value( "type", std::string{} ),
                                 .m_protocol = body.value( "protocol", std::string{ "mqtt" } ),
                                 .m_address = body.value( "address", std::string{} ) };

                             if ( request.m_id.empty() || request.m_name.empty() )
                             {
                                 res.Error( 400, "BadRequest", "id and name are required" );
                                 return;
                             }

                             auto result = m_deviceManager.Register( request );
                             if ( !result )
                             {
                                 res.Error( 409, "Conflict", "Device already exists or invalid data" );
                                 return;
                             }

                             res.Json( 201, DeviceToJson( result.value() ).dump() );
                         } );

        // DELETE /devices/{id}  — path param extracted from query for simplicity
        router.AddMiddleware( writeAuth );
        router.AddRoute( network::http::HttpMethod::DELETE, "/devices",
                         [this]( const network::http::HttpRequest& req, network::http::HttpResponse& res )
                         {
                             std::string id = req.GetQueryParam( "id" );
                             if ( id.empty() )
                             {
                                 res.Error( 400, "BadRequest", "id query parameter required" );
                                 return;
                             }

                             auto result = m_deviceManager.Unregister( id );
                             if ( !result )
                             {
                                 res.Error( 404, "NotFound", "Device not found" );
                                 return;
                             }

                             res.NoContent();
                         } );

        // POST /commands
        router.AddMiddleware( writeAuth );
        router.AddRoute( network::http::HttpMethod::POST, "/commands",
                         [this]( const network::http::HttpRequest& req, network::http::HttpResponse& res )
                         {
                             nlohmann::json body;
                             try
                             {
                                 body = nlohmann::json::parse( req.m_body );
                             }
                             catch ( ... )
                             {
                                 res.Error( 400, "BadRequest", "Invalid JSON" );
                                 return;
                             }

                             std::string deviceId = body.value( "device_id", std::string{} );
                             std::string command = body.value( "command", std::string{} );
                             std::string params = body.value( "parameters", nlohmann::json::object() ).dump();

                             if ( deviceId.empty() || command.empty() )
                             {
                                 res.Error( 400, "BadRequest", "device_id and command required" );
                                 return;
                             }

                             auto result = m_deviceManager.SendCommand( deviceId, command, params );
                             if ( !result )
                             {
                                 if ( result.error() == Error::DeviceNotFound )
                                 {
                                     res.Error( 404, "NotFound", "Device not found" );
                                 }
                                 else if ( result.error() == Error::DeviceOffline )
                                 {
                                     res.Error( 422, "DeviceOffline", "Device is offline" );
                                 }
                                 else
                                 {
                                     res.Error( 500, "InternalError", "Failed to send command" );
                                 }
                                 return;
                             }

                             nlohmann::json j = {
                                 { "status", "accepted" }, { "device_id", deviceId }, { "command", command } };
                             res.Json( 202, j.dump() );
                         } );
    }

} // namespace iot::api
