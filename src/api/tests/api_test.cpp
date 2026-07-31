/**
 * @file       api_test.cpp
 * @brief      Unit tests for API layer (middleware, controllers, application)
 * @standard   C++23
 *
 * Unit tests use the router + controllers directly without network I/O.
 * Integration tests (SKIP_IF_PORT_BUSY) start a real Application.
 */

#include "api/application.hpp"
#include "api/middleware/auth_middleware.hpp"
#include "network/http/http_router.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <gtest/gtest.h>
#include <thread>

namespace
{
    using namespace iot;
    using namespace iot::api;
    using namespace iot::network::http;
    namespace asio = boost::asio;
    namespace beast = boost::beast;
    namespace bhttp = boost::beast::http;

    // ── Helpers ───────────────────────────────────────────────────────────

    static std::unique_ptr<security::AuthenticationService> MakeAuthService()
    {
        security::SecurityConfig cfg;
        cfg.m_jwtSecret = "test-secret";
        cfg.m_pbkdf2Iterations = 1000;
        auto svc = std::make_unique<security::AuthenticationService>( cfg );
        std::ignore = svc->RegisterUser( "u-001", "admin", "password", core::Role::Admin );
        return svc;
    }

    static std::string LoginToken( security::AuthenticationService& svc )
    {
        auto result = svc.Login( "admin", "password" );
        return result.has_value() ? result->m_accessToken : "";
    }

    static std::pair<uint16_t, std::string> SyncGet( uint16_t port, std::string_view path, std::string_view token = "" )
    {
        asio::io_context ioc;
        asio::ip::tcp::resolver resolver( ioc );
        beast::tcp_stream stream( ioc );
        auto results = resolver.resolve( "127.0.0.1", std::to_string( port ) );
        stream.connect( results );

        bhttp::request<bhttp::empty_body> req{ bhttp::verb::get, path, 11 };
        req.set( bhttp::field::host, "127.0.0.1" );
        if ( !token.empty() )
        {
            req.set( bhttp::field::authorization, std::string( "Bearer " ) + std::string( token ) );
        }

        bhttp::write( stream, req );

        beast::flat_buffer buf;
        bhttp::response<bhttp::string_body> res;
        bhttp::read( stream, buf, res );

        beast::error_code ec;
        stream.socket().shutdown( asio::ip::tcp::socket::shutdown_both, ec );
        return { res.result_int(), res.body() };
    }

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

    // ─────────────────────────────────────────────────────────────────────
    // Auth middleware unit tests
    // ─────────────────────────────────────────────────────────────────────

    TEST( AuthMiddlewareTest, MissingTokenReturns401 )
    {
        auto svc = MakeAuthService();
        auto mw = MakeTokenMiddleware( *svc );

        HttpRequest req;
        HttpResponse res;
        EXPECT_FALSE( mw( req, res ) );
        EXPECT_EQ( res.m_status, 401 );
    }

    TEST( AuthMiddlewareTest, InvalidTokenReturns401 )
    {
        auto svc = MakeAuthService();
        auto mw = MakeTokenMiddleware( *svc );

        HttpRequest req;
        req.m_headers["authorization"] = "Bearer not.a.token";
        HttpResponse res;
        EXPECT_FALSE( mw( req, res ) );
        EXPECT_EQ( res.m_status, 401 );
    }

    TEST( AuthMiddlewareTest, ValidTokenAllowsThrough )
    {
        auto svc = MakeAuthService();
        std::string token = LoginToken( *svc );
        ASSERT_FALSE( token.empty() );

        auto mw = MakeTokenMiddleware( *svc );
        HttpRequest req;
        req.m_headers["authorization"] = "Bearer " + token;
        HttpResponse res;
        EXPECT_TRUE( mw( req, res ) );
    }

    TEST( AuthMiddlewareTest, InsufficientPermissionsReturns403 )
    {
        auto svc = MakeAuthService();
        std::ignore = svc->RegisterUser( "v-001", "viewer", "pass", core::Role::Viewer );
        auto viewerToken = svc->Login( "viewer", "pass" )->m_accessToken;

        auto mw = MakeAuthMiddleware( *svc, "devices", security::Permission::Write );

        HttpRequest req;
        req.m_headers["authorization"] = "Bearer " + viewerToken;
        HttpResponse res;
        EXPECT_FALSE( mw( req, res ) );
        EXPECT_EQ( res.m_status, 403 );
    }

    TEST( AuthMiddlewareTest, AdminCanAccessAnyResource )
    {
        auto svc = MakeAuthService();
        std::string token = LoginToken( *svc );

        auto mw = MakeAuthMiddleware( *svc, "users", security::Permission::Admin );
        HttpRequest req;
        req.m_headers["authorization"] = "Bearer " + token;
        HttpResponse res;
        EXPECT_TRUE( mw( req, res ) );
    }

    // ─────────────────────────────────────────────────────────────────────
    // Health controller unit test
    // ─────────────────────────────────────────────────────────────────────

    TEST( HealthControllerTest, HealthRouteReturns200 )
    {
        HttpRouter router;
        HealthController health( "1.0.0" );
        health.Register( router );

        HttpRequest req;
        req.m_method = HttpMethod::GET;
        req.m_path = "/health";
        HttpResponse res;
        router.Dispatch( req, res );

        EXPECT_EQ( res.m_status, 200 );
        EXPECT_NE( res.m_body.find( "healthy" ), std::string::npos );
        EXPECT_NE( res.m_body.find( "1.0.0" ), std::string::npos );
    }

    // ─────────────────────────────────────────────────────────────────────
    // Auth controller unit tests
    // ─────────────────────────────────────────────────────────────────────

    TEST( AuthControllerTest, LoginSuccessReturnsTokens )
    {
        auto svc = MakeAuthService();
        HttpRouter router;
        AuthController auth( *svc );
        auth.Register( router );

        HttpRequest req;
        req.m_method = HttpMethod::POST;
        req.m_path = "/auth/login";
        req.m_body = R"({"username":"admin","password":"password"})";
        HttpResponse res;
        router.Dispatch( req, res );

        EXPECT_EQ( res.m_status, 200 );
        EXPECT_NE( res.m_body.find( "access_token" ), std::string::npos );
    }

    TEST( AuthControllerTest, LoginBadCredentialsReturns401 )
    {
        auto svc = MakeAuthService();
        HttpRouter router;
        AuthController auth( *svc );
        auth.Register( router );

        HttpRequest req;
        req.m_method = HttpMethod::POST;
        req.m_path = "/auth/login";
        req.m_body = R"({"username":"admin","password":"wrong"})";
        HttpResponse res;
        router.Dispatch( req, res );

        EXPECT_EQ( res.m_status, 401 );
    }

    TEST( AuthControllerTest, LoginMissingFieldReturns400 )
    {
        auto svc = MakeAuthService();
        HttpRouter router;
        AuthController auth( *svc );
        auth.Register( router );

        HttpRequest req;
        req.m_method = HttpMethod::POST;
        req.m_path = "/auth/login";
        req.m_body = R"({"username":"admin"})";
        HttpResponse res;
        router.Dispatch( req, res );

        EXPECT_EQ( res.m_status, 400 );
    }

    // ─────────────────────────────────────────────────────────────────────
    // Application integration test
    // ─────────────────────────────────────────────────────────────────────

    TEST( ApplicationTest, HealthEndpointNoAuth )
    {
        SKIP_IF_PORT_BUSY( 17080 );

        AppConfig cfg;
        cfg.m_httpPort = 17080;
        cfg.m_wsPort = 17081;
        cfg.m_mqttBroker = "tcp://127.0.0.1:1883";
        cfg.m_dbPath = ":memory:";
        cfg.m_pbkdf2Iterations = 1000;
        cfg.m_jwtSecret = "test";

        Application app( cfg );
        ASSERT_TRUE( app.Initialize().has_value() );
        ASSERT_TRUE( app.Start().has_value() );

        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

        auto [status, body] = SyncGet( 17080, "/health" );
        EXPECT_EQ( status, 200 );
        EXPECT_NE( body.find( "healthy" ), std::string::npos );

        app.Stop();
    }

    TEST( ApplicationTest, DevicesRequiresAuth )
    {
        SKIP_IF_PORT_BUSY( 17082 );

        AppConfig cfg;
        cfg.m_httpPort = 17082;
        cfg.m_wsPort = 17083;
        cfg.m_mqttBroker = "tcp://127.0.0.1:1883";
        cfg.m_dbPath = ":memory:";
        cfg.m_pbkdf2Iterations = 1000;
        cfg.m_jwtSecret = "test";

        Application app( cfg );
        ASSERT_TRUE( app.Initialize().has_value() );
        ASSERT_TRUE( app.Start().has_value() );

        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

        // No token → 401
        auto [status, body] = SyncGet( 17082, "/devices" );
        EXPECT_EQ( status, 401 );

        app.Stop();
    }

} // namespace
