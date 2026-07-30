/**
 * @file       error.cpp
 * @brief      Error category implementation
 * @standard   C++20
 */

#include "common/error.hpp"

namespace iot
{
    std::string ErrorCategory::message( int ev ) const
    {
        switch ( static_cast<Error>( ev ) )
        {
            case Error::Success:             return "success";

            case Error::InvalidInput:        return "invalid input";
            case Error::InvalidConfig:       return "invalid configuration";
            case Error::FileNotFound:        return "file not found";
            case Error::InternalError:       return "internal error";

            case Error::DeviceNotFound:      return "device not found";
            case Error::DeviceOffline:       return "device offline";
            case Error::ConnectionFailed:    return "connection failed";
            case Error::DeviceTimeout:       return "device timeout";

            case Error::DatabaseError:       return "database error";
            case Error::DataNotFound:        return "data not found";
            case Error::DataCorrupted:       return "data corrupted";

            case Error::ProtocolError:       return "protocol error";
            case Error::MqttError:           return "MQTT error";
            case Error::ModbusError:         return "Modbus error";
            case Error::OpcuaError:          return "OPC UA error";

            case Error::AuthenticationFailed: return "authentication failed";
            case Error::AuthorizationFailed:  return "authorization failed";
            case Error::TokenExpired:         return "token expired";

            case Error::NetworkError:        return "network error";
            case Error::Timeout:             return "timeout";
            case Error::ConnectionClosed:     return "connection closed";
        }

        return "unknown error";
    }

    const ErrorCategory& GetErrorCategory()
    {
        static const ErrorCategory instance;
        return instance;
    }

} // namespace iot
