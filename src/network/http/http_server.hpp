/**
 * @file       http_server.hpp
 * @brief      Boost.Beast async HTTP/1.1 server
 * @standard   C++23
 */

#ifndef IOT_NETWORK_HTTP_HTTP_SERVER_HPP
#define IOT_NETWORK_HTTP_HTTP_SERVER_HPP

#include "common/error.hpp"
#include "network/http/http_router.hpp"
#include <boost/asio.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <vector>

namespace iot::network::http
{
    struct HttpServerConfig
    {
        std::string m_host = "0.0.0.0";
        uint16_t m_port = 8080;
        uint32_t m_threadCount = 4; // io_context thread pool size
        uint32_t m_requestTimeoutMs = 30000;
        bool m_enableCors = true;
    };

    /**
     * Async HTTP/1.1 server built on Boost.Beast + Boost.Asio.
     *
     * Lifecycle:
     *   1. Construct with config.
     *   2. Configure routes on Router().
     *   3. Call Start() — non-blocking, spins up m_threadCount threads.
     *   4. Call Stop() or destroy.
     *
     * Each connection is handled by a Beast session object that lives
     * on the io_context thread pool; the main thread is never blocked.
     */
    class HttpServer
    {
        public:
        explicit HttpServer( HttpServerConfig config );
        ~HttpServer();

        HttpServer( const HttpServer& ) = delete;
        HttpServer& operator=( const HttpServer& ) = delete;

        [[nodiscard]] Result<void> Start();
        void Stop();

        [[nodiscard]] bool IsRunning() const noexcept;

        /// Access the router to register routes before Start().
        [[nodiscard]] HttpRouter& Router() noexcept
        {
            return m_router;
        }

        private:
        HttpServerConfig m_config;
        HttpRouter m_router;

        boost::asio::io_context m_ioc;
        boost::asio::ip::tcp::acceptor m_acceptor;
        std::vector<std::thread> m_threads;
        std::atomic<bool> m_running{ false };

        std::shared_ptr<spdlog::logger> m_logger;

        void AcceptLoop();
    };

} // namespace iot::network::http

#endif // IOT_NETWORK_HTTP_HTTP_SERVER_HPP
