# IoT Dashboard Project Guide

A production-grade Industrial IoT Dashboard built with C++20 and Flutter. This guide covers the current project setup and how to extend it as development proceeds module by module.

## What's Included

### 1. Code Style & Formatting
- **`.clang-format`** — Complete formatting configuration
  - Based on Microsoft style with Allman braces
  - 120-character line limit
  - Space-balanced parentheses: `if ( condition )`
  - C++20-aware formatting (concepts, requires clauses, coroutines)

### 2. CMake Build System
- **`CMakeLists.txt`** — Root project configuration
- **`cmake/functions.cmake`** — Helper functions
- **`cmake/CompilationFlags.cmake`** — Compiler warnings and optimizations
- Platform-specific builds (OSX, Linux, Windows)
- Ninja build system support
- C++20 standard enforcement

### 3. Current Project Structure
```
iot-dashboard-flutter/
├── .clang-format              # Code formatting rules
├── CLAUDE.md                  # AI-assisted development rules
├── CMakeLists.txt             # Root CMake config
├── README.md                  # Project overview
├── PROJECT_TEMPLATE_GUIDE.md  # This file
├── QUICK_START.md             # Quick reference
├── docs/
│   ├── ARCHITECTURE.md        # System design and module breakdown
│   ├── API.md                 # REST/WebSocket/MQTT API reference
│   └── DEVELOPMENT.md         # Developer guide
├── cmake/
│   ├── functions.cmake        # CMake helpers
│   └── CompilationFlags.cmake # Compiler flags
└── src/
    ├── CMakeLists.txt         # Source tree build
    ├── main.cpp               # CLI entry point
    └── common/
        ├── CMakeLists.txt     # Module build
        ├── types.hpp          # C++20 types with concepts
        ├── error.hpp          # std::expected error handling
        └── logging.hpp        # spdlog integration
```

As development proceeds, new top-level modules will be added under `src/` (`core/`, `network/`, `database/`, `devices/`, `automation/`, `security/`, `api/`, `metrics/`) and a `flutter/` directory for the frontend. See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the full module breakdown.

### 4. C++20 Best Practices

**Concepts:**
```cpp
template<typename T>
concept Device = requires( T device )
{
    { device.GetId() } -> std::convertible_to<std::string>;
    { device.Connect() } -> std::same_as<bool>;
};
```

**Error Handling:**
```cpp
Result<DeviceInfo> GetDeviceInfo( const std::string& deviceId )
{
    if ( deviceId.empty() )
        return std::unexpected( Error::InvalidInput );
    return DeviceInfo{ /* ... */ };
}
```

**Structured Logging:**
```cpp
auto logger = CreateLogger( "MyModule" );
logger->info( "Device connected: id={}", deviceId );
```

## Getting Started

### Step 1: Configure Dependencies

`CMakeLists.txt` declares the required and optional dependencies:
```cmake
# Required
find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)

# Optional
option(ENABLE_MQTT "Enable MQTT support" ON)
option(ENABLE_MODBUS "Enable Modbus support" ON)
```

Install platform dependencies (macOS example):
```bash
brew install cmake ninja boost nlohmann-json spdlog fmt openssl sqlite3
```

### Step 2: Build

```bash
# Create build directory
mkdir -p build/OSX/Debug
cd build/OSX/Debug

# Configure
cmake ../.. -G "Ninja" -DCMAKE_BUILD_TYPE=Debug

# Build
ninja

# Test (if BUILD_TESTING=ON)
ninja test
```

### Step 3: Follow the Incremental Module Workflow

Per [CLAUDE.md](CLAUDE.md), modules are implemented one at a time: design → approve → implement → test → static analysis → document → next module. See the roadmap in [README.md](README.md) for module order.

## Naming Conventions

### Classes and Types
```cpp
class DeviceManager { /* ... */ };      // PascalCase
struct SensorReading { /* ... */ };     // PascalCase
enum class AlertSeverity { /* ... */ }; // PascalCase
```

### Functions and Methods
```cpp
bool Connect();                         // PascalCase
void ProcessData( const Data& data );   // PascalCase
```

### Variables
```cpp
int deviceCount;                        // camelCase
std::string brokerUrl;                  // camelCase
```

### Constants
```cpp
constexpr int kMaxConnections = 1000;   // kCamelCase
#define MAX_BUFFER_SIZE 4096            // ALL_CAPS (macros)
```

### Member Variables
```cpp
class Device
{
    private:
    std::string m_id;                   // m_ prefix
    bool m_isConnected;                 // m_ prefix
    uint16_t m_port;                    // m_ prefix
};
```

## Code Style Rules

### Braces (Allman Style)
```cpp
// Correct
if ( condition )
{
    DoSomething();
}

// Wrong
if ( condition ) {
    DoSomething();
}
```

### Spacing in Parentheses
```cpp
// Correct
if ( x > 0 && y < 100 )
for ( auto& item : items )
void Process( int value, const std::string& name )

// Wrong
if (x>0&&y<100)
for (auto& item : items)
void Process(int value,const std::string& name)
```

### Pointer/Reference Alignment
```cpp
// Correct
int* ptr = &value;
const std::string& name = getName();

// Wrong
int *ptr = &value;
const std::string &name = getName();
```

## CMake Module Pattern

When adding a new module:

```cmake
# src/mymodule/CMakeLists.txt
add_library(iot_mymodule STATIC
    my_component.cpp
    my_helper.cpp
)

target_include_directories(iot_mymodule PUBLIC
    $<BUILD_INTERFACE:${PROJECT_ROOT}/src>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(iot_mymodule PUBLIC
    iot_common
    Boost::boost
    spdlog::spdlog
)

target_compile_features(iot_mymodule PUBLIC cxx_std_20)

install(TARGETS iot_mymodule
    EXPORT IoTDashboardTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
```

## Error Handling Pattern

```cpp
#include "common/error.hpp"

Result<int> ProcessInput( const std::string& input )
{
    if ( input.empty() )
        return std::unexpected( Error::InvalidInput );
    
    int result = /* parse input */;
    return result;
}

// Caller
auto result = ProcessInput( data );
if ( result )
{
    // Success
    int value = result.value();
}
else
{
    // Error
    auto logger = CreateLogger( "Main" );
    logger->error( "Input processing failed: {}", 
                   static_cast<int>( result.error() ) );
}
```

## Logging Pattern

```cpp
#include "common/logging.hpp"

class MyComponent
{
    public:
    MyComponent()
        : m_logger( CreateLogger( "MyComponent" ) )
    {
        m_logger->info( "Component initialized" );
    }

    void Process( const Data& data )
    {
        m_logger->debug( "Processing data: size={}", data.size() );
        
        if ( !Validate( data ) )
        {
            m_logger->error( "Validation failed" );
            return;
        }
        
        m_logger->info( "Processing complete: result={}", result );
    }

    private:
    std::shared_ptr<spdlog::logger> m_logger;
};
```

## Testing Pattern

```cpp
#include <gtest/gtest.h>
#include "device/mqtt_client.hpp"

class MqttClientTest : public ::testing::Test
{
    protected:
    void SetUp() override
    {
        m_client = std::make_unique<MqttClient>( "mqtt://localhost:1883" );
    }

    void TearDown() override
    {
        if ( m_client->IsConnected() )
            m_client->Disconnect();
    }

    std::unique_ptr<MqttClient> m_client;
};

TEST_F( MqttClientTest, ConnectsToBroker )
{
    auto result = m_client->Connect();
    
    EXPECT_TRUE( result );
    EXPECT_TRUE( m_client->IsConnected() );
}

TEST_F( MqttClientTest, PublishesMessage )
{
    m_client->Connect();
    
    auto result = m_client->Publish( "test/topic", "Hello, IoT!" );
    
    EXPECT_TRUE( result );
}
```

## Configuration Files

### Core Files
- `.clang-format` — Complete formatting config
- `cmake/functions.cmake` — CMake helper functions
- `cmake/CompilationFlags.cmake` — Warning flags for C++20

### Project Structure
- `CMakeLists.txt` — Root build configuration
- `docs/DEVELOPMENT.md` — Development guidance
- `src/common/*` — Shared types and error handling

## Next Steps

1. **Core Module**: Domain types, interfaces, and concepts in `src/core/`
2. **Network Modules**: HTTP (`src/network/http/`), WebSocket (`src/network/websocket/`), MQTT (`src/network/mqtt/`)
3. **Database Module**: SQLite repositories in `src/database/`
4. **Device Manager**: `src/devices/` with heartbeat monitoring
5. **Automation Engine**: Rule engine in `src/automation/`
6. **Security Module**: JWT auth and RBAC in `src/security/`
7. **API Composition**: Wire everything together in `src/api/`
8. **Flutter Frontend**: Create `flutter/` directory with Riverpod + clean architecture
9. **Write Tests**: Unit and integration tests per module, in each module's `tests/` directory
10. **Configure CI/CD**: GitHub Actions for build, test, lint, sanitizers

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for detailed module responsibilities and [docs/API.md](docs/API.md) for the target API surface.

## References

- **Code Style**: Microsoft C++ style with Allman braces
- **Build System**: CMake 3.20+ with Ninja
- **Testing**: Google Test
- **Logging**: spdlog

---

**Project Version**: 1.0.0 (in development)  
**Last Updated**: July 30, 2026
