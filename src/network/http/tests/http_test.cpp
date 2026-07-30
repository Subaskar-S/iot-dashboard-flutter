/**
 * @file       http_test.cpp
 * @brief      Unit tests for HTTP types, router, and server
 * @standard   C++23
 *
 * Tests are split:
 *  - UNIT: HttpRequest/HttpResponse helpers, HttpRouter dispatch logic.
 *    No network I/O — always run.
 *  - INTEGRATION (DISABLED_): full HttpServer with real TCP socket.
 *    Run with: --gtest_also_run_disabled_tests
 */

#include "network/http/http_router.hpp"
#include "network/http/http_server.hpp"
#include "network/http/http_types.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <gtest/gtest.h>
#include <thread>

namespace
{
    using namespace iot::network::http;
    namespace asio = boost::asio;
    namespace beast = boost::beast;
    namespace bhttp = boost::beast::http;

    // -----------------------------------------------------------------------
    // HttpRequest helpers
    // -----------------------------------------------------------------------

    TEST( HttpRequestTest, GetQueryParam )
    {
        HttpRequest req;
        req.m_query = "limit=10&offset=20&status=online";

        EXPECT_EQ( req.GetQueryParam( "limit" ), "10" );
        EXPECT_EQ( req.GetQueryParam( "offset" ), "20" );
        EXPECT_EQ( req.GetQueryParam( "status" ), "online" );
        EXPECT_EQ( req.GetQueryParam( "missing" ), "" );
    }

    TEST( HttpRequestTest, GetHeader )
    {
        HttpRequest req;
        req.m_headers["content-type"] = "application/json";
        req.m_headers["authorization"] = "Bearer token123";

        EXPECT_EQ( req.GetHeader( "content-type" ), "application/json" );
        EXPECT_EQ( req.GetHeader( "Content-Type" ), "application/json" );
        EXPECT_EQ( req.GetHeader( "AUTHORIZATION" ), "Bearer token123" );
        EXPECT_EQ( req.GetHeader( "x-missing" ), "" );
    }

    TEST( HttpRequestTest, EmptyQueryReturnsEmpty )
    {
        HttpRequest req;
        EXPECT_EQ( req.GetQueryParam( "anything" ), "" );
    }

    // -----------------------------------------------------------------------
    // HttpResponse helpers
    // -----------------------------------------------------------------------

    TEST( HttpResponseTest, JsonSetsContentTypeAndStatus )
    {
        HttpResponse res;
        res.Json( 201, R"({"id":"abc"})" );

        EXPECT_EQ( res.m_status, 201 );
        EXPECT_EQ( res.m_body, R"({"id":"abc"})" );
        EXPECT_EQ( res.m_headers.at( "content-type" ), "application/json" );
    }

    TEST( HttpResponseTest, NoContentClearsBody )
    {
        HttpResponse res;
        res.m_body = "something";
        res.NoContent();

        EXPECT_EQ( res.m_status, 204 );
        EXPECT_TRUE( res.m_body.empty() );
    }

    TEST( HttpResponseTest, ErrorSetsJsonBody )
    {
        HttpResponse res;
        res.Error( 404, "NotFound", "Device not found" );

        EXPECT_EQ( res.m_status, 404 );
        EXPECT_NE( res.m_body.find( "NotFound" ), std::string::npos );
        EXPECT_NE( res.m_body.find( "Device not found" ), std::string::npos );
    }

    // -----------------------------------------------------------------------
    // MethodFromString
    // -----------------------------------------------------------------------

    TEST( HttpMethodTest, FromString )
    {
        EXPECT_EQ( MethodFromString( "GET" ), HttpMethod::GET );
        EXPECT_EQ( MethodFromString( "POST" ), HttpMethod::POST );
        EXPECT_EQ( MethodFromString( "PUT" ), HttpMethod::PUT );
        EXPECT_EQ( MethodFromString( "DELETE" ), HttpMethod::DELETE );
        EXPECT_EQ( MethodFromString( "PATCH" ), HttpMethod::PATCH );
        EXPECT_EQ( MethodFromString( "BOGUS" ), HttpMethod::Unknown );
    }

    // -----------------------------------------------------------------------
    // HttpRouter
    // -----------------------------------------------------------------------

    TEST( HttpRouterTest, DispatchMatchingRoute )
    {
        HttpRouter router;
        router.AddRoute( HttpMethod::GET, "/health",
                         []( const HttpRequest&, HttpResponse& res )
                         {
                             res.Json( 200, R"({"status":"ok"})" );
                         } );

        HttpRequest req;
        req.m_method = HttpMethod::GET;
        req.m_path = "/health";

        HttpResponse res;
        router.Dispatch( req, res );

        EXPECT_EQ( res.m_status, 200 );
        EXPECT_NE( res.m_body.find( "ok" ), std::string::npos );
    }

    TEST( HttpRouterTest, DispatchUnknownPathReturns404 )
    {
        HttpRouter router;

        HttpRequest req;
        req.m_method = HttpMethod::GET;
        req.m_path = "/no-such-path";

        HttpResponse res;
        router.Dispatch( req, res );

        EXPECT_EQ( res.m_status, 404 );
    }

    TEST( HttpRouterTest, DispatchWrongMethodReturns405 )
    {
        HttpRouter router;
        router.AddRoute( HttpMethod::GET, "/devices",
                         []( const HttpRequest&, HttpResponse& res )
                         {
                             res.Json( 200, "[]" );
                         } );

        HttpRequest req;
        req.m_method = HttpMethod::DELETE;
        req.m_path = "/devices";

        HttpResponse res;
        router.Dispatch( req, res );

        EXPECT_EQ( res.m_status, 405 );
    }

    TEST( HttpRouterTest, MiddlewareRunsBeforeHandler )
    {
        HttpRouter router;

        bool middlewareCalled = false;
        router.AddMiddleware(
            [&middlewareCalled]( const HttpRequest&, HttpResponse& )
            {
                middlewareCalled = true;
                return true; // continue
            } );

        router.AddRoute( HttpMethod::GET, "/ping",
                         []( const HttpRequest&, HttpResponse& res )
                         {
                             res.Text( 200, "pong" );
                         } );

        HttpRequest req;
        req.m_method = HttpMethod::GET;
        req.m_path = "/ping";
        HttpResponse res;
        router.Dispatch( req, res );

        EXPECT_TRUE( middlewareCalled );
        EXPECT_EQ( res.m_status, 200 );
    }

    TEST( HttpRouterTest, MiddlewareCanShortCircuit )
    {
        HttpRouter router;

        router.AddMiddleware(
            []( const HttpRequest&, HttpResponse& res )
            {
                res.Error( 401, "Unauthorized", "No token" );
                return false; // abort
            } );

        bool handlerCalled = false;
        router.AddRoute( HttpMethod::GET, "/secret",
                         [&handlerCalled]( const HttpRequest&, HttpResponse& )
                         {
                             handlerCalled = true;
                         } );

        HttpRequest req;
        req.m_method = HttpMethod::GET;
        req.m_path = "/secret";
        HttpResponse res;
        router.Dispatch( req, res );

        EXPECT_FALSE( handlerCalled );
        EXPECT_EQ( res.m_status, 401 );
    }

    TEST( HttpRouterTest, MultipleRoutesDispatchCorrectly )
    {
        HttpRouter router;
        router.AddRoute( HttpMethod::GET, "/a",
                         []( const HttpRequest&, HttpResponse& res )
                         {
                             res.Text( 200, "A" );
                         } );
        router.AddRoute( HttpMethod::GET, "/b",
                         []( const HttpRequest&, HttpResponse& res )
                         {
                             res.Text( 200, "B" );
                         } );
        router.AddRoute( HttpMethod::POST, "/a",
                         []( const HttpRequest&, HttpResponse& res )
                         {
                             res.Text( 201, "A-created" );
                         } );

        auto dispatch = [&]( HttpMethod m, std::string path )
        {
            HttpRequest r;
            r.m_method = m;
            r.m_path = std::move( path );
            HttpResponse res;
            router.Dispatch( r, res );
            return res;
        };

        EXPECT_EQ( dispatch( HttpMethod::GET, "/a" ).m_body, "A" );
        EXPECT_EQ( dispatch( HttpMethod::GET, "/b" ).m_body, "B" );
        EXPECT_EQ( dispatch( HttpMethod::POST, "/a" ).m_status, 201 );
        EXPECT_EQ( dispatch( HttpMethod::GET, "/c" ).m_status, 404 );
    }

    // -----------------------------------------------------------------------
    // Integration test — real TCP, HttpServer
    // -----------------------------------------------------------------------

    /// Simple blocking HTTP GET via Beast (used in integration test only).
    static std::pair<unsigned, std::string> SyncGet( uint16_t port, std::string_view path )
    {
        asio::io_context ioc;
        asio::ip::tcp::resolver resolver( ioc );
        beast::tcp_stream stream( ioc );

        auto results = resolver.resolve( "127.0.0.1", std::to_string( port ) );
        stream.connect( results );

        bhttp::request<bhttp::empty_body> req{ bhttp::verb::get, path, 11 };
        req.set( bhttp::field::host, "127.0.0.1" );
        req.set( bhttp::field::user_agent, "iot-test" );
        bhttp::write( stream, req );

        beast::flat_buffer buf;
        bhttp::response<bhttp::string_body> res;
        bhttp::read( stream, buf, res );

        beast::error_code ec;
        stream.socket().shutdown( asio::ip::tcp::socket::shutdown_both, ec );
        return { res.result_int(), res.body() };
    }

    TEST( HttpServerIntegrationTest, DISABLED_HealthEndpoint )
    {
        HttpServerConfig cfg;
        cfg.m_port = 18080;
        cfg.m_threadCount = 2;
        HttpServer server( cfg );

        server.Router().AddRoute( HttpMethod::GET, "/health",
                                  []( const HttpRequest&, HttpResponse& res )
                                  {
                                      res.Json( 200, R"({"status":"ok"})" );
                                  } );

        ASSERT_TRUE( server.Start().has_value() );

        auto [status, body] = SyncGet( 18080, "/health" );
        EXPECT_EQ( status, 200 );
        EXPECT_NE( body.find( "ok" ), std::string::npos );

        server.Stop();
    }

    TEST( HttpServerIntegrationTest, DISABLED_NotFoundReturns404 )
    {
        HttpServerConfig cfg;
        cfg.m_port = 18081;
        cfg.m_threadCount = 2;
        HttpServer server( cfg );

        ASSERT_TRUE( server.Start().has_value() );

        auto [status, body] = SyncGet( 18081, "/no-such-thing" );
        EXPECT_EQ( status, 404 );

        server.Stop();
    }

} // namespace
