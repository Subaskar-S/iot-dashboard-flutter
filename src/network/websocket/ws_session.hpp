/**
 * @file       ws_session.hpp
 * @brief      Beast per-connection WebSocket session with heartbeat
 * @standard   C++23
 */

#ifndef IOT_NETWORK_WEBSOCKET_WS_SESSION_HPP
#define IOT_NETWORK_WEBSOCKET_WS_SESSION_HPP

#include "network/websocket/ws_types.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <deque>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>

namespace iot::network::websocket
{
    namespace beast = boost::beast;
    namespace asio = boost::asio;

    class WsServer;

    /**
     * One WebSocket connection.  Lives on the io_context thread pool.
     * Send() is safe to call from any thread — messages are queued and
     * written sequentially via async_write chaining.
     */
    class WsSession : public std::enable_shared_from_this<WsSession>
    {
        public:
        WsSession( asio::ip::tcp::socket socket,
                   ConnectionId id,
                   WsServer& server,
                   size_t maxMessageBytes,
                   std::shared_ptr<spdlog::logger> logger );

        void Run();

        /// Thread-safe: queues message and triggers write if idle.
        void Send( std::string message );

        void Close();

        [[nodiscard]] ConnectionId Id() const noexcept
        {
            return m_id;
        }

        private:
        beast::websocket::stream<beast::tcp_stream> m_ws;
        ConnectionId m_id;
        WsServer& m_server;
        size_t m_maxMessageBytes;
        std::shared_ptr<spdlog::logger> m_logger;

        beast::flat_buffer m_readBuf;

        // Write queue — async_write may only have one in-flight at a time.
        std::mutex m_writeMutex;
        std::deque<std::string> m_writeQueue;
        bool m_writing = false;

        void DoAccept();
        void OnAccept( beast::error_code ec );
        void DoRead();
        void OnRead( beast::error_code ec, std::size_t bytes );
        void DoWrite();
        void OnWrite( beast::error_code ec, std::size_t bytes );
        void OnClose( beast::error_code ec );
    };

} // namespace iot::network::websocket

#endif // IOT_NETWORK_WEBSOCKET_WS_SESSION_HPP
