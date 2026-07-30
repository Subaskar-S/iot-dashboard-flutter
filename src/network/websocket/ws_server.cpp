/**
 * @file       ws_server.cpp
 * @brief      Beast async WebSocket server — accept loop, pub/sub, broadcast
 * @standard   C++23
 */

#include "network/websocket/ws_server.hpp"
#include "common/logging.hpp"
#include "network/websocket/ws_session.hpp"

namespace iot::network::websocket
{
    namespace asio = boost::asio;
    namespace beast = boost::beast;

    WsServer::WsServer( WsServerConfig config )
        : m_config( std::move( config ) )
        , m_ioc( static_cast<int>( m_config.m_threadCount ) )
        , m_acceptor( m_ioc )
        , m_logger( CreateLogger( "WsServer" ) )
    {
    }

    WsServer::~WsServer()
    {
        Stop();
    }

    Result<void> WsServer::Start()
    {
        beast::error_code ec;

        asio::ip::tcp::endpoint endpoint{ asio::ip::make_address( m_config.m_host ), m_config.m_port };

        m_acceptor.open( endpoint.protocol(), ec );
        if ( ec )
        {
            m_logger->error( "WS acceptor open: {}", ec.message() );
            return std::unexpected( Error::NetworkError );
        }

        m_acceptor.set_option( asio::socket_base::reuse_address( true ), ec );
        m_acceptor.bind( endpoint, ec );
        if ( ec )
        {
            m_logger->error( "WS acceptor bind {}:{}: {}", m_config.m_host, m_config.m_port, ec.message() );
            return std::unexpected( Error::NetworkError );
        }

        m_acceptor.listen( asio::socket_base::max_listen_connections, ec );
        if ( ec )
        {
            m_logger->error( "WS acceptor listen: {}", ec.message() );
            return std::unexpected( Error::NetworkError );
        }

        m_running = true;
        AcceptLoop();

        m_threads.reserve( m_config.m_threadCount );
        for ( uint32_t i = 0; i < m_config.m_threadCount; ++i )
        {
            m_threads.emplace_back(
                [this]
                {
                    m_ioc.run();
                } );
        }

        m_logger->info( "WebSocket server listening on {}:{} ({} threads)", m_config.m_host, m_config.m_port,
                        m_config.m_threadCount );
        return {};
    }

    void WsServer::Stop()
    {
        if ( !m_running.exchange( false ) )
        {
            return;
        }

        beast::error_code ec;
        m_acceptor.close( ec );

        // Close all active sessions.
        {
            std::lock_guard lock( m_sessionsMutex );
            for ( auto& [id, session] : m_sessions )
            {
                session->Close();
            }
        }

        m_ioc.stop();

        for ( auto& t : m_threads )
        {
            if ( t.joinable() )
            {
                t.join();
            }
        }
        m_threads.clear();
        m_logger->info( "WebSocket server stopped" );
    }

    bool WsServer::IsRunning() const noexcept
    {
        return m_running.load();
    }

    // ── Callbacks ─────────────────────────────────────────────────────────

    void WsServer::OnConnect( ConnectCallback cb )
    {
        m_onConnect = std::move( cb );
    }

    void WsServer::OnDisconnect( DisconnectCallback cb )
    {
        m_onDisconnect = std::move( cb );
    }

    void WsServer::OnMessage( MessageCallback cb )
    {
        m_onMessage = std::move( cb );
    }

    // ── Messaging ─────────────────────────────────────────────────────────

    void WsServer::Send( ConnectionId id, std::string_view message )
    {
        std::lock_guard lock( m_sessionsMutex );
        auto it = m_sessions.find( id );
        if ( it != m_sessions.end() )
        {
            it->second->Send( std::string( message ) );
        }
    }

    void WsServer::Broadcast( std::string_view message )
    {
        std::lock_guard lock( m_sessionsMutex );
        for ( auto& [id, session] : m_sessions )
        {
            session->Send( std::string( message ) );
        }
    }

    void WsServer::BroadcastToTopic( std::string_view topic, std::string_view message )
    {
        std::lock_guard lock( m_sessionsMutex );
        auto it = m_topics.find( std::string( topic ) );
        if ( it == m_topics.end() )
        {
            return;
        }

        for ( ConnectionId id : it->second )
        {
            auto sIt = m_sessions.find( id );
            if ( sIt != m_sessions.end() )
            {
                sIt->second->Send( std::string( message ) );
            }
        }
    }

    // ── Topic pub/sub ─────────────────────────────────────────────────────

    void WsServer::Subscribe( ConnectionId id, std::string_view topic )
    {
        std::lock_guard lock( m_sessionsMutex );
        m_topics[std::string( topic )].insert( id );
    }

    void WsServer::Unsubscribe( ConnectionId id, std::string_view topic )
    {
        std::lock_guard lock( m_sessionsMutex );
        auto it = m_topics.find( std::string( topic ) );
        if ( it != m_topics.end() )
        {
            it->second.erase( id );
            if ( it->second.empty() )
            {
                m_topics.erase( it );
            }
        }
    }

    size_t WsServer::ConnectionCount() const
    {
        std::lock_guard lock( m_sessionsMutex );
        return m_sessions.size();
    }

    // ── Internal session lifecycle ─────────────────────────────────────────

    void WsServer::OnSessionConnected( ConnectionId id, std::shared_ptr<WsSession> session )
    {
        {
            std::lock_guard lock( m_sessionsMutex );
            m_sessions[id] = std::move( session );
        }

        m_logger->debug( "WS client connected [{}], total={}", id, ConnectionCount() );

        if ( m_onConnect )
        {
            m_onConnect( id );
        }
    }

    void WsServer::OnSessionDisconnected( ConnectionId id )
    {
        {
            std::lock_guard lock( m_sessionsMutex );
            m_sessions.erase( id );

            // Remove from all topic subscriptions.
            for ( auto& [topic, ids] : m_topics )
            {
                ids.erase( id );
            }
        }

        m_logger->debug( "WS client disconnected [{}], total={}", id, ConnectionCount() );

        if ( m_onDisconnect )
        {
            m_onDisconnect( id );
        }
    }

    void WsServer::OnSessionMessage( ConnectionId id, std::string payload )
    {
        m_logger->debug( "WS message [{}]: {} bytes", id, payload.size() );

        if ( m_onMessage )
        {
            m_onMessage( { id, std::move( payload ) } );
        }
    }

    // ── Accept loop ────────────────────────────────────────────────────────

    void WsServer::AcceptLoop()
    {
        m_acceptor.async_accept( asio::make_strand( m_ioc ),
                                 [this]( beast::error_code ec, asio::ip::tcp::socket socket )
                                 {
                                     if ( !m_running )
                                     {
                                         return;
                                     }

                                     if ( ec )
                                     {
                                         m_logger->debug( "WS accept: {}", ec.message() );
                                     }
                                     else
                                     {
                                         ConnectionId id = m_nextId.fetch_add( 1 );
                                         std::make_shared<WsSession>( std::move( socket ), id, *this,
                                                                      m_config.m_maxMessageBytes, m_logger )
                                             ->Run();
                                     }

                                     AcceptLoop();
                                 } );
    }

} // namespace iot::network::websocket
