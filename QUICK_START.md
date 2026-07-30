# Quick Start Guide

## 1. Copy Template

```bash
cp -r IoT-Dashboard-Template ~/my-iot-project
cd ~/my-iot-project
```

## 2. Customize Project Name

Edit `CMakeLists.txt` (line 3):
```cmake
project(MyIoTProject VERSION 1.0.0 LANGUAGES CXX)
```

## 3. Install Dependencies

### macOS
```bash
brew install cmake ninja boost nlohmann-json spdlog fmt openssl sqlite3
```

### Linux (Ubuntu/Debian)
```bash
sudo apt install cmake ninja-build libboost-all-dev \
    nlohmann-json3-dev libspdlog-dev libfmt-dev \
    libssl-dev libsqlite3-dev
```

## 4. Build

```bash
mkdir -p build/$(uname)/Debug
cd build/$(uname)/Debug
cmake ../.. -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
ninja
```

## 5. Run

```bash
./iot-dashboard --help
./iot-dashboard --serve --verbose
```

## Code Style Quick Reference

### Naming
```cpp
class DeviceManager {};           // PascalCase
void ProcessData() {};            // PascalCase
int deviceCount;                  // camelCase
constexpr int kMaxSize = 100;    // kCamelCase
std::string m_deviceId;           // m_ prefix for members
```

### Braces
```cpp
if ( condition )
{
    DoSomething();
}
```

### Spacing
```cpp
if ( x > 0 && y < 100 )           // Spaces inside parentheses
for ( auto& item : items )
```

### Error Handling
```cpp
Result<Data> GetData( const std::string& id )
{
    if ( id.empty() )
        return std::unexpected( Error::InvalidInput );
    return data;
}
```

### Logging
```cpp
auto logger = CreateLogger( "MyModule" );
logger->info( "Device connected: id={}", deviceId );
```

## Format Code

```bash
# Format all files
find src -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Check formatting
clang-format --dry-run src/main.cpp
```

## Add a New Module

1. Create directory: `src/mymodule/`
2. Add `CMakeLists.txt`:
   ```cmake
   add_library(iot_mymodule STATIC mycomponent.cpp)
   target_link_libraries(iot_mymodule PUBLIC iot_common)
   target_compile_features(iot_mymodule PUBLIC cxx_std_20)
   ```
3. Add to `src/CMakeLists.txt`:
   ```cmake
   add_subdirectory(mymodule)
   ```
4. Link from main:
   ```cmake
   target_link_libraries(iot-dashboard PRIVATE iot_mymodule)
   ```

## Complete Documentation

- **PROJECT_TEMPLATE_GUIDE.md** — Full usage guide
- **docs/DEVELOPMENT.md** — Coding standards
- **README.md** — Project overview
