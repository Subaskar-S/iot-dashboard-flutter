# Development Guide

## Project Layout

```
├── src/                    # C++20 source code
│   ├── main.cpp            # CLI entry point
│   ├── api/                # HTTP/WebSocket API server
│   ├── common/             # Shared types, errors, logging
│   ├── device/             # Device drivers (MQTT, Modbus, OPC UA)
│   ├── data/               # Time-series data storage
│   ├── analytics/          # Analytics and alerting
│   └── security/           # Auth, TLS, encryption
├── flutter/                # Flutter frontend
│   ├── lib/                # Dart source code
│   ├── assets/             # Images, fonts, configs
│   └── test/               # Flutter widget tests
├── test/                   # C++ tests (mirrors src/ structure)
├── build/                  # Platform-specific build dirs
├── cmake/                  # CMake configuration
└── docs/                   # Documentation
```

## Code Style

Based on Microsoft style with C++20 enhancements (see `.clang-format`):

- **Standard:** C++20 (concepts, ranges, coroutines, modules)
- **Indent:** 4 spaces
- **Line length:** 120 characters max
- **Naming:**
  - Classes/Types: `PascalCase`
  - Functions/Methods: `PascalCase`
  - Variables: `camelCase`
  - Constants: `kCamelCase` for `constexpr`, `ALL_CAPS` for macros
  - Member variables: `m_` prefix (e.g., `m_deviceId`, `m_isConnected`)
  - Private members: `m_` prefix
- **Braces:** Allman style (each on own line)
- **Parentheses:** Space after `(` and before `)`: `if ( condition )`

### C++20 Features to Use

- **Concepts:** For template constraints
- **Ranges:** Prefer `std::ranges::` algorithms
- **Coroutines:** For async I/O operations
- **Modules:** When compiler support stabilizes
- **std::format:** Instead of printf/iostreams
- **std::span:** For safe array views
- **designated initializers:** For struct initialization

### Example

```cpp
#include <concepts>
#include <ranges>
#include <format>

namespace iot::device
{
    // Concept for device types
    template<typename T>
    concept Device = requires( T device )
    {
        { device.GetId() } -> std::convertible_to<std::string>;
        { device.Connect() } -> std::same_as<bool>;
        { device.Disconnect() } -> std::same_as<void>;
    };

    // Device manager using concepts and ranges
    class DeviceManager
    {
        public:
        template<Device D>
        void RegisterDevice( D&& device )
        {
            m_devices.push_back( std::forward<D>( device ) );
        }

        auto GetConnectedDevices() const
        {
            return m_devices 
                | std::views::filter( []( const auto& dev ) { return dev.IsConnected(); } )
                | std::views::transform( []( const auto& dev ) { return dev.GetId(); } );
        }

        private:
        std::vector<std::unique_ptr<IDevice>> m_devices;
    };
}
```

## Building

```bash
cd build/<Platform>/<BuildType>
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=<BuildType>
ninja
```

### Platform-Specific Flags

**macOS:**
```bash
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_CXX_STANDARD=20
```

**Linux:**
```bash
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=g++-11 \
    -DCMAKE_CXX_STANDARD=20
```

**Windows (MSVC):**
```bash
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=20
```

## Module Structure

Each module under `src/` is a static library (`iot_<module>`):

```
src/<module>/
├── CMakeLists.txt          # Build definition
├── <component>.hpp         # Public header
├── <component>.cpp         # Implementation
└── test/                   # Module-specific tests
```

## Adding a New Module

1. Create `src/<module>/` directory
2. Add `CMakeLists.txt`:
   ```cmake
   add_library(iot_<module> STATIC
       <component>.cpp
   )
   
   target_include_directories(iot_<module> PUBLIC
       $<BUILD_INTERFACE:${PROJECT_ROOT}/src>
       $<INSTALL_INTERFACE:include>
   )
   
   target_link_libraries(iot_<module> PUBLIC iot_common)
   target_compile_features(iot_<module> PUBLIC cxx_std_20)
   ```
3. Add `add_subdirectory(src/<module>)` to `src/CMakeLists.txt`
4. Link from `iot_api` in `src/api/CMakeLists.txt`

## Error Handling

Uses `std::expected<T, Error>` for error propagation (C++23 preview via outcome):

```cpp
#include <expected>
#include "common/error.hpp"

std::expected<DeviceInfo, Error> GetDeviceInfo( const std::string& deviceId )
{
    if ( deviceId.empty() )
    {
        return std::unexpected( Error::InvalidInput );
    }
    
    DeviceInfo info = { /* ... */ };
    return info;
}

// Caller
auto result = GetDeviceInfo( "device-123" );
if ( result )
{
    // Success: use result.value()
}
else
{
    // Error: check result.error()
}
```

## Logging

All diagnostic output uses spdlog with structured logging:

```cpp
#include "common/logging.hpp"

auto logger = iot::CreateLogger( "DeviceManager" );

logger->info( "Device connected: id={} type={}", deviceId, deviceType );
logger->debug( "Sensor reading: value={:.2f} unit={}", value, unit );
logger->error( "Connection failed: device={} error={}", deviceId, errorMsg );
```

## Testing

Tests use Google Test in `test/`:

```bash
cd build/OSX/Debug
ninja test
```

### Writing Tests

```cpp
#include <gtest/gtest.h>
#include "device/mqtt_client.hpp"

TEST( MqttClientTest, ConnectsToBroker )
{
    iot::device::MqttClient client{ "mqtt://localhost:1883" };
    
    auto result = client.Connect();
    
    EXPECT_TRUE( result );
    EXPECT_TRUE( client.IsConnected() );
}

TEST( MqttClientTest, PublishesMessage )
{
    iot::device::MqttClient client{ "mqtt://localhost:1883" };
    client.Connect();
    
    auto result = client.Publish( "sensors/temp", "23.5" );
    
    EXPECT_TRUE( result );
}
```

## Dependencies

Key dependencies:
- **Boost.Asio** — Async I/O
- **Boost.Beast** — HTTP/WebSocket server
- **nlohmann/json** — JSON parsing
- **spdlog** — Logging
- **SQLite** — Embedded database
- **Paho MQTT** — MQTT protocol
- **OpenSSL** — TLS/encryption

## Pull Request Guidelines

- Max 300 lines changed per PR
- Functions max ~100 lines
- Use C++20 features appropriately
- No deep nesting (>3 levels)
- Single exit point per function
- Run tests, linter, and formatter before committing
- Update documentation for API changes

## Code Review Checklist

- [ ] Follows naming conventions
- [ ] Uses C++20 concepts where appropriate
- [ ] Error handling via `std::expected`
- [ ] Proper RAII and move semantics
- [ ] No raw pointers (use smart pointers)
- [ ] Thread-safe where applicable
- [ ] Tests added/updated
- [ ] Documentation updated
