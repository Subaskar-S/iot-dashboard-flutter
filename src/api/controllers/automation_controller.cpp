/**
 * @file       automation_controller.cpp
 * @standard   C++23
 */

#include "api/controllers/automation_controller.hpp"
#include "api/middleware/auth_middleware.hpp"
#include "automation/rule_types.hpp"
#include <nlohmann/json.hpp>

namespace iot::api
{
    namespace
    {
        nlohmann::json RuleToJson( const automation::AutomationRule& r )
        {
            nlohmann::json conditions = nlohmann::json::array();
            for ( const auto& c : r.m_conditions )
            {
                conditions.push_back( nlohmann::json( c ) );
            }

            nlohmann::json actions = nlohmann::json::array();
            for ( const auto& a : r.m_actions )
            {
                actions.push_back( nlohmann::json{
                    { "device_id", a.m_deviceId }, { "command", a.m_command }, { "parameters", a.m_parameters } } );
            }

            return { { "id", r.m_id },
                     { "name", r.m_name },
                     { "enabled", r.m_enabled },
                     { "conditions", conditions },
                     { "actions", actions } };
        }
    } // namespace

    AutomationController::AutomationController( automation::RuleEngine& ruleEngine,
                                                security::AuthenticationService& authService )
        : m_ruleEngine( ruleEngine )
        , m_authService( authService )
    {
    }

    void AutomationController::Register( network::http::HttpRouter& router )
    {
        auto readAuth = MakeAuthMiddleware( m_authService, "automation", security::Permission::Read );
        auto writeAuth = MakeAuthMiddleware( m_authService, "automation", security::Permission::Write );

        // GET /automation/rules
        router.AddMiddleware( readAuth );
        router.AddRoute( network::http::HttpMethod::GET, "/automation/rules",
                         [this]( const network::http::HttpRequest&, network::http::HttpResponse& res )
                         {
                             auto result = m_ruleEngine.ListRules();
                             if ( !result )
                             {
                                 res.Error( 500, "InternalError", "Failed to list rules" );
                                 return;
                             }

                             nlohmann::json arr = nlohmann::json::array();
                             for ( const auto& r : result.value() )
                             {
                                 arr.push_back( RuleToJson( r ) );
                             }

                             nlohmann::json j = { { "rules", arr }, { "total", arr.size() } };
                             res.Json( 200, j.dump() );
                         } );

        // POST /automation/rules
        router.AddMiddleware( writeAuth );
        router.AddRoute( network::http::HttpMethod::POST, "/automation/rules",
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

                             automation::AutomationRule rule;
                             rule.m_id = body.value( "id", std::string{} );
                             rule.m_name = body.value( "name", std::string{} );
                             rule.m_enabled = body.value( "enabled", true );
                             rule.m_createdBy = req.GetHeader( "x-user-id" );

                             if ( rule.m_id.empty() || rule.m_name.empty() )
                             {
                                 res.Error( 400, "BadRequest", "id and name required" );
                                 return;
                             }

                             try
                             {
                                 for ( const auto& c : body.at( "conditions" ) )
                                 {
                                     rule.m_conditions.push_back( c.get<automation::Condition>() );
                                 }
                                 for ( const auto& a : body.at( "actions" ) )
                                 {
                                     core::Action action;
                                     action.m_deviceId = a.value( "device_id", std::string{} );
                                     action.m_command = a.value( "command", std::string{} );
                                     action.m_parameters = a.value( "parameters", nlohmann::json::object() );
                                     rule.m_actions.push_back( std::move( action ) );
                                 }
                             }
                             catch ( const nlohmann::json::exception& e )
                             {
                                 res.Error( 400, "BadRequest",
                                            std::string( "Invalid conditions/actions: " ) + e.what() );
                                 return;
                             }

                             auto result = m_ruleEngine.AddRule( std::move( rule ) );
                             if ( !result )
                             {
                                 res.Error( 409, "Conflict", "Rule ID already exists" );
                                 return;
                             }

                             auto created = m_ruleEngine.GetRule( rule.m_id );
                             if ( !created )
                             {
                                 // Fetch what was just stored.
                                 res.Json( 201, body.dump() );
                                 return;
                             }

                             res.Json( 201, RuleToJson( created.value() ).dump() );
                         } );

        // DELETE /automation/rules?id=...
        router.AddMiddleware( writeAuth );
        router.AddRoute( network::http::HttpMethod::DELETE, "/automation/rules",
                         [this]( const network::http::HttpRequest& req, network::http::HttpResponse& res )
                         {
                             std::string id = req.GetQueryParam( "id" );
                             if ( id.empty() )
                             {
                                 res.Error( 400, "BadRequest", "id query parameter required" );
                                 return;
                             }

                             auto result = m_ruleEngine.RemoveRule( id );
                             if ( !result )
                             {
                                 res.Error( 404, "NotFound", "Rule not found" );
                                 return;
                             }

                             res.NoContent();
                         } );
    }

} // namespace iot::api
