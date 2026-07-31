/**
 * @file       sensor_controller.hpp
 * @standard   C++23
 */

#ifndef IOT_API_CONTROLLERS_SENSOR_CONTROLLER_HPP
#define IOT_API_CONTROLLERS_SENSOR_CONTROLLER_HPP

#include "database/sensor_repository.hpp"
#include "network/http/http_router.hpp"
#include "security/authentication_service.hpp"

namespace iot::api
{
    class SensorController
    {
        public:
        SensorController( database::SqliteSensorRepository& sensorRepo, security::AuthenticationService& authService );

        void Register( network::http::HttpRouter& router );

        private:
        database::SqliteSensorRepository& m_sensorRepo;
        security::AuthenticationService& m_authService;
    };

} // namespace iot::api

#endif // IOT_API_CONTROLLERS_SENSOR_CONTROLLER_HPP
