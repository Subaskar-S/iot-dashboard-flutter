/**
 * @file       health_controller.hpp
 * @standard   C++23
 */

#ifndef IOT_API_CONTROLLERS_HEALTH_CONTROLLER_HPP
#define IOT_API_CONTROLLERS_HEALTH_CONTROLLER_HPP

#include "network/http/http_router.hpp"
#include <atomic>
#include <chrono>

namespace iot::api
{
    class HealthController
    {
        public:
        explicit HealthController( const std::string& version = "1.0.0" );

        void Register( network::http::HttpRouter& router );

        private:
        std::string m_version;
        std::chrono::steady_clock::time_point m_startTime;
    };

} // namespace iot::api

#endif // IOT_API_CONTROLLERS_HEALTH_CONTROLLER_HPP
