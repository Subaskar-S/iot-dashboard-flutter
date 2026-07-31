/**
 * @file       auth_controller.hpp
 * @standard   C++23
 */

#ifndef IOT_API_CONTROLLERS_AUTH_CONTROLLER_HPP
#define IOT_API_CONTROLLERS_AUTH_CONTROLLER_HPP

#include "network/http/http_router.hpp"
#include "security/authentication_service.hpp"

namespace iot::api
{
    class AuthController
    {
        public:
        explicit AuthController( security::AuthenticationService& authService );

        void Register( network::http::HttpRouter& router );

        private:
        security::AuthenticationService& m_authService;
    };

} // namespace iot::api

#endif // IOT_API_CONTROLLERS_AUTH_CONTROLLER_HPP
