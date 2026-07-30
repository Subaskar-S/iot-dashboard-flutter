/**
 * @file       http_session.hpp
 * @brief      Beast per-connection HTTP session
 * @standard   C++23
 */

#ifndef IOT_NETWORK_HTTP_HTTP_SESSION_HPP
#define IOT_NETWORK_HTTP_HTTP_SESSION_HPP

#include "network/http/http_router.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <spdlog/spdlog.h>

namespace iot::network::http
{
    namespace beast = boost::beast;
    namespace bhttp = boost::beast::http;
    namespace asio = boost::asio;

    /**
     * One HTTP/1.1 keep-alive session per accepted TCP connection.
     * Lives entirely on the io_context thread pool — no locking needed
     * inside this class.  Destroyed when the connection closes or errors.
     */
    class HttpSession : public std::enable_shared_from_this<HttpSession>
    {
        public:
        HttpSession( asio::ip::tcp::socket socket,
                     const HttpRouter& router,
                     bool enableCors,
                     std::shared_ptr<spdlog::logger> logger );

        void Run();

        private:
        beast::tcp_stream m_stream;
        beast::flat_buffer m_buffer;
        bhttp::request<bhttp::string_body> m_beastReq;

        const HttpRouter& m_router;
        bool m_enableCors;
        std::shared_ptr<spdlog::logger> m_logger;

        void DoRead();
        void OnRead( beast::error_code ec, std::size_t bytes );
        void SendResponse( HttpResponse appRes );
        void OnWrite( bool close, beast::error_code ec, std::size_t bytes );

        static HttpRequest BeastToApp( const bhttp::request<bhttp::string_body>& bReq );
        static bhttp::response<bhttp::string_body> AppToBeast( const HttpResponse& appRes,
                                                               unsigned httpVersion,
                                                               bool keepAlive );
    };

} // namespace iot::network::http

#endif // IOT_NETWORK_HTTP_HTTP_SESSION_HPP
