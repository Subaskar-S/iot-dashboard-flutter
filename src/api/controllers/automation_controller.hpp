/**
 * @file       automation_controller.hpp
 * @standard   C++23
 */

#ifndef IOT_API_CONTROLLERS_AUTOMATION_CONTROLLER_HPP
#define IOT_API_CONTROLLERS_AUTOMATION_CONTROLLER_HPP

#include "automation/rule_engine.hpp"
#include "network/http/http_router.hpp"
#include "security/authentication_service.hpp"

namespace iot::api
{
    class AutomationController
    {
        public:
        AutomationController( automation::RuleEngine& ruleEngine, security::AuthenticationService& authService );

        void Register( network::http::HttpRouter& router );

        private:
        automation::RuleEngine& m_ruleEngine;
        security::AuthenticationService& m_authService;
    };

} // namespace iot::api

#endif // IOT_API_CONTROLLERS_AUTOMATION_CONTROLLER_HPP
