# Quick Start

## macOS — Full Stack in 5 Minutes

### 1. Install Dependencies

```bash
brew install cmake ninja boost nlohmann-json spdlog fmt openssl sqlite3 libpaho-mqtt mosquitto
```

### 2. Build the Backend

```bash
mkdir -p build/OSX/Debug && cd build/OSX/Debug
cmake ../../.. -G Ninja -DCMAKE_BUILD_TYPE=Debug
ninja
ctest --output-on-failure   # must show 153 passed
```

### 3. Start MQTT Broker

```bash
brew services start mosquitto
# or run manually:
/opt/homebrew/sbin/mosquitto -c /opt/homebrew/etc/mosquitto/mosquitto.conf
```

### 4. Start the Backend

```bash
# From build/OSX/Debug
./src/iot-dashboard --serve --db ./iot.db
```

Backend is now running:
- REST API → `http://localhost:8080`
- WebSocket → `ws://localhost:8081`

### 5. Run the Flutter App

```bash
cd flutter
flutter pub get
flutter run -d macos
```

Login with `admin` / `admin123`.

---

## Linux (Ubuntu/Debian)

```bash
sudo apt install cmake ninja-build libboost-all-dev nlohmann-json3-dev \
    libspdlog-dev libfmt-dev libssl-dev libsqlite3-dev \
    libpaho-mqtt-dev mosquitto mosquitto-clients

mkdir -p build/Linux/Debug && cd build/Linux/Debug
cmake ../../.. -G Ninja -DCMAKE_BUILD_TYPE=Debug
ninja
./src/iot-dashboard --serve --db ./iot.db
```

---

## CLI Reference

```
./iot-dashboard [options]

  --serve              Start HTTP + WebSocket server (blocking)
  --db <path>          SQLite database file (default: ./data/iot.db)
  --mqtt-broker <url>  MQTT broker URL (default: tcp://localhost:1883)
  --port <n>           HTTP port (default: 8080)
  --ws-port <n>        WebSocket port (default: 8081)
  --log-level <level>  trace|debug|info|warn|error (default: info)
  --verbose            Shorthand for --log-level debug
  --help               Show this help
```

---

## Quick API Test

```bash
# Health check (no auth)
curl http://localhost:8080/health

# Login
TOKEN=$(curl -s -X POST http://localhost:8080/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' | python3 -m json.tool | grep access_token | awk -F'"' '{print $4}')

# Register a device
curl -s -X POST http://localhost:8080/devices \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"id":"temp-001","name":"Temperature Sensor","type":"sensor","protocol":"mqtt"}'

# List devices
curl -s http://localhost:8080/devices \
  -H "Authorization: Bearer $TOKEN" | python3 -m json.tool
```

---

## Add a New Backend Module

1. Create `src/mymodule/`
2. Add `CMakeLists.txt`:
   ```cmake
   add_library(iot_mymodule STATIC mycomponent.cpp)
   target_link_libraries(iot_mymodule PUBLIC iot_core)
   target_compile_features(iot_mymodule PUBLIC cxx_std_23)
   if(BUILD_TESTING)
       add_subdirectory(tests)
   endif()
   ```
3. Add `mymodule` to `IOT_MODULES` in `src/CMakeLists.txt`
4. Write tests in `src/mymodule/tests/`

## Code Formatting

```bash
# Format all C++ files
find src -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Check only (CI)
find src -name "*.cpp" -o -name "*.hpp" | xargs clang-format --dry-run -Werror
```

## Further Reading

- [**README.md**](README.md) — Full project overview
- [**docs/ARCHITECTURE.md**](docs/ARCHITECTURE.md) — Module design
- [**docs/API.md**](docs/API.md) — API reference
- [**docs/DEVELOPMENT.md**](docs/DEVELOPMENT.md) — Coding standards
- [**docs/DEPLOYMENT.md**](docs/DEPLOYMENT.md) — Production deployment
