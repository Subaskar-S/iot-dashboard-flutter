# Architecture Documentation

## System Overview

The Industrial IoT Dashboard is a distributed system designed with Clean Architecture principles, consisting of a C++20 backend server and Flutter multi-platform frontend communicating over HTTP REST and WebSocket protocols.

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                   Flutter Application                        │
│  (Windows, macOS, Linux, iOS, Android)                      │
│                                                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ Presentation │  │   Domain     │  │     Data     │     │
│  │    Layer     │  │    Layer     │  │    Layer     │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
└─────────────────────────────────────────────────────────────┘
                            │
                    ┌───────┴───────┐
                    │               │
            REST API (HTTP)    WebSocket
                    │               │
┌───────────────────┴───────────────┴─────────────────────────┐
│                   C++20 Backend Server                       │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │                    API Layer                          │  │
│  │         (HTTP Routes, WebSocket Handlers)            │  │
│  └──────────────────────────────────────────────────────┘  │
│                            │                                │
│  ┌──────────────────────────────────────────────────────┐  │
│  │                 Business Logic Layer                  │  │
│  │  ┌─────────┐  ┌──────────┐  ┌────────────┐          │  │
│  │  │ Devices │  │Automation│  │  Security  │          │  │
│  │  │ Manager │  │  Engine  │  │   (Auth)   │          │  │
│  │  └─────────┘  └──────────┘  └────────────┘          │  │
│  └──────────────────────────────────────────────────────┘  │
│                            │                                │
│  ┌──────────────────────────────────────────────────────┐  │
│  │              Infrastructure Layer                     │  │
│  │  ┌─────────┐  ┌──────────┐  ┌────────────┐          │  │
│  │  │Database │  │   MQTT   │  │  Logging   │          │  │
│  │  │ (SQLite)│  │  Client  │  │  (spdlog)  │          │  │
│  │  └─────────┘  └──────────┘  └────────────┘          │  │
│  └──────────────────────────────────────────────────────┘  │
└───────────────────────────────────────┬─────────────────────┘
                                        │
                                   MQTT Protocol
                                        │
┌───────────────────────────────────────┴─────────────────────┐
│                      MQTT Broker                             │
│              (Mosquitto / HiveMQ / EMQX)                    │
└───────────────────────────────────────┬─────────────────────┘
                                        │
                    ┌───────────────────┼───────────────────┐
                    │                   │                   │
            ┌───────▼───────┐   ┌──────▼──────┐   ┌───────▼───────┐
            │     ESP32     │   │ Raspberry Pi │   │  Other IoT    │
            │   Sensors     │   │   Gateway    │   │   Devices     │
            └───────────────┘   └──────────────┘   └───────────────┘
```

## Clean Architecture Layers

### 1. Entities (Core Domain)

**Location**: `src/core/`

Pure business logic with no dependencies on external frameworks.

```cpp
// Core domain types
struct DeviceId { std::string value; };
struct SensorReading { /* ... */ };
struct AutomationRule { /* ... */ };

// Domain interfaces (ports)
class IDeviceRepository;
class IMqttClient;
class IAuthenticationService;
```

### 2. Use Cases (Application Business Rules)

**Location**: `src/devices/`, `src/automation/`, `src/security/`

Application-specific business rules.

```cpp
class RegisterDeviceUseCase
{
public:
    RegisterDeviceUseCase( IDeviceRepository& repo );
    Result<DeviceId> Execute( const RegisterDeviceRequest& request );
};

class EvaluateAutomationRulesUseCase
{
public:
    Result<std::vector<Action>> Execute( const SensorReading& reading );
};
```

### 3. Interface Adapters

**Location**: `src/api/`, `src/database/repositories/`

Converts data between use cases and external systems.

```cpp
class HttpDeviceController
{
public:
    void HandleRegisterDevice( const HttpRequest& req, HttpResponse& res );
    void HandleListDevices( const HttpRequest& req, HttpResponse& res );
};

class SqliteDeviceRepository : public IDeviceRepository
{
    // Implements repository with SQLite
};
```

### 4. Frameworks & Drivers

**Location**: `src/network/`, `src/database/`, `src/logging/`

External frameworks and tools.

- Boost.Asio for networking
- SQLite for persistence
- spdlog for logging
- Eclipse Paho for MQTT

## Module Breakdown

### Core Module (`src/core/`)

**Responsibility**: Domain types, interfaces, and core abstractions

```
core/
├── include/
│   ├── types.hpp              # Domain types (DeviceInfo, SensorReading, etc.)
│   ├── interfaces/
│   │   ├── i_device_repository.hpp
│   │   ├── i_mqtt_client.hpp
│   │   ├── i_auth_service.hpp
│   │   └── i_rule_evaluator.hpp
│   ├── concepts.hpp           # C++20 concepts (Device, Sensor, Repository)
│   └── error.hpp              # Error codes and Result<T>
└── tests/
```

**Key Design Decisions**:
- No dependencies on external libraries
- Pure interfaces (abstract base classes)
- C++20 concepts for compile-time constraints
- `std::expected<T, Error>` for error handling

### Network Module (`src/network/`)

**Responsibility**: All network communication

#### HTTP Submodule (`network/http/`)

```cpp
class HttpServer
{
public:
    HttpServer( boost::asio::io_context& ioc, uint16_t port );
    
    void AddRoute( HttpMethod method, 
                   std::string_view path, 
                   RouteHandler handler );
    
    void Start();
    void Stop();
};

class HttpRouter
{
public:
    void RegisterController( std::unique_ptr<IController> controller );
    HttpResponse Route( const HttpRequest& request );
};
```

**Technology**: Boost.Beast (HTTP/1.1 and HTTP/2)

**Features**:
- Async I/O with coroutines
- Thread pool for request handling
- Connection pooling
- Request timeout management
- CORS support
- Compression (gzip)

#### WebSocket Submodule (`network/websocket/`)

```cpp
class WebSocketServer
{
public:
    void OnConnect( ConnectionHandler handler );
    void OnMessage( MessageHandler handler );
    void OnDisconnect( ConnectionHandler handler );
    
    void Broadcast( const std::string& message );
    void SendTo( const ConnectionId& id, const std::string& message );
};

class WebSocketSession
{
    void Subscribe( std::string_view topic );
    void Unsubscribe( std::string_view topic );
    void Send( const nlohmann::json& message );
};
```

**Technology**: Boost.Beast WebSocket

**Features**:
- Binary and text message support
- Heartbeat/ping-pong
- Automatic reconnection
- Topic-based subscriptions
- Message queueing
- Backpressure handling

#### MQTT Submodule (`network/mqtt/`)

```cpp
class MqttClient : public IMqttClient
{
public:
    MqttClient( const MqttConfig& config );
    
    Result<void> Connect();
    Result<void> Disconnect();
    
    Result<void> Subscribe( std::string_view topic, QoS qos );
    Result<void> Publish( std::string_view topic, 
                          std::span<const std::byte> payload,
                          QoS qos,
                          bool retain );
    
    void SetMessageCallback( MessageCallback callback );
};
```

**Technology**: Eclipse Paho MQTT C++

**Features**:
- QoS 0, 1, 2 support
- Retained messages
- Last Will and Testament (LWT)
- Automatic reconnection with exponential backoff
- Topic wildcard subscriptions
- TLS/SSL support

### Database Module (`src/database/`)

**Responsibility**: Data persistence and retrieval

```
database/
├── include/
│   ├── connection_pool.hpp
│   ├── transaction.hpp
│   ├── migration_manager.hpp
│   └── repositories/
│       ├── device_repository.hpp
│       ├── sensor_repository.hpp
│       ├── user_repository.hpp
│       ├── automation_repository.hpp
│       └── alert_repository.hpp
├── src/
│   ├── migrations/
│   │   ├── 001_initial_schema.sql
│   │   ├── 002_add_automation_rules.sql
│   │   └── 003_add_user_roles.sql
└── tests/
```

**Schema**:

```sql
-- Devices
CREATE TABLE devices (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    type TEXT NOT NULL,
    status TEXT NOT NULL,
    firmware_version TEXT,
    ip_address TEXT,
    battery_level REAL,
    capabilities TEXT, -- JSON array
    last_seen INTEGER,
    created_at INTEGER NOT NULL,
    updated_at INTEGER NOT NULL
);

-- Sensor Readings
CREATE TABLE sensor_readings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id TEXT NOT NULL,
    sensor_type TEXT NOT NULL,
    value REAL NOT NULL,
    unit TEXT NOT NULL,
    timestamp INTEGER NOT NULL,
    FOREIGN KEY (device_id) REFERENCES devices(id) ON DELETE CASCADE
);
CREATE INDEX idx_sensor_readings_device_timestamp 
    ON sensor_readings(device_id, timestamp);

-- Users
CREATE TABLE users (
    id TEXT PRIMARY KEY,
    username TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    role TEXT NOT NULL,
    created_at INTEGER NOT NULL,
    updated_at INTEGER NOT NULL
);

-- Automation Rules
CREATE TABLE automation_rules (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    enabled INTEGER NOT NULL DEFAULT 1,
    condition TEXT NOT NULL, -- JSON
    action TEXT NOT NULL,    -- JSON
    created_by TEXT NOT NULL,
    created_at INTEGER NOT NULL,
    updated_at INTEGER NOT NULL,
    FOREIGN KEY (created_by) REFERENCES users(id)
);

-- Alerts
CREATE TABLE alerts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id TEXT NOT NULL,
    severity TEXT NOT NULL,
    message TEXT NOT NULL,
    acknowledged INTEGER DEFAULT 0,
    created_at INTEGER NOT NULL,
    FOREIGN KEY (device_id) REFERENCES devices(id)
);
```

**Repository Pattern**:

```cpp
class IDeviceRepository
{
public:
    virtual ~IDeviceRepository() = default;
    
    virtual Result<DeviceInfo> GetById( const DeviceId& id ) = 0;
    virtual Result<std::vector<DeviceInfo>> GetAll() = 0;
    virtual Result<void> Add( const DeviceInfo& device ) = 0;
    virtual Result<void> Update( const DeviceInfo& device ) = 0;
    virtual Result<void> Delete( const DeviceId& id ) = 0;
    virtual Result<std::vector<DeviceInfo>> FindByStatus( DeviceStatus status ) = 0;
};

class SqliteDeviceRepository : public IDeviceRepository
{
private:
    std::shared_ptr<ConnectionPool> m_pool;
    std::shared_ptr<spdlog::logger> m_logger;
};
```

### Device Manager (`src/devices/`)

**Responsibility**: Device lifecycle management

```cpp
class DeviceManager
{
public:
    DeviceManager( IDeviceRepository& repo,
                   IMqttClient& mqtt,
                   std::shared_ptr<spdlog::logger> logger );
    
    Result<DeviceId> RegisterDevice( const RegisterDeviceRequest& req );
    Result<void> UnregisterDevice( const DeviceId& id );
    Result<DeviceInfo> GetDevice( const DeviceId& id );
    Result<std::vector<DeviceInfo>> ListDevices( const DeviceFilter& filter );
    
    Result<void> SendCommand( const DeviceId& id, const Command& cmd );
    
    void StartHeartbeatMonitoring();
    void StopHeartbeatMonitoring();
};

class HeartbeatMonitor
{
public:
    void RegisterDevice( const DeviceId& id, 
                        std::chrono::seconds timeout );
    
    void RecordHeartbeat( const DeviceId& id );
    
    void SetTimeoutCallback( 
        std::function<void(const DeviceId&)> callback );
};
```

### Automation Engine (`src/automation/`)

**Responsibility**: Rule-based automation (IF-THEN-ELSE)

```cpp
// Rule representation
struct Condition
{
    std::string sensor_type;
    ComparisonOperator op;  // <, >, ==, !=, <=, >=
    double threshold;
};

struct Action
{
    std::string device_id;
    std::string command;
    nlohmann::json parameters;
};

struct AutomationRule
{
    std::string id;
    std::string name;
    bool enabled;
    std::vector<Condition> conditions;  // AND logic
    std::vector<Action> actions;
};

class RuleEngine
{
public:
    RuleEngine( IAutomationRepository& repo,
                IDeviceManager& devices );
    
    Result<void> AddRule( const AutomationRule& rule );
    Result<void> RemoveRule( const std::string& rule_id );
    Result<void> EnableRule( const std::string& rule_id );
    Result<void> DisableRule( const std::string& rule_id );
    
    // Evaluate all active rules against a sensor reading
    Result<std::vector<Action>> Evaluate( const SensorReading& reading );
    
    void StartEvaluation( std::chrono::milliseconds interval );
    void StopEvaluation();
};
```

**Example Rule**:
```json
{
  "id": "rule-001",
  "name": "High Temperature Alert",
  "enabled": true,
  "conditions": [
    {
      "sensor_type": "temperature",
      "operator": ">",
      "threshold": 30.0
    }
  ],
  "actions": [
    {
      "device_id": "fan-001",
      "command": "turn_on",
      "parameters": {
        "speed": "high"
      }
    },
    {
      "device_id": "alert-system",
      "command": "send_notification",
      "parameters": {
        "message": "Temperature exceeded 30°C"
      }
    }
  ]
}
```

### Security Module (`src/security/`)

**Responsibility**: Authentication and authorization

```cpp
class AuthenticationService
{
public:
    Result<LoginResponse> Login( 
        std::string_view username, 
        std::string_view password );
    
    Result<TokenPair> RefreshToken( std::string_view refresh_token );
    
    Result<void> Logout( std::string_view token );
    
    Result<UserInfo> ValidateToken( std::string_view token );
};

class JwtHandler
{
public:
    Result<std::string> Generate( const UserClaims& claims,
                                  std::chrono::seconds expiry );
    
    Result<UserClaims> Verify( std::string_view token );
};

class PasswordHasher
{
public:
    std::string Hash( std::string_view password );
    bool Verify( std::string_view password, std::string_view hash );
};

// RBAC
enum class Role
{
    Admin,      // Full access
    Operator,   // Read/write devices, view data
    Viewer      // Read-only access
};

class AccessControl
{
public:
    bool CanAccessResource( Role role, 
                           std::string_view resource,
                           Permission permission );
};
```

## Data Flow Examples

### Example 1: Device Registration

```
1. Flutter App → POST /devices { name, type }
2. HttpServer → HttpDeviceController
3. Controller → RegisterDeviceUseCase
4. UseCase → DeviceRepository.Add()
5. Repository → SQLite INSERT
6. UseCase → MqttClient.Subscribe("devices/{id}/#")
7. Response ← DeviceId
```

### Example 2: Real-time Sensor Update

```
1. ESP32 → MQTT PUBLISH devices/temp-001/sensors { temp: 25.5 }
2. MqttClient → MessageCallback
3. Callback → DeviceManager.ProcessSensorReading()
4. Manager → SensorRepository.Add()
5. Manager → RuleEngine.Evaluate()
6. (if rule matched) → DeviceManager.SendCommand()
7. Manager → WebSocketServer.Broadcast({ type: "sensor", data })
8. Flutter App ← WebSocket message
9. UI updates chart in real-time
```

### Example 3: Automation Rule Execution

```
1. Sensor reading: Temperature = 32°C
2. RuleEngine.Evaluate() checks all enabled rules
3. Rule matched: IF temp > 30 THEN turn_on(fan)
4. Action generated: { device: "fan-001", cmd: "turn_on" }
5. DeviceManager.SendCommand()
6. MqttClient.Publish("devices/fan-001/commands", "turn_on")
7. ESP32 receives command and activates fan
8. ESP32 publishes status update
9. WebSocket broadcasts to all connected clients
```

## Concurrency Model

### Thread Pool Architecture

```cpp
class ThreadPool
{
public:
    ThreadPool( size_t num_threads );
    
    template<typename F>
    auto Submit( F&& task ) -> std::future<decltype(task())>;
};

// Backend uses multiple thread pools
struct ServerThreadPools
{
    ThreadPool http_workers;      // HTTP request handlers
    ThreadPool ws_workers;        // WebSocket message handlers
    ThreadPool mqtt_workers;      // MQTT message processing
    ThreadPool db_workers;        // Database operations
    ThreadPool automation_workers; // Rule evaluation
};
```

### Synchronization

- **Lock-free data structures** where possible (concurrent queues)
- **Reader-writer locks** for device registry
- **Mutex per resource** (avoid global locks)
- **RAII lock guards** (std::lock_guard, std::unique_lock)
- **Thread-safe repository pattern** with connection pooling

## Error Handling Strategy

### Result<T> Type

```cpp
template<typename T>
using Result = std::expected<T, Error>;

enum class Error
{
    // Network errors
    ConnectionFailed,
    Timeout,
    InvalidRequest,
    
    // Database errors
    DatabaseError,
    RecordNotFound,
    DuplicateKey,
    
    // Business logic errors
    InvalidInput,
    Unauthorized,
    DeviceOffline,
    
    // MQTT errors
    MqttConnectionLost,
    PublishFailed,
    SubscriptionFailed
};
```

### Error Propagation

```cpp
Result<DeviceInfo> GetDevice( const DeviceId& id )
{
    auto result = m_repository.GetById( id );
    if ( !result )
        return std::unexpected( result.error() );
    
    return result.value();
}
```

## Dependency Injection

### Manual DI (Constructor Injection)

```cpp
class Application
{
public:
    Application( const Config& config )
    {
        // Infrastructure
        m_logger = CreateLogger( "Application" );
        m_db_pool = std::make_shared<ConnectionPool>( config.database );
        m_mqtt_client = std::make_shared<MqttClient>( config.mqtt );
        
        // Repositories
        m_device_repo = std::make_unique<SqliteDeviceRepository>( 
            m_db_pool, m_logger );
        
        // Services
        m_device_manager = std::make_unique<DeviceManager>(
            *m_device_repo, *m_mqtt_client, m_logger );
        
        m_auth_service = std::make_unique<AuthenticationService>(
            *m_user_repo, config.security );
        
        // API
        m_http_server = std::make_unique<HttpServer>( 
            m_io_context, config.server.http_port );
        
        RegisterRoutes();
    }
};
```

## Performance Considerations

### Database Optimizations

- **Connection pooling**: Reuse SQLite connections
- **Prepared statements**: Cache compiled queries
- **Batch inserts**: Group sensor readings
- **Indexes**: On frequently queried columns (device_id, timestamp)
- **WAL mode**: Write-Ahead Logging for concurrent reads

### Network Optimizations

- **Keep-alive connections**: Reduce TCP handshake overhead
- **Message batching**: Group WebSocket messages
- **Compression**: gzip for HTTP responses
- **Binary protocol**: Protobuf for MQTT payloads (optional)

### Memory Management

- **Object pooling**: Reuse HTTP request/response objects
- **Move semantics**: Avoid unnecessary copies
- **Small string optimization**: std::string_view where possible
- **Smart pointers**: shared_ptr for shared state, unique_ptr for ownership

## Security Considerations

### Authentication Flow

```
1. User enters credentials
2. POST /auth/login { username, password }
3. Server verifies password_hash (bcrypt)
4. Generate JWT access token (15 min expiry)
5. Generate refresh token (30 days expiry)
6. Return { access_token, refresh_token }
7. Client stores tokens securely
8. All API requests include: Authorization: Bearer <access_token>
9. On expiry, POST /auth/refresh { refresh_token }
```

### Transport Security

- **TLS 1.3** for all HTTP/WebSocket/MQTT traffic
- **Certificate pinning** in Flutter app
- **MQTT over TLS** with client certificates

### Data Security

- **Password hashing**: bcrypt with 12 rounds
- **JWT signing**: HMAC-SHA256
- **SQL injection prevention**: Prepared statements only
- **Input validation**: Strong typing + validation layer

## Testing Strategy

### Unit Tests

```cpp
TEST( DeviceManagerTest, RegisterDevice_Success )
{
    // Arrange
    MockDeviceRepository repo;
    MockMqttClient mqtt;
    auto logger = CreateTestLogger();
    DeviceManager manager( repo, mqtt, logger );
    
    EXPECT_CALL( repo, Add( _ ) ).WillOnce( Return( Result<void>{} ) );
    EXPECT_CALL( mqtt, Subscribe( _, _ ) ).WillOnce( Return( Result<void>{} ) );
    
    RegisterDeviceRequest request{ "temp-001", "temperature" };
    
    // Act
    auto result = manager.RegisterDevice( request );
    
    // Assert
    ASSERT_TRUE( result );
    EXPECT_EQ( result->value, "temp-001" );
}
```

### Integration Tests

```cpp
TEST_F( ApiIntegrationTest, DeviceLifecycle )
{
    // Start test server
    TestServer server;
    
    // Register device
    auto register_response = server.Post( "/devices", {
        { "name", "test-device" },
        { "type", "sensor" }
    });
    ASSERT_EQ( register_response.status, 201 );
    auto device_id = register_response.json["id"];
    
    // Get device
    auto get_response = server.Get( "/devices/" + device_id );
    ASSERT_EQ( get_response.status, 200 );
    EXPECT_EQ( get_response.json["name"], "test-device" );
    
    // Delete device
    auto delete_response = server.Delete( "/devices/" + device_id );
    ASSERT_EQ( delete_response.status, 204 );
}
```

## Monitoring and Observability

### Metrics Collection

```cpp
struct SystemMetrics
{
    double cpu_usage_percent;
    uint64_t memory_usage_bytes;
    size_t connected_devices;
    size_t active_websocket_clients;
    double mqtt_publish_latency_ms;
    double api_request_latency_ms;
    uint64_t requests_per_second;
    uint64_t errors_per_second;
};

class MetricsCollector
{
public:
    void RecordRequestLatency( std::chrono::milliseconds latency );
    void RecordMqttLatency( std::chrono::milliseconds latency );
    void IncrementActiveConnections();
    void DecrementActiveConnections();
    
    SystemMetrics GetMetrics() const;
};
```

### Structured Logging

```json
{
  "timestamp": "2026-07-30T10:30:45.123Z",
  "level": "info",
  "module": "DeviceManager",
  "message": "Device registered successfully",
  "context": {
    "device_id": "temp-001",
    "device_type": "temperature",
    "user_id": "admin"
  }
}
```

## Deployment Architecture

### Production Deployment

```
┌────────────────────────────────────────────────────┐
│                  Load Balancer                      │
│              (nginx or cloud LB)                   │
└────────────────────┬───────────────────────────────┘
                     │
        ┌────────────┼────────────┐
        │            │            │
   ┌────▼───┐   ┌───▼────┐   ┌───▼────┐
   │Backend │   │Backend │   │Backend │
   │Instance│   │Instance│   │Instance│
   │   1    │   │   2    │   │   3    │
   └────┬───┘   └───┬────┘   └───┬────┘
        │           │            │
        └───────────┼────────────┘
                    │
         ┌──────────┼──────────┐
         │          │          │
    ┌────▼───┐ ┌───▼────┐ ┌───▼────┐
    │ SQLite │ │  MQTT  │ │ Redis  │
    │  (NFS) │ │ Broker │ │ Cache  │
    └────────┘ └────────┘ └────────┘
```

### Docker Deployment

```dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libboost-all-dev \
    libsqlite3-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

COPY build/Linux/Release/iot-dashboard /app/
COPY config.json /app/
COPY certs/ /app/certs/

WORKDIR /app
EXPOSE 8080 8081

CMD ["./iot-dashboard", "--config", "config.json", "--serve"]
```

---

**Next Steps**: See [DEVELOPMENT.md](DEVELOPMENT.md) for coding standards and [API.md](API.md) for complete API reference.
