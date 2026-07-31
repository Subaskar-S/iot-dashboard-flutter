# Development Guide

## Project Layout

```
iot-dashboard-flutter/
├── src/
│   ├── main.cpp              CLI entry point (Application composition)
│   ├── common/               Error codes, logging, shared types
│   ├── core/                 Domain interfaces (ports) + C++20 concepts
│   ├── database/             SQLite connection, migrations, repositories
│   ├── network/
│   │   ├── http/             Boost.Beast HTTP/1.1 server + router
│   │   ├── websocket/        Boost.Beast WebSocket server + pub/sub
│   │   └── mqtt/             Paho C async MQTT client
│   ├── devices/              DeviceManager + HeartbeatMonitor
│   ├── automation/           ConditionEvaluator + RuleEngine
│   ├── security/             PasswordHasher, JwtHandler, AccessControl
│   └── api/                  Application (composition root), controllers,
│                             middleware
├── flutter/
│   └── lib/
│       ├── core/             Theme, constants, Result<T> sealed class
│       ├── domain/           Entities + repository abstract interfaces
│       ├── data/             Dio API client, WebSocket client, repos
│       └── presentation/     Riverpod providers, pages, widgets
├── cmake/                    CompilationFlags.cmake, functions.cmake
├── docs/                     ARCHITECTURE.md, API.md, DEVELOPMENT.md, DEPLOYMENT.md
├── CMakeLists.txt
└── CLAUDE.md
```

## C++ Coding Standards

**Standard**: C++23 (uses `std::expected` for `Result<T>`, all C++20 features available)

### Naming Conventions

| Element | Convention | Example |
|---------|-----------|---------|
| Classes / Types | PascalCase | `DeviceManager`, `SqliteConnection` |
| Functions / Methods | PascalCase | `Connect()`, `GetById()` |
| Variables | camelCase | `deviceId`, `brokerUrl` |
| Constants (`constexpr`) | `k` + PascalCase | `kMaxConnections` |
| Macros | ALL_CAPS | `IOT_HAS_MQTT` |
| Member variables | `m_` prefix | `m_connected`, `m_logger` |
| Interfaces | `I` prefix | `IDeviceRepository`, `IMqttClient` |
| CMake targets | `iot_` prefix | `iot_common`, `iot_database` |
| Files | snake_case | `device_manager.cpp` |

### Formatting (enforced by `.clang-format`)

- **Indent**: 4 spaces
- **Line length**: 120 characters
- **Braces**: Allman style
- **Parentheses**: spaces inside — `if ( condition )`, `func( arg )`
- **Pointers**: left-aligned — `int* ptr`

### Error Handling

Always use `Result<T>` (`std::expected<T, Error>`). Never throw in production paths.

```cpp
Result<DeviceInfo> DeviceManager::Get( const std::string& id )
{
    auto result = m_repository.GetById( id );
    if ( !result )
    {
        return std::unexpected( result.error() );
    }
    return result.value();
}
```

### Thread Safety

- Protect shared state with `std::mutex` + `std::lock_guard`
- Use `std::atomic` for simple flags (`m_running`, `m_connected`)
- `[[nodiscard]]` on all `Result<T>` return values
- Never block the Boost.Asio io_context thread pool

### Modern C++ Patterns

```cpp
// Concepts
template<typename T>
concept Repository = requires( T repo, Entity e, Id id )
{
    { repo.GetById( id ) } -> std::same_as<Result<Entity>>;
    { repo.Add( e ) }      -> std::same_as<Result<void>>;
};

// Ranges
auto online = devices
    | std::views::filter( []( const auto& d ) { return d.m_isConnected; } );

// Structured bindings + std::expected
if ( auto result = m_repo.GetById( id ); result )
{
    auto& device = result.value();
}
```

## Building

### C++ (macOS/Linux)

```bash
mkdir -p build/OSX/Debug && cd build/OSX/Debug
cmake ../../.. -G Ninja -DCMAKE_BUILD_TYPE=Debug
ninja
ctest --output-on-failure
```

### Build types

| Type | Flags | Use |
|------|-------|-----|
| `Debug` | `-g`, no opt | Development |
| `Release` | `-O3`, LTO | Production |
| `RelWithDebInfo` | `-O2 -g` | Profiling |

### Flutter

```bash
cd flutter
flutter pub get
flutter analyze           # 0 errors required
flutter build macos       # verify build
flutter test              # widget tests
```

## Adding a New C++ Module

1. Create `src/mymodule/` and `src/mymodule/tests/`
2. Write `src/mymodule/CMakeLists.txt`:

```cmake
add_library(iot_mymodule STATIC
    my_component.cpp
)

target_include_directories(iot_mymodule PUBLIC
    $<BUILD_INTERFACE:${PROJECT_ROOT}/src>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(iot_mymodule PUBLIC iot_core spdlog::spdlog)
target_compile_features(iot_mymodule PUBLIC cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
```

3. Add `mymodule` to `IOT_MODULES` in `src/CMakeLists.txt`
4. Write at least one test per public method

## Adding a New HTTP Endpoint

1. Create or extend a controller in `src/api/controllers/`
2. Inject dependencies via constructor
3. Call `router.AddMiddleware(MakeAuthMiddleware(...))` before the route
4. Register via `controller.Register(router)` in `Application::RegisterHttpRoutes()`
5. Update `docs/API.md`

## Adding a New Flutter Page

1. Create `flutter/lib/presentation/pages/<feature>/<feature>_page.dart`
2. Add a Riverpod provider in `flutter/lib/presentation/providers/`
3. Add a route in `flutter/lib/presentation/pages/app_router.dart`
4. Add a NavigationRail destination in `flutter/lib/presentation/widgets/common/main_scaffold.dart`

## Testing

### C++ test structure

```cpp
// src/mymodule/tests/my_test.cpp
#include "mymodule/my_component.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

class MyComponentTest : public ::testing::Test
{
    protected:
    void SetUp() override { /* arrange */ }
};

TEST_F( MyComponentTest, DoSomethingReturnsExpected )
{
    // Act
    auto result = m_component.DoSomething( "input" );

    // Assert
    ASSERT_TRUE( result.has_value() );
    EXPECT_EQ( result.value(), "expected" );
}
```

### Integration tests (real services)

Tests that need a live broker or port use `SKIP_IF_NO_BROKER()` / `SKIP_IF_PORT_BUSY()` guards — they run when the service is available and skip gracefully in CI without a broker.

### Coverage targets

- New modules: ≥ 90% line coverage
- Critical paths (auth, security): 100%

## Code Quality Gates (pre-commit)

```bash
# 1. Format
find src -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# 2. Verify format (CI uses --dry-run -Werror)
find src -name "*.cpp" -o -name "*.hpp" | xargs clang-format --dry-run -Werror

# 3. Build
cd build/OSX/Debug && ninja

# 4. Tests
ctest --output-on-failure

# 5. Flutter
cd flutter && flutter analyze && flutter test
```

## Dependency Management

### C++ dependencies (installed via brew / apt)

| Library | brew formula | apt package |
|---------|-------------|-------------|
| Boost | `boost` | `libboost-all-dev` |
| nlohmann/json | `nlohmann-json` | `nlohmann-json3-dev` |
| spdlog | `spdlog` | `libspdlog-dev` |
| fmt | `fmt` | `libfmt-dev` |
| OpenSSL | `openssl` | `libssl-dev` |
| SQLite3 | `sqlite3` | `libsqlite3-dev` |
| Paho MQTT C | `libpaho-mqtt` | `libpaho-mqtt-dev` |
| GoogleTest | `googletest` | `libgtest-dev` |

### Flutter dependencies (pubspec.yaml)

Key packages: `flutter_riverpod`, `go_router`, `dio`, `web_socket_channel`, `fl_chart`, `flutter_secure_storage`

## PR Guidelines

- Max 300 lines changed per PR
- Branch from `develop`, merge into `develop`
- Title format: `feat:`, `fix:`, `docs:`, `test:`, `ci:`
- All tests must pass, `flutter analyze` must be clean
- Include a description table (files changed, what was tested)
