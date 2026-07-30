/**
 * @file       error.hpp
 * @brief      Error codes and result types
 * @standard   C++20
 */

#ifndef IOT_COMMON_ERROR_HPP
#define IOT_COMMON_ERROR_HPP

#include <expected>
#include <string>
#include <string_view>
#include <system_error>

namespace iot
{
    // -----------------------------------------------------------------------
    // Error codes
    // -----------------------------------------------------------------------
    enum class Error
    {
        Success = 0,
        
        // General errors
        InvalidInput = 100,
        InvalidConfig = 101,
        FileNotFound = 102,
        InternalError = 103,
        
        // Device errors
        DeviceNotFound = 200,
        DeviceOffline = 201,
        ConnectionFailed = 202,
        DeviceTimeout = 203,
        
        // Data errors
        DatabaseError = 300,
        DataNotFound = 301,
        DataCorrupted = 302,
        
        // Protocol errors
        ProtocolError = 400,
        MqttError = 401,
        ModbusError = 402,
        OpcuaError = 403,
        
        // Authentication errors
        AuthenticationFailed = 500,
        AuthorizationFailed = 501,
        TokenExpired = 502,
        
        // Network errors
        NetworkError = 600,
        Timeout = 601,
        ConnectionClosed = 602
    };

    // -----------------------------------------------------------------------
    // Error category for std::error_code
    // -----------------------------------------------------------------------
    class ErrorCategory : public std::error_category
    {
        public:
        [[nodiscard]] const char* name() const noexcept override
        {
            return "iot";
        }

        [[nodiscard]] std::string message( int ev ) const override;
    };

    const ErrorCategory& GetErrorCategory();

    // -----------------------------------------------------------------------
    // Make error_code from Error enum
    // -----------------------------------------------------------------------
    inline std::error_code make_error_code( Error e )
    {
        return { static_cast<int>( e ), GetErrorCategory() };
    }

    // -----------------------------------------------------------------------
    // Result type using std::expected (C++23 preview)
    // Fall back to boost::outcome if std::expected unavailable
    // -----------------------------------------------------------------------
    template<typename T>
    using Result = std::expected<T, Error>;

} // namespace iot

// Register Error enum with std::error_code
template<>
struct std::is_error_code_enum<iot::Error> : std::true_type {};

#endif // IOT_COMMON_ERROR_HPP
