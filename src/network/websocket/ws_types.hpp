/**
 * @file       ws_types.hpp
 * @brief      WebSocket shared types — connection IDs, messages, callbacks
 * @standard   C++23
 */

#ifndef IOT_NETWORK_WEBSOCKET_WS_TYPES_HPP
#define IOT_NETWORK_WEBSOCKET_WS_TYPES_HPP

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace iot::network::websocket
{
    /// Unique identifier for a WebSocket connection.
    using ConnectionId = uint64_t;

    /// Text message received from a client.
    struct WsMessage
    {
        ConnectionId m_connectionId;
        std::string m_payload;
    };

    /// Callbacks supplied by the server consumer.
    using ConnectCallback = std::function<void( ConnectionId )>;
    using DisconnectCallback = std::function<void( ConnectionId )>;
    using MessageCallback = std::function<void( const WsMessage& )>;

} // namespace iot::network::websocket

#endif // IOT_NETWORK_WEBSOCKET_WS_TYPES_HPP
