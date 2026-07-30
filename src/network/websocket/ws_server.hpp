/**
 * @file       ws_server.hpp
 * @brief      Boost.Beast async WebSocket server with pub/sub topics
 * @standard   C++23
 */

#ifndef IOT_NETWORK_WEBSOCKET_WS_SERVER_HPP
#define IOT_NETWORK_WEBSOCKET_WS_SERVER_HPP

#include "common/error.hpp"
#include "network/websocket/ws_types.hpp"
#include <atomic>
#include <boost/asio.hpp>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace iot::network::websocket
{
    struct WsServerConfig
    {
        std::string m_host = "0.0.0.0";
        uint16_t m_port = 8081;
        uint32_t m_threadCount = 2;
        uint32_t m_heartbeatIntervalSeconds = 30;
        size_t m_maxMessageBytes = 65536;
    };

    class WsSession; // forward declaration

    /**
     * Boost.Beast async WebSocket server.
     *
     * Lifecycle:
     *   1. Construct with config + callbacks.
     *   2. Call Start() — non-blocking, spins worker threads.
     *   3. Use Send() / Broadcast() / Subscribe() at any time.
     *   4. Call Stop() or destroy.
     *
     * Thread-safety: all public methods lock m_sessionsMutex.
     */
    class WsServer
    {
        public:
        explicit WsServer( WsServerConfig config );
        ~WsServer();

        WsServer( const WsServer& ) = delete;
        WsServer& operator=( const WsServer& ) = delete;

        [[nodiscard]] Result<void> Start();
        void Stop();
        [[nodiscard]] bool IsRunning() const noexcept;

        // ── Callbacks ──────────────────────────────────────────────────────
        void OnConnect( ConnectCallback cb );
        void OnDisconnect( DisconnectCallback cb );
        void OnMessage( MessageCallback cb );

        // ── Messaging ──────────────────────────────────────────────────────
        /// Send a text frame to a single connection.
        void Send( ConnectionId id, std::string_view message );

        /// Broadcast to all connected clients.
        void Broadcast( std::string_view message );

        /// Broadcast to all connections subscribed to a topic.
        void BroadcastToTopic( std::string_view topic, std::string_view message );

        // ── Topic pub/sub ─────────────────────────────────────────────────
        void Subscribe( ConnectionId id, std::string_view topic );
        void Unsubscribe( ConnectionId id, std::string_view topic );

        [[nodiscard]] size_t ConnectionCount() const;

        private:
        WsServerConfig m_config;

        boost::asio::io_context m_ioc;
        boost::asio::ip::tcp::acceptor m_acceptor;
        std::vector<std::thread> m_threads;
        std::atomic<bool> m_running{ false };
        std::atomic<ConnectionId> m_nextId{ 1 };

        // Active sessions
        mutable std::mutex m_sessionsMutex;
        std::unordered_map<ConnectionId, std::shared_ptr<WsSession>> m_sessions;

        // Topic → set of connection IDs
        std::unordered_map<std::string, std::unordered_set<ConnectionId>> m_topics;

        ConnectCallback m_onConnect;
        DisconnectCallback m_onDisconnect;
        MessageCallback m_onMessage;
        std::shared_ptr<spdlog::logger> m_logger;

        void AcceptLoop();

        // Called by WsSession on its own lifecycle events.
        friend class WsSession;
        void OnSessionConnected( ConnectionId id, std::shared_ptr<WsSession> session );
        void OnSessionDisconnected( ConnectionId id );
        void OnSessionMessage( ConnectionId id, std::string payload );
    };

} // namespace iot::network::websocket

#endif // IOT_NETWORK_WEBSOCKET_WS_SERVER_HPP
