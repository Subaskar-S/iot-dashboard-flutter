# Production-Grade Industrial IoT Dashboard (C++23 + Flutter)

A fully implemented Industrial IoT monitoring and control dashboard with a C++23 backend and Flutter multi-platform frontend. Built to production engineering standards (Clean Architecture, SOLID, RAII, zero raw owning pointers, 153 automated tests).

## Architecture

```
Flutter Desktop/Mobile  (macOS · iOS · Android · Web)
          ↓  HTTP REST + WebSocket
  C++23 Backend Server
          ↓  MQTT
     MQTT Broker (Mosquitto / HiveMQ)
          ↓
  ESP32 / Raspberry Pi / IoT Devices
```

## What Is Implemented

### Backend (C++23, Boost.Beast, Paho MQTT, SQLite)

| Module | Status | Tests |
|--------|--------|-------|
| `common` | ✅ | — |
| `core` | ✅ | 11 |
| `database` | ✅ | 13 |
| `network/mqtt` | ✅ | 7 |
| `network/http` | ✅ | 15 |
| `network/websocket` | ✅ | 11 |
| `devices` | ✅ | 20 |
| `automation` | ✅ | 32 |
| `security` | ✅ | 33 |
| `api` | ✅ | 11 |
| **Total** | | **153** |

### Flutter Frontend

| Feature | Status |
|---------|--------|
| Login (JWT Bearer auth) | ✅ |
| Dashboard (live device status) | ✅ |
| Devices (register/delete/command) | ✅ |
| Sensors (live charts via WebSocket) | ✅ |
| Automation (IF-THEN rule builder) | ✅ |
| Settings (server config, dark mode) | ✅ |
| NavigationRail desktop layout | ✅ |
| Dark / Light mode | ✅ |

## Quick Start

### Prerequisites

**macOS:**
```bash
brew install cmake ninja boost nlohmann-json spdlog fmt openssl sqlite3 libpaho-mqtt mosquitto
flutter sdk  # https://flutter.dev/docs/get-started/install
```

**Ubuntu/Debian:**
```bash
sudo apt install cmake ninja-build libboost-all-dev nlohmann-json3-dev \
    libspdlog-dev libfmt-dev libssl-dev libsqlite3-dev libpaho-mqtt-dev
# Install Flutter: https://flutter.dev/docs/get-started/install/linux
```

### 1. Build the Backend

```bash
mkdir -p build/OSX/Debug && cd build/OSX/Debug
cmake ../../.. -G Ninja -DCMAKE_BUILD_TYPE=Debug
ninja
ctest --output-on-failure   # 153 tests
```

### 2. Start MQTT Broker

```bash
# macOS
brew services start mosquitto

# Linux
sudo systemctl start mosquitto
```

### 3. Run the Backend

```bash
./src/iot-dashboard --serve --db ./iot.db
# HTTP API: http://localhost:8080
# WebSocket: ws://localhost:8081
```

### 4. Run Flutter

```bash
cd flutter
flutter pub get
flutter run -d macos   # or: ios, android, chrome
```

### 5. Login

Default credentials (change in production):
- **Username:** `admin`
- **Password:** `admin123`

## HTTP API

| Method | Path | Auth | Description |
|--------|------|------|-------------|
| GET | `/health` | — | Health check + uptime |
| POST | `/auth/login` | — | Login → JWT tokens |
| POST | `/auth/refresh` | — | Refresh access token |
| POST | `/auth/logout` | Bearer | Revoke token |
| GET | `/devices` | Bearer | List devices |
| POST | `/devices` | Bearer | Register device |
| DELETE | `/devices?id=` | Bearer | Unregister device |
| POST | `/commands` | Bearer | Send device command |
| GET | `/sensors` | Bearer | Query sensor history |
| GET | `/automation/rules` | Bearer | List rules |
| POST | `/automation/rules` | Bearer | Create rule |
| DELETE | `/automation/rules?id=` | Bearer | Delete rule |
| GET | `/metrics` | Bearer | System metrics |

## WebSocket Protocol

Connect to `ws://localhost:8081` after login:

```json
// Subscribe to live sensor updates
{ "type": "subscribe", "topic": "sensors" }

// Server pushes sensor data
{ "type": "sensor_data", "data": { "device_id": "temp-001", "value": 25.5 } }

// Server pushes device status
{ "type": "device_status", "data": { "device_id": "temp-001", "online": true } }

// Ping/pong heartbeat
{ "type": "ping" } → { "type": "pong" }
```

## MQTT Topics

```
devices/{id}/status       → backend  (device online/offline)
devices/{id}/sensors      → backend  (sensor readings)
devices/{id}/heartbeat    → backend  (keep-alive)
devices/{id}/commands     ← backend  (commands to device)
```

## Project Structure

```
iot-dashboard-flutter/
├── src/
│   ├── main.cpp
│   ├── common/             # Error codes, logging, types
│   ├── core/               # Domain interfaces (ports)
│   │   ├── concepts.hpp
│   │   ├── interfaces/     # IDeviceRepository, IMqttClient, ...
│   │   └── tests/
│   ├── database/           # SQLite: connection, migrations, repos
│   │   └── tests/
│   ├── network/
│   │   ├── http/           # Boost.Beast HTTP server + router
│   │   ├── websocket/      # Boost.Beast WS server + pub/sub
│   │   └── mqtt/           # Paho C async MQTT client
│   ├── devices/            # DeviceManager + HeartbeatMonitor
│   ├── automation/         # ConditionEvaluator + RuleEngine
│   ├── security/           # PBKDF2, HS256 JWT, RBAC
│   └── api/                # Composition root + controllers
│       ├── application.cpp
│       ├── middleware/
│       └── controllers/
├── flutter/
│   └── lib/
│       ├── core/           # Theme, constants, Result<T>
│       ├── domain/         # Entities + repository interfaces
│       ├── data/           # Dio, WebSocket, repository impls
│       └── presentation/
│           ├── providers/  # Riverpod notifiers
│           ├── pages/      # Login, Dashboard, Devices, ...
│           └── widgets/
├── cmake/
├── docs/
│   ├── ARCHITECTURE.md
│   ├── API.md
│   ├── DEVELOPMENT.md
│   └── DEPLOYMENT.md
├── CMakeLists.txt
├── CLAUDE.md
└── QUICK_START.md
```

## Technology Stack

### Backend

| Concern | Library / Tool |
|---------|---------------|
| Language | C++23 |
| Build | CMake 3.20+ + Ninja |
| HTTP/WebSocket | Boost.Beast (Boost.Asio) |
| MQTT | Eclipse Paho MQTT C (async) |
| Database | SQLite3 (WAL mode) |
| JSON | nlohmann/json |
| Logging | spdlog + fmt |
| Security | OpenSSL (PBKDF2, HMAC-SHA256) |
| Testing | GoogleTest + GMock |

### Frontend

| Concern | Package |
|---------|---------|
| Framework | Flutter 3.41+ |
| State | flutter_riverpod |
| Navigation | go_router |
| HTTP | dio |
| WebSocket | web_socket_channel |
| Charts | fl_chart |
| Storage | flutter_secure_storage |

## Security

- **Passwords**: PBKDF2-SHA256 (600,000 iterations, random 16-byte salt)
- **Tokens**: HS256 JWT (access: 1h, refresh: 30d)
- **RBAC**: Admin / Operator / Viewer roles
- **Transport**: TLS support via OpenSSL (configure in AppConfig)
- **Constant-time compare**: `CRYPTO_memcmp` throughout

## Automation Example

IF temperature > 30°C → turn on fan:

```json
{
  "id": "rule-001",
  "name": "High Temperature",
  "enabled": true,
  "conditions": [{ "sensor_type": "temperature", "operator": ">", "threshold": 30.0 }],
  "actions": [{ "device_id": "fan-001", "command": "turn_on", "parameters": {} }]
}
```

## Testing

```bash
# C++ tests (from build dir)
ctest --output-on-failure
# → 153 tests, 0 failures

# Flutter analysis
cd flutter && flutter analyze
# → 0 errors

# Flutter build check
flutter build macos --debug
```

## Documentation

- [**ARCHITECTURE.md**](docs/ARCHITECTURE.md) — Module design, data flows, DB schema
- [**API.md**](docs/API.md) — Full REST + WebSocket + MQTT reference
- [**DEVELOPMENT.md**](docs/DEVELOPMENT.md) — Coding standards, adding modules
- [**DEPLOYMENT.md**](docs/DEPLOYMENT.md) — Production deployment (Docker, systemd)
- [**CLAUDE.md**](CLAUDE.md) — AI-assisted development rules

## License

MIT License

---

**Status**: Complete v1.0  
**Tests**: 153 C++ (passing) · 0 Flutter errors  
**Platforms**: macOS · Linux · Windows (backend) · macOS · iOS · Android · Web (Flutter)
