/**
 * @file       ws_test.cpp
 * @brief      Unit + integration tests for WebSocket server
 * @standard   C++23
 *
 * Unit tests: WsTypes, topic pub/sub logic, server state before Start().
 * Integration tests: real TCP WebSocket connections via Beast client.
 *   Self-skip when the target port is busy.
 */

#include "network/websocket/ws_server.hpp"
#include "network/websocket/ws_types.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

namespace
{
    using namespace iot::network::websocket;
    namespace asio = boost::asio;
    namespace beast = boost::beast;
    namespace bws = boost::beast::websocket;

    constexpr uint16_t kTestPort = 19080;

    static bool PortFree( uint16_t port )
    {
        try
        {
            asio::io_context ioc;
            asio::ip::tcp::acceptor a( ioc );
            asio::ip::tcp::endpoint ep{ asio::ip::tcp::v4(), port };
            a.open( ep.protocol() );
            a.set_option( asio::socket_base::reuse_address( true ) );
            a.bind( ep );
            return true;
        }
        catch ( ... )
        {
            return false;
        }
    }

#define SKIP_IF_PORT_BUSY( port )                                                                                      \
    if ( !PortFree( port ) )                                                                                           \
    {                                                                                                                  \
        GTEST_SKIP() << "Port " #port " not free, skipping";                                                           \
    }

    // ── Simple blocking Beast WebSocket client ───────────────────────────

    class SyncWsClient
    {
        public:
        explicit SyncWsClient( uint16_t port )
            : m_stream( m_ioc )
        {
            asio::ip::tcp::resolver resolver( m_ioc );
            auto endpoints = resolver.resolve( "127.0.0.1", std::to_string( port ) );
            auto ep = asio::connect( m_stream.next_layer().socket(), endpoints );
            m_stream.handshake( "127.0.0.1:" + std::to_string( ep.port() ), "/" );
        }

        void Send( std::string_view msg )
        {
            m_stream.write( asio::buffer( msg ) );
        }

        std::string Receive()
        {
            beast::flat_buffer buf;
            m_stream.read( buf );
            return beast::buffers_to_string( buf.data() );
        }

        void Close()
        {
            m_stream.close( bws::close_code::normal );
        }

        private:
        asio::io_context m_ioc;
        bws::stream<beast::tcp_stream> m_stream;
    };

    // ─────────────────────────────────────────────────────────────────────
    // Unit tests — no network I/O
    // ─────────────────────────────────────────────────────────────────────

    TEST( WsServerUnitTest, IsNotRunningBeforeStart )
    {
        WsServerConfig cfg;
        cfg.m_port = kTestPort;
        WsServer server( cfg );
        EXPECT_FALSE( server.IsRunning() );
    }

    TEST( WsServerUnitTest, ConnectionCountZeroBeforeStart )
    {
        WsServerConfig cfg;
        cfg.m_port = kTestPort;
        WsServer server( cfg );
        EXPECT_EQ( server.ConnectionCount(), 0u );
    }

    TEST( WsServerUnitTest, SetCallbacksDoNotThrow )
    {
        WsServerConfig cfg;
        cfg.m_port = kTestPort;
        WsServer server( cfg );

        EXPECT_NO_THROW( server.OnConnect( []( ConnectionId ) {} ) );
        EXPECT_NO_THROW( server.OnDisconnect( []( ConnectionId ) {} ) );
        EXPECT_NO_THROW( server.OnMessage( []( const WsMessage& ) {} ) );
    }

    TEST( WsServerUnitTest, SubscribeBeforeStartDoesNotCrash )
    {
        WsServerConfig cfg;
        cfg.m_port = kTestPort;
        WsServer server( cfg );

        EXPECT_NO_THROW( server.Subscribe( 42, "sensors" ) );
        EXPECT_NO_THROW( server.Unsubscribe( 42, "sensors" ) );
    }

    TEST( WsServerUnitTest, BroadcastBeforeStartDoesNotCrash )
    {
        WsServerConfig cfg;
        cfg.m_port = kTestPort;
        WsServer server( cfg );
        EXPECT_NO_THROW( server.Broadcast( "hello" ) );
        EXPECT_NO_THROW( server.BroadcastToTopic( "sensors", "data" ) );
    }

    // ─────────────────────────────────────────────────────────────────────
    // Integration tests — real TCP WebSocket connections
    // ─────────────────────────────────────────────────────────────────────

    TEST( WsServerIntegrationTest, StartAndStop )
    {
        SKIP_IF_PORT_BUSY( kTestPort );

        WsServerConfig cfg;
        cfg.m_port = kTestPort;
        cfg.m_threadCount = 2;
        WsServer server( cfg );

        ASSERT_TRUE( server.Start().has_value() );
        EXPECT_TRUE( server.IsRunning() );

        server.Stop();
        EXPECT_FALSE( server.IsRunning() );
    }

    TEST( WsServerIntegrationTest, ClientConnectTriggersCallback )
    {
        SKIP_IF_PORT_BUSY( kTestPort );

        WsServerConfig cfg;
        cfg.m_port = kTestPort;
        cfg.m_threadCount = 2;
        WsServer server( cfg );

        std::atomic<int> connectCount{ 0 };
        server.OnConnect(
            [&]( ConnectionId )
            {
                connectCount++;
            } );

        ASSERT_TRUE( server.Start().has_value() );

        {
            SyncWsClient client( kTestPort );
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
            EXPECT_EQ( connectCount.load(), 1 );
            EXPECT_EQ( server.ConnectionCount(), 1u );
            client.Close();
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        }

        EXPECT_EQ( server.ConnectionCount(), 0u );
        server.Stop();
    }

    TEST( WsServerIntegrationTest, ServerSendsMessageToClient )
    {
        SKIP_IF_PORT_BUSY( kTestPort );

        WsServerConfig cfg;
        cfg.m_port = kTestPort;
        cfg.m_threadCount = 2;
        WsServer server( cfg );

        ConnectionId connId{ 0 };
        server.OnConnect(
            [&]( ConnectionId id )
            {
                connId = id;
            } );

        ASSERT_TRUE( server.Start().has_value() );

        SyncWsClient client( kTestPort );
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

        server.Send( connId, R"({"type":"ping"})" );

        std::string received = client.Receive();
        EXPECT_NE( received.find( "ping" ), std::string::npos );

        client.Close();
        server.Stop();
    }

    TEST( WsServerIntegrationTest, ClientSendsMessageTriggersCallback )
    {
        SKIP_IF_PORT_BUSY( kTestPort );

        WsServerConfig cfg;
        cfg.m_port = kTestPort;
        cfg.m_threadCount = 2;
        WsServer server( cfg );

        std::string received;
        std::mutex mu;
        server.OnMessage(
            [&]( const WsMessage& msg )
            {
                std::lock_guard lock( mu );
                received = msg.m_payload;
            } );

        ASSERT_TRUE( server.Start().has_value() );

        SyncWsClient client( kTestPort );
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

        client.Send( R"({"type":"subscribe","topic":"sensors"})" );
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

        std::lock_guard lock( mu );
        EXPECT_NE( received.find( "subscribe" ), std::string::npos );

        server.Stop();
    }

    TEST( WsServerIntegrationTest, BroadcastToTopicReachesSubscribedClients )
    {
        SKIP_IF_PORT_BUSY( kTestPort );

        WsServerConfig cfg;
        cfg.m_port = kTestPort;
        cfg.m_threadCount = 2;
        WsServer server( cfg );

        // Subscribe both clients to the same topic server-side on connect.
        server.OnConnect(
            [&]( ConnectionId id )
            {
                server.Subscribe( id, "sensors" );
            } );

        ASSERT_TRUE( server.Start().has_value() );

        SyncWsClient c1( kTestPort );
        SyncWsClient c2( kTestPort );
        std::this_thread::sleep_for( std::chrono::milliseconds( 150 ) );

        EXPECT_EQ( server.ConnectionCount(), 2u );

        server.BroadcastToTopic( "sensors", R"({"value":42})" );

        std::string msg1 = c1.Receive();
        std::string msg2 = c2.Receive();

        EXPECT_NE( msg1.find( "42" ), std::string::npos );
        EXPECT_NE( msg2.find( "42" ), std::string::npos );

        c1.Close();
        c2.Close();
        server.Stop();
    }

    TEST( WsServerIntegrationTest, UnsubscribeStopsTopicMessages )
    {
        SKIP_IF_PORT_BUSY( kTestPort );

        WsServerConfig cfg;
        cfg.m_port = kTestPort;
        cfg.m_threadCount = 2;
        WsServer server( cfg );

        ConnectionId connId{ 0 };
        server.OnConnect(
            [&]( ConnectionId id )
            {
                connId = id;
                server.Subscribe( id, "alerts" );
            } );

        ASSERT_TRUE( server.Start().has_value() );

        SyncWsClient client( kTestPort );
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

        // Verify subscribed — should receive broadcast.
        server.BroadcastToTopic( "alerts", R"({"alert":"fire"})" );
        std::string msg = client.Receive();
        EXPECT_NE( msg.find( "fire" ), std::string::npos );

        // Unsubscribe — no more messages.
        server.Unsubscribe( connId, "alerts" );
        server.BroadcastToTopic( "alerts", R"({"alert":"smoke"})" );

        // Broadcast to "sensors" so client gets something it can receive.
        server.Subscribe( connId, "sensors" );
        server.BroadcastToTopic( "sensors", R"({"ok":true})" );
        msg = client.Receive();
        EXPECT_NE( msg.find( "ok" ), std::string::npos );

        client.Close();
        server.Stop();
    }

} // namespace
