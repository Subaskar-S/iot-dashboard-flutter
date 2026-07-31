/**
 * @file       device_controller.hpp
 * @standard   C++23
 */

#ifndef IOT_API_CONTROLLERS_DEVICE_CONTROLLER_HPP
#define IOT_API_CONTROLLERS_DEVICE_CONTROLLER_HPP

#include "devices/device_manager.hpp"
#include "network/http/http_router.hpp"
#include "security/authentication_service.hpp"

namespace iot::api
{
    class DeviceController
    {
        public:
        DeviceController( devices::DeviceManager& deviceManager, security::AuthenticationService& authService );

        void Register( network::http::HttpRouter& router );

        private:
        devices::DeviceManager& m_deviceManager;
        security::AuthenticationService& m_authService;
    };

} // namespace iot::api

#endif // IOT_API_CONTROLLERS_DEVICE_CONTROLLER_HPP
