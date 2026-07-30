/**
 * @file       http_server.cpp
 * @brief      Boost.Beast async HTTP server — accept loop + thread pool
 * @standard   C++23
 */

#include "network/http/http_server.hpp"
#include "common/logging.hpp"
#include "network/http/http_session.hpp"
#include <boost/asio.hpp>

namespace iot::network::http
{
    namespace asio = boost::asio;
    namespace beast = boost::beast;

    HttpServer::HttpServer( HttpServerConfig config )
        : m_config( std::move( config ) )
        , m_ioc( static_cast<int>( m_config.m_threadCount ) )
        , m_acceptor( m_ioc )
        , m_logger( CreateLogger( "HttpServer" ) )
    {
    }

    HttpServer::~HttpServer()
    {
        Stop();
    }

    Result<void> HttpServer::Start()
    {
        beast::error_code ec;

        asio::ip::tcp::endpoint endpoint{ asio::ip::make_address( m_config.m_host ), m_config.m_port };

        m_acceptor.open( endpoint.protocol(), ec );
        if ( ec )
        {
            m_logger->error( "Acceptor open: {}", ec.message() );
            return std::unexpected( Error::NetworkError );
        }

        m_acceptor.set_option( asio::socket_base::reuse_address( true ), ec );
        m_acceptor.bind( endpoint, ec );
        if ( ec )
        {
            m_logger->error( "Acceptor bind {}:{}: {}", m_config.m_host, m_config.m_port, ec.message() );
            return std::unexpected( Error::NetworkError );
        }

        m_acceptor.listen( asio::socket_base::max_listen_connections, ec );
        if ( ec )
        {
            m_logger->error( "Acceptor listen: {}", ec.message() );
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

        m_logger->info( "HTTP server listening on {}:{} ({} threads)", m_config.m_host, m_config.m_port,
                        m_config.m_threadCount );
        return {};
    }

    void HttpServer::Stop()
    {
        if ( !m_running.exchange( false ) )
        {
            return;
        }

        beast::error_code ec;
        m_acceptor.close( ec );
        m_ioc.stop();

        for ( auto& t : m_threads )
        {
            if ( t.joinable() )
            {
                t.join();
            }
        }
        m_threads.clear();
        m_logger->info( "HTTP server stopped" );
    }

    bool HttpServer::IsRunning() const noexcept
    {
        return m_running.load();
    }

    void HttpServer::AcceptLoop()
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
                                         m_logger->debug( "Accept error: {}", ec.message() );
                                     }
                                     else
                                     {
                                         std::make_shared<HttpSession>( std::move( socket ), m_router,
                                                                        m_config.m_enableCors, m_logger )
                                             ->Run();
                                     }

                                     AcceptLoop(); // Queue next accept.
                                 } );
    }

} // namespace iot::network::http
