# Production-Grade Industrial IoT Dashboard (C++20 + Flutter)

A production-quality Industrial IoT monitoring and control dashboard built with modern C++20 backend and Flutter frontend, designed to professional engineering standards suitable for deployment in real-world industrial environments.

## Architecture Overview

```
Flutter Desktop/Mobile
        ↓
REST API + WebSocket
        ↓
C++20 Backend Server
        ↓
MQTT Broker
        ↓
ESP32 / Raspberry Pi / IoT Devices
```

## Engineering Standards

**Core Principles:**
- Clean Architecture with SOLID principles
- Dependency Injection
- Repository Pattern
- Thread-safe, exception-safe code
- Zero raw owning pointers (RAII + smart pointers)
- Strong type safety with C++20 features

**C++20 Features:**
- Concepts for type constraints
- Ranges and views
- `std::expected` for error handling
- `std::span` and `std::string_view`
- Coroutines for async operations
- `constexpr` and compile-time evaluation
- Move semantics throughout

## Key Features

### Backend
- **HTTP REST API**: Device management, sensor queries, command execution
- **WebSocket**: Real-time bidirectional communication with live updates
- **MQTT Integration**: Subscribe/publish with QoS, retained messages, auto-reconnect
- **Device Management**: Track status, firmware, battery, capabilities, heartbeat
- **Authentication**: JWT tokens, role-based access, password hashing, refresh tokens
- **Automation Engine**: Rule-based automation (IF sensor > threshold THEN action)
- **Database**: SQLite with repository pattern, migrations, versioning
- **Logging**: Structured logging with spdlog, rotation, levels
- **Metrics**: System monitoring, latency tracking, resource usage
- **Configuration**: JSON-based with hot reload support

### Frontend (Flutter)
- **Dashboard**: Real-time charts and live sensor visualization
- **Device Management**: Add, configure, monitor, and control devices
- **Automation Editor**: Visual rule builder for IF-THEN automation
- **Alerts**: Configurable thresholds and notifications
- **User Management**: Role-based access control
- **Settings**: System configuration and preferences
- **Firmware Updates**: OTA update management
- **System Logs**: Searchable audit trail
- **Dark Mode**: Adaptive theming
- **Multi-Platform**: Windows, macOS, Linux, iOS, Android

## Module Architecture

The backend is organized into independent, testable C++20 modules:

| Module | Purpose |
|--------|---------|
| `core` | Core types, interfaces, abstractions |
| `network/http` | HTTP server with REST endpoints |
| `network/websocket` | WebSocket server for real-time communication |
| `network/mqtt` | MQTT client with pub/sub abstraction |
| `database` | SQLite repository pattern implementation |
| `devices` | Device registry, tracking, and management |
| `automation` | Rule engine for automated responses |
| `security` | JWT auth, password hashing, access control |
| `logging` | Structured logging infrastructure |
| `config` | Configuration management and hot reload |
| `api` | API composition root and routing |
| `metrics` | System metrics and monitoring |
| `storage` | Time-series data storage and retrieval |
| `utils` | Shared utilities and helpers |

## Technology Stack

### Backend
- **Language**: C++20
- **Build System**: CMake 3.20+ with Ninja
- **Networking**: Boost.Asio, Boost.Beast / websocketpp
- **MQTT**: Eclipse Paho MQTT C++
- **Database**: SQLite3 with modern C++ wrapper
- **JSON**: nlohmann/json
- **Logging**: spdlog with fmt
- **Testing**: GoogleTest + GMock
- **Security**: OpenSSL for TLS/JWT
- **Standards**: RAII, Smart Pointers, Dependency Injection, Clean Architecture

### Frontend
- **Framework**: Flutter 3.16+
- **State Management**: Riverpod
- **HTTP Client**: Dio
- **WebSocket**: web_socket_channel
- **Charts**: fl_chart
- **Navigation**: go_router
- **Architecture**: Clean Architecture (presentation/domain/data layers)

## Quick Start

```bash
# Build Backend
mkdir -p build/$(uname)/Debug
cd build/$(uname)/Debug
cmake ../.. -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
ninja

# Run Backend
./iot-dashboard --config config.json --serve

# Run Flutter (separate terminal)
cd flutter
flutter run -d macos  # or windows, linux, ios, android
```

## Backend API

### HTTP REST Endpoints

```
GET    /health              Health check
GET    /devices             List all devices
POST   /devices             Register new device
DELETE /devices/{id}        Remove device
GET    /sensors             Query sensor data
GET    /sensors/history     Historical data
POST   /commands            Send device command
GET    /metrics             System metrics
POST   /auth/login          Authenticate user
POST   /auth/refresh        Refresh JWT token
GET    /automation/rules    List automation rules
POST   /automation/rules    Create automation rule
```

### WebSocket Events

```
→ subscribe:sensors         Subscribe to sensor updates
← sensor:data               Real-time sensor reading
→ command:execute           Execute device command
← device:status             Device status change
→ heartbeat                 Keep-alive ping
← heartbeat:ack             Keep-alive response
```

### MQTT Topics

```
devices/{id}/status         Device online/offline status
devices/{id}/sensors        Sensor readings
devices/{id}/commands       Command messages
devices/{id}/heartbeat      Device health check
devices/{id}/firmware       OTA update messages
```

## Building

Builds use Ninja + CMake from platform-specific directories under `build/`.

### Prerequisites

- CMake 3.20+
- Ninja
- C++20 compiler (clang++14+ or g++11+)
- Flutter SDK 3.16+ (for frontend)

### Build Pattern

```bash
cd build/<Platform>/<BuildType>   # e.g. build/OSX/Debug
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=<BuildType>
ninja
```

Platforms: `OSX`, `Linux`, `Windows`, `Android`, `iOS`
Build types: `Debug`, `Release`, `RelWithDebInfo`

## Testing

```bash
cd build/OSX/Debug
ninja test
```

## Project Structure

```
IoT-Dashboard-Template/
├── src/
│   ├── main.cpp                    # CLI entry point
│   ├── core/                       # Core types and interfaces
│   │   ├── include/
│   │   ├── src/
│   │   └── tests/
│   ├── network/                    # Network layer
│   │   ├── http/                   # HTTP server
│   │   ├── websocket/              # WebSocket server
│   │   └── mqtt/                   # MQTT client
│   ├── database/                   # Database layer
│   │   ├── repositories/           # Repository pattern
│   │   ├── migrations/             # Schema migrations
│   │   └── tests/
│   ├── devices/                    # Device management
│   │   ├── registry.hpp            # Device registry
│   │   ├── heartbeat.hpp           # Heartbeat monitoring
│   │   └── tests/
│   ├── automation/                 # Rule engine
│   │   ├── rule_engine.hpp         # IF-THEN rule processor
│   │   ├── condition_evaluator.hpp # Condition evaluation
│   │   └── tests/
│   ├── security/                   # Authentication & authorization
│   │   ├── jwt_handler.hpp         # JWT token management
│   │   ├── password_hash.hpp       # Password hashing
│   │   ├── rbac.hpp                # Role-based access control
│   │   └── tests/
│   ├── logging/                    # Logging infrastructure
│   ├── config/                     # Configuration management
│   ├── api/                        # API composition
│   ├── metrics/                    # System metrics
│   ├── storage/                    # Time-series storage
│   ├── utils/                      # Utilities
│   └── common/                     # Shared code
│       ├── types.hpp
│       ├── error.hpp
│       └── logging.hpp
├── flutter/                        # Flutter frontend
│   ├── lib/
│   │   ├── main.dart
│   │   ├── core/                   # Core utilities
│   │   ├── features/               # Feature modules
│   │   │   ├── dashboard/
│   │   │   ├── devices/
│   │   │   ├── automation/
│   │   │   ├── alerts/
│   │   │   ├── settings/
│   │   │   └── auth/
│   │   ├── data/                   # Data layer
│   │   ├── domain/                 # Domain layer
│   │   └── presentation/           # Presentation layer
│   └── test/
├── test/                           # Backend integration tests
├── build/                          # Platform-specific builds
│   ├── OSX/
│   ├── Linux/
│   ├── Windows/
│   └── Docker/
├── cmake/                          # CMake modules
├── docs/                           # Documentation
│   ├── ARCHITECTURE.md
│   ├── API.md
│   ├── DEVELOPMENT.md
│   ├── DEPLOYMENT.md
│   └── diagrams/
├── scripts/                        # Build and deployment scripts
│   ├── build.sh
│   ├── test.sh
│   └── deploy.sh
├── .github/
│   └── workflows/                  # CI/CD pipelines
│       ├── build.yml
│       ├── test.yml
│       └── lint.yml
├── CMakeLists.txt
├── CLAUDE.md                       # Development guide
├── README.md
└── config.json                     # Configuration template
```

## Configuration

Example `config.json`:

```json
{
  "server": {
    "http_port": 8080,
    "ws_port": 8081,
    "host": "0.0.0.0",
    "max_connections": 1000,
    "thread_pool_size": 8,
    "enable_cors": true,
    "request_timeout_ms": 30000
  },
  "database": {
    "path": "./data/iot.db",
    "backup_path": "./data/backups/",
    "retention_days": 90,
    "enable_wal": true,
    "auto_vacuum": true
  },
  "mqtt": {
    "broker": "mqtt://localhost:1883",
    "client_id": "iot-dashboard-backend",
    "username": "",
    "password": "",
    "qos": 1,
    "keep_alive_seconds": 60,
    "clean_session": false,
    "reconnect_delay_ms": 5000,
    "max_reconnect_attempts": -1
  },
  "security": {
    "enable_tls": true,
    "tls_cert": "./certs/server.crt",
    "tls_key": "./certs/server.key",
    "ca_cert": "./certs/ca.crt",
    "jwt_secret": "your-secret-key-change-in-production",
    "jwt_expiry_hours": 24,
    "refresh_token_expiry_days": 30,
    "password_min_length": 8,
    "bcrypt_rounds": 12
  },
  "logging": {
    "level": "info",
    "file_path": "./logs/iot-dashboard.log",
    "max_file_size_mb": 100,
    "max_files": 10,
    "console_output": true,
    "enable_json": false
  },
  "devices": {
    "heartbeat_timeout_seconds": 300,
    "cleanup_interval_seconds": 3600,
    "max_offline_retention_days": 30
  },
  "automation": {
    "enable": true,
    "evaluation_interval_ms": 1000,
    "max_rules": 1000,
    "max_actions_per_rule": 10
  },
  "metrics": {
    "enable": true,
    "collection_interval_seconds": 60,
    "retention_hours": 168
  }
}
```

## Development Workflow

### Incremental Module Development

This project follows an incremental, test-driven approach:

1. **Design**: Architect the module interfaces and dependencies
2. **Implement**: Write production code with RAII and modern C++20
3. **Test**: Achieve >90% code coverage with GoogleTest
4. **Review**: Run clang-format, clang-tidy, and sanitizers
5. **Document**: Update architecture diagrams and API docs
6. **Integrate**: Merge and verify integration tests pass

### Code Quality Standards

```bash
# Format code
find src -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Static analysis
clang-tidy src/**/*.cpp -- -std=c++20

# Run tests with coverage
cd build/OSX/Debug
cmake ../.. -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
ninja
ninja test
ninja coverage

# Run sanitizers
cmake ../.. -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON -DENABLE_UBSAN=ON
ninja
ninja test
```

### Testing Strategy

- **Unit Tests**: Test individual classes in isolation with mocks
- **Integration Tests**: Test module interactions (e.g., API → Database)
- **End-to-End Tests**: Test full workflows (HTTP request → MQTT → Device)
- **Performance Tests**: Benchmark critical paths (WebSocket throughput, MQTT latency)
- **Thread Safety Tests**: Concurrent access validation with ThreadSanitizer

### CI/CD Pipeline

GitHub Actions workflow:
1. Build on Linux, macOS, Windows
2. Run all tests with sanitizers
3. Check code formatting (clang-format)
4. Run static analysis (clang-tidy)
5. Generate coverage report
6. Build Flutter apps
7. Deploy documentation

## Documentation

- **[ARCHITECTURE.md](docs/ARCHITECTURE.md)**: System design and module interactions
- **[API.md](docs/API.md)**: Complete API reference with examples
- **[DEVELOPMENT.md](docs/DEVELOPMENT.md)**: Coding standards and guidelines
- **[DEPLOYMENT.md](docs/DEPLOYMENT.md)**: Production deployment guide
- **[CLAUDE.md](CLAUDE.md)**: AI-assisted development rules

## Roadmap

### Phase 1: Core Infrastructure ✓
- [x] Project structure and build system
- [x] Core types and error handling
- [x] Logging infrastructure
- [x] Configuration management

### Phase 2: Networking (In Progress)
- [ ] HTTP server with REST API
- [ ] WebSocket server with real-time updates
- [ ] MQTT client with pub/sub
- [ ] Connection pooling and retry logic

### Phase 3: Data Layer
- [ ] SQLite integration
- [ ] Repository pattern implementation
- [ ] Database migrations
- [ ] Time-series data optimization

### Phase 4: Business Logic
- [ ] Device registry and management
- [ ] Heartbeat monitoring
- [ ] Automation rule engine
- [ ] Alert system

### Phase 5: Security
- [ ] JWT authentication
- [ ] Password hashing (bcrypt)
- [ ] Role-based access control
- [ ] TLS/SSL support

### Phase 6: Flutter Frontend
- [ ] Dashboard with live charts
- [ ] Device management UI
- [ ] Automation editor
- [ ] User management

### Phase 7: Production Readiness
- [ ] Performance optimization
- [ ] Load testing
- [ ] Production deployment guide
- [ ] Monitoring and alerting

## Contributing

This is a production-grade reference implementation. Contributions should:
- Follow SOLID principles and Clean Architecture
- Include comprehensive tests (>90% coverage)
- Pass all static analysis checks
- Include documentation updates
- Follow the existing code style

## License

MIT License

---

**Status**: Active Development  
**Engineering Level**: Production-Grade  
**Standards**: Microsoft/Google/NVIDIA-level code quality  
**Built with**: Modern C++20, Flutter, Clean Architecture, SOLID principles
