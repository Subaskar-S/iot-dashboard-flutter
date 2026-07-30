/**
 * @file       ws_session.cpp
 * @brief      Beast WebSocket session — accept, read, write, close
 * @standard   C++23
 */

#include "network/websocket/ws_session.hpp"
#include "network/websocket/ws_server.hpp"

namespace iot::network::websocket
{
    WsSession::WsSession( asio::ip::tcp::socket socket,
                          ConnectionId id,
                          WsServer& server,
                          size_t maxMessageBytes,
                          std::shared_ptr<spdlog::logger> logger )
        : m_ws( std::move( socket ) )
        , m_id( id )
        , m_server( server )
        , m_maxMessageBytes( maxMessageBytes )
        , m_logger( std::move( logger ) )
    {
        m_ws.set_option( beast::websocket::stream_base::timeout::suggested( beast::role_type::server ) );
        m_ws.set_option( beast::websocket::stream_base::decorator(
            []( beast::websocket::response_type& res )
            {
                res.set( boost::beast::http::field::server, "IoTDashboard/1.0" );
            } ) );

        m_ws.read_message_max( maxMessageBytes );
    }

    void WsSession::Run()
    {
        asio::dispatch( m_ws.get_executor(), beast::bind_front_handler( &WsSession::DoAccept, shared_from_this() ) );
    }

    void WsSession::DoAccept()
    {
        m_ws.async_accept( beast::bind_front_handler( &WsSession::OnAccept, shared_from_this() ) );
    }

    void WsSession::OnAccept( beast::error_code ec )
    {
        if ( ec )
        {
            m_logger->debug( "WS accept error [{}]: {}", m_id, ec.message() );
            return;
        }

        m_server.OnSessionConnected( m_id, shared_from_this() );
        DoRead();
    }

    void WsSession::DoRead()
    {
        m_ws.async_read( m_readBuf, beast::bind_front_handler( &WsSession::OnRead, shared_from_this() ) );
    }

    void WsSession::OnRead( beast::error_code ec, std::size_t /* bytes */ )
    {
        if ( ec == beast::websocket::error::closed )
        {
            m_server.OnSessionDisconnected( m_id );
            return;
        }

        if ( ec )
        {
            m_logger->debug( "WS read error [{}]: {}", m_id, ec.message() );
            m_server.OnSessionDisconnected( m_id );
            return;
        }

        if ( m_ws.got_text() )
        {
            std::string payload = beast::buffers_to_string( m_readBuf.data() );
            m_readBuf.consume( m_readBuf.size() );
            m_server.OnSessionMessage( m_id, std::move( payload ) );
        }
        else
        {
            m_readBuf.consume( m_readBuf.size() ); // discard binary
        }

        DoRead();
    }

    void WsSession::Send( std::string message )
    {
        std::lock_guard lock( m_writeMutex );
        m_writeQueue.push_back( std::move( message ) );
        if ( !m_writing )
        {
            m_writing = true;
            // Schedule DoWrite on the session's strand.
            asio::post( m_ws.get_executor(), beast::bind_front_handler( &WsSession::DoWrite, shared_from_this() ) );
        }
    }

    void WsSession::DoWrite()
    {
        std::string msg;
        {
            std::lock_guard lock( m_writeMutex );
            if ( m_writeQueue.empty() )
            {
                m_writing = false;
                return;
            }
            msg = std::move( m_writeQueue.front() );
            m_writeQueue.pop_front();
        }

        m_ws.text( true );
        m_ws.async_write( asio::buffer( msg ), beast::bind_front_handler( &WsSession::OnWrite, shared_from_this() ) );
    }

    void WsSession::OnWrite( beast::error_code ec, std::size_t /* bytes */ )
    {
        if ( ec )
        {
            m_logger->debug( "WS write error [{}]: {}", m_id, ec.message() );
            m_server.OnSessionDisconnected( m_id );
            return;
        }

        // Check if more messages are queued.
        {
            std::lock_guard lock( m_writeMutex );
            if ( m_writeQueue.empty() )
            {
                m_writing = false;
                return;
            }
        }

        DoWrite();
    }

    void WsSession::Close()
    {
        asio::post( m_ws.get_executor(),
                    [self = shared_from_this()]
                    {
                        beast::error_code ec;
                        self->m_ws.close( beast::websocket::close_code::normal, ec );
                    } );
    }

    void WsSession::OnClose( beast::error_code /* ec */ )
    {
        m_server.OnSessionDisconnected( m_id );
    }

} // namespace iot::network::websocket
