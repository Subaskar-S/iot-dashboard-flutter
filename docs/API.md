# API Reference

## Overview

The Industrial IoT Dashboard backend exposes two primary interfaces:
1. **REST API** (HTTP) - CRUD operations and queries
2. **WebSocket API** - Real-time bidirectional communication

Base URL: `http://localhost:8080` (configurable)  
WebSocket URL: `ws://localhost:8081` (configurable)

## Authentication

All API endpoints (except `/health` and `/auth/*`) require JWT authentication.

### Headers

```
Authorization: Bearer <access_token>
Content-Type: application/json
```

### Authentication Flow

```http
POST /auth/login
{
  "username": "admin",
  "password": "secure_password"
}

Response 200:
{
  "access_token": "eyJhbGc...",
  "refresh_token": "eyJhbGc...",
  "expires_in": 3600,
  "token_type": "Bearer",
  "user": {
    "id": "user-001",
    "username": "admin",
    "role": "Admin"
  }
}
```

---

## REST API Endpoints

### Health Check

#### GET /health

Check server health and version.

**Request:**
```http
GET /health
```

**Response 200:**
```json
{
  "status": "healthy",
  "version": "1.0.0",
  "uptime_seconds": 3600,
  "timestamp": "2026-07-30T10:30:00Z"
}
```

---

### Authentication

#### POST /auth/login

Authenticate user and receive JWT tokens.

**Request:**
```http
POST /auth/login
Content-Type: application/json

{
  "username": "admin",
  "password": "secure_password"
}
```

**Response 200:**
```json
{
  "access_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "refresh_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "expires_in": 3600,
  "token_type": "Bearer",
  "user": {
    "id": "user-001",
    "username": "admin",
    "role": "Admin"
  }
}
```

**Response 401:**
```json
{
  "error": "Unauthorized",
  "message": "Invalid username or password"
}
```

#### POST /auth/refresh

Refresh access token using refresh token.

**Request:**
```http
POST /auth/refresh
Content-Type: application/json

{
  "refresh_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
}
```

**Response 200:**
```json
{
  "access_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "expires_in": 3600,
  "token_type": "Bearer"
}
```

#### POST /auth/logout

Invalidate current token.

**Request:**
```http
POST /auth/logout
Authorization: Bearer <access_token>
```

**Response 204:** No content

---

### Devices

#### GET /devices

List all devices with optional filtering.

**Query Parameters:**
- `status` (optional): Filter by status (`online`, `offline`, `error`)
- `type` (optional): Filter by device type
- `limit` (optional): Max results (default: 100)
- `offset` (optional): Pagination offset (default: 0)

**Request:**
```http
GET /devices?status=online&limit=10
Authorization: Bearer <access_token>
```

**Response 200:**
```json
{
  "devices": [
    {
      "id": "temp-001",
      "name": "Temperature Sensor 1",
      "type": "temperature",
      "status": "online",
      "firmware_version": "v1.2.3",
      "ip_address": "192.168.1.100",
      "battery_level": 85.5,
      "capabilities": ["temperature", "humidity"],
      "last_seen": "2026-07-30T10:29:55Z",
      "created_at": "2026-07-01T08:00:00Z",
      "updated_at": "2026-07-30T10:29:55Z"
    }
  ],
  "total": 1,
  "limit": 10,
  "offset": 0
}
```

#### GET /devices/{id}

Get a specific device by ID.

**Request:**
```http
GET /devices/temp-001
Authorization: Bearer <access_token>
```

**Response 200:**
```json
{
  "id": "temp-001",
  "name": "Temperature Sensor 1",
  "type": "temperature",
  "status": "online",
  "firmware_version": "v1.2.3",
  "ip_address": "192.168.1.100",
  "battery_level": 85.5,
  "capabilities": ["temperature", "humidity"],
  "last_seen": "2026-07-30T10:29:55Z",
  "created_at": "2026-07-01T08:00:00Z",
  "updated_at": "2026-07-30T10:29:55Z"
}
```

**Response 404:**
```json
{
  "error": "NotFound",
  "message": "Device not found"
}
```

#### POST /devices

Register a new device.

**Request:**
```http
POST /devices
Authorization: Bearer <access_token>
Content-Type: application/json

{
  "id": "temp-002",
  "name": "Temperature Sensor 2",
  "type": "temperature",
  "ip_address": "192.168.1.101",
  "capabilities": ["temperature", "humidity", "pressure"]
}
```

**Response 201:**
```json
{
  "id": "temp-002",
  "name": "Temperature Sensor 2",
  "type": "temperature",
  "status": "offline",
  "firmware_version": null,
  "ip_address": "192.168.1.101",
  "battery_level": null,
  "capabilities": ["temperature", "humidity", "pressure"],
  "last_seen": null,
  "created_at": "2026-07-30T10:30:00Z",
  "updated_at": "2026-07-30T10:30:00Z"
}
```

**Response 400:**
```json
{
  "error": "BadRequest",
  "message": "Invalid device data",
  "details": {
    "id": "Device ID already exists"
  }
}
```

#### PUT /devices/{id}

Update device information.

**Request:**
```http
PUT /devices/temp-001
Authorization: Bearer <access_token>
Content-Type: application/json

{
  "name": "Updated Sensor Name",
  "ip_address": "192.168.1.102"
}
```

**Response 200:**
```json
{
  "id": "temp-001",
  "name": "Updated Sensor Name",
  "type": "temperature",
  "status": "online",
  "firmware_version": "v1.2.3",
  "ip_address": "192.168.1.102",
  "battery_level": 85.5,
  "capabilities": ["temperature", "humidity"],
  "last_seen": "2026-07-30T10:29:55Z",
  "created_at": "2026-07-01T08:00:00Z",
  "updated_at": "2026-07-30T10:30:05Z"
}
```

#### DELETE /devices/{id}

Delete a device.

**Request:**
```http
DELETE /devices/temp-001
Authorization: Bearer <access_token>
```

**Response 204:** No content

**Response 404:**
```json
{
  "error": "NotFound",
  "message": "Device not found"
}
```

---

### Sensors

#### GET /sensors

Query sensor readings with time range and filtering.

**Query Parameters:**
- `device_id` (optional): Filter by device
- `sensor_type` (optional): Filter by sensor type (e.g., "temperature")
- `start_time` (optional): ISO 8601 timestamp
- `end_time` (optional): ISO 8601 timestamp
- `limit` (optional): Max results (default: 1000)
- `order` (optional): `asc` or `desc` (default: `desc`)

**Request:**
```http
GET /sensors?device_id=temp-001&sensor_type=temperature&limit=10
Authorization: Bearer <access_token>
```

**Response 200:**
```json
{
  "readings": [
    {
      "id": 12345,
      "device_id": "temp-001",
      "sensor_type": "temperature",
      "value": 25.5,
      "unit": "celsius",
      "timestamp": "2026-07-30T10:30:00Z"
    },
    {
      "id": 12344,
      "device_id": "temp-001",
      "sensor_type": "temperature",
      "value": 25.3,
      "unit": "celsius",
      "timestamp": "2026-07-30T10:29:00Z"
    }
  ],
  "total": 2,
  "limit": 10
}
```

#### GET /sensors/history

Get aggregated historical data (hourly/daily averages).

**Query Parameters:**
- `device_id` (required): Device ID
- `sensor_type` (required): Sensor type
- `start_time` (required): ISO 8601 timestamp
- `end_time` (required): ISO 8601 timestamp
- `aggregation` (optional): `hourly` or `daily` (default: `hourly`)

**Request:**
```http
GET /sensors/history?device_id=temp-001&sensor_type=temperature&start_time=2026-07-29T00:00:00Z&end_time=2026-07-30T23:59:59Z&aggregation=hourly
Authorization: Bearer <access_token>
```

**Response 200:**
```json
{
  "device_id": "temp-001",
  "sensor_type": "temperature",
  "aggregation": "hourly",
  "data": [
    {
      "timestamp": "2026-07-30T10:00:00Z",
      "avg": 25.4,
      "min": 24.8,
      "max": 26.1,
      "count": 60
    },
    {
      "timestamp": "2026-07-30T09:00:00Z",
      "avg": 24.9,
      "min": 24.2,
      "max": 25.6,
      "count": 60
    }
  ]
}
```

---

### Commands

#### POST /commands

Send a command to a device.

**Request:**
```http
POST /commands
Authorization: Bearer <access_token>
Content-Type: application/json

{
  "device_id": "fan-001",
  "command": "set_speed",
  "parameters": {
    "speed": 75
  }
}
```

**Response 202:**
```json
{
  "command_id": "cmd-12345",
  "device_id": "fan-001",
  "command": "set_speed",
  "status": "pending",
  "created_at": "2026-07-30T10:30:00Z"
}
```

**Response 404:**
```json
{
  "error": "NotFound",
  "message": "Device not found or offline"
}
```

#### GET /commands/{id}

Get command execution status.

**Request:**
```http
GET /commands/cmd-12345
Authorization: Bearer <access_token>
```

**Response 200:**
```json
{
  "command_id": "cmd-12345",
  "device_id": "fan-001",
  "command": "set_speed",
  "status": "completed",
  "result": {
    "success": true,
    "message": "Speed set to 75%"
  },
  "created_at": "2026-07-30T10:30:00Z",
  "completed_at": "2026-07-30T10:30:02Z"
}
```

---

### Automation Rules

#### GET /automation/rules

List all automation rules.

**Request:**
```http
GET /automation/rules
Authorization: Bearer <access_token>
```

**Response 200:**
```json
{
  "rules": [
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
        }
      ],
      "created_by": "user-001",
      "created_at": "2026-07-01T08:00:00Z",
      "updated_at": "2026-07-15T12:00:00Z"
    }
  ],
  "total": 1
}
```

#### POST /automation/rules

Create a new automation rule.

**Request:**
```http
POST /automation/rules
Authorization: Bearer <access_token>
Content-Type: application/json

{
  "name": "Low Battery Alert",
  "enabled": true,
  "conditions": [
    {
      "sensor_type": "battery",
      "operator": "<",
      "threshold": 20.0
    }
  ],
  "actions": [
    {
      "device_id": "alert-system",
      "command": "send_notification",
      "parameters": {
        "message": "Device battery below 20%",
        "severity": "warning"
      }
    }
  ]
}
```

**Response 201:**
```json
{
  "id": "rule-002",
  "name": "Low Battery Alert",
  "enabled": true,
  "conditions": [
    {
      "sensor_type": "battery",
      "operator": "<",
      "threshold": 20.0
    }
  ],
  "actions": [
    {
      "device_id": "alert-system",
      "command": "send_notification",
      "parameters": {
        "message": "Device battery below 20%",
        "severity": "warning"
      }
    }
  ],
  "created_by": "user-001",
  "created_at": "2026-07-30T10:30:00Z",
  "updated_at": "2026-07-30T10:30:00Z"
}
```

#### PUT /automation/rules/{id}

Update an automation rule.

**Request:**
```http
PUT /automation/rules/rule-001
Authorization: Bearer <access_token>
Content-Type: application/json

{
  "enabled": false
}
```

**Response 200:** (Updated rule object)

#### DELETE /automation/rules/{id}

Delete an automation rule.

**Request:**
```http
DELETE /automation/rules/rule-001
Authorization: Bearer <access_token>
```

**Response 204:** No content

---

### Metrics

#### GET /metrics

Get system metrics.

**Request:**
```http
GET /metrics
Authorization: Bearer <access_token>
```

**Response 200:**
```json
{
  "system": {
    "cpu_usage_percent": 45.2,
    "memory_usage_bytes": 524288000,
    "uptime_seconds": 86400
  },
  "devices": {
    "total": 150,
    "online": 142,
    "offline": 8
  },
  "network": {
    "active_websocket_clients": 12,
    "mqtt_messages_per_second": 250,
    "api_requests_per_second": 45
  },
  "performance": {
    "avg_api_latency_ms": 12.5,
    "avg_mqtt_latency_ms": 8.3,
    "p95_api_latency_ms": 35.0,
    "p99_api_latency_ms": 120.0
  },
  "errors": {
    "total_errors_last_hour": 3,
    "errors_per_second": 0.0008
  },
  "timestamp": "2026-07-30T10:30:00Z"
}
```

---

## WebSocket API

### Connection

```javascript
const ws = new WebSocket('ws://localhost:8081');

ws.onopen = () => {
  console.log('Connected');
  
  // Authenticate
  ws.send(JSON.stringify({
    type: 'auth',
    token: '<access_token>'
  }));
};
```

### Message Format

All messages are JSON with a `type` field:

```json
{
  "type": "message_type",
  "data": { /* ... */ }
}
```

### Client → Server Messages

#### Authentication

```json
{
  "type": "auth",
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
}
```

**Response:**
```json
{
  "type": "auth_success",
  "data": {
    "user_id": "user-001",
    "role": "Admin"
  }
}
```

#### Subscribe to Sensor Updates

```json
{
  "type": "subscribe",
  "topic": "sensors",
  "filters": {
    "device_id": "temp-001",
    "sensor_type": "temperature"
  }
}
```

**Response:**
```json
{
  "type": "subscribe_success",
  "topic": "sensors"
}
```

#### Unsubscribe

```json
{
  "type": "unsubscribe",
  "topic": "sensors"
}
```

#### Send Command

```json
{
  "type": "command",
  "device_id": "fan-001",
  "command": "turn_on",
  "parameters": {
    "speed": 75
  }
}
```

**Response:**
```json
{
  "type": "command_accepted",
  "command_id": "cmd-12345"
}
```

#### Heartbeat

```json
{
  "type": "ping"
}
```

**Response:**
```json
{
  "type": "pong",
  "timestamp": "2026-07-30T10:30:00Z"
}
```

### Server → Client Messages

#### Sensor Data Update

```json
{
  "type": "sensor_data",
  "data": {
    "device_id": "temp-001",
    "sensor_type": "temperature",
    "value": 25.5,
    "unit": "celsius",
    "timestamp": "2026-07-30T10:30:00Z"
  }
}
```

#### Device Status Change

```json
{
  "type": "device_status",
  "data": {
    "device_id": "temp-001",
    "status": "online",
    "timestamp": "2026-07-30T10:30:00Z"
  }
}
```

#### Alert

```json
{
  "type": "alert",
  "data": {
    "id": 12345,
    "device_id": "temp-001",
    "severity": "warning",
    "message": "Temperature exceeded threshold",
    "timestamp": "2026-07-30T10:30:00Z"
  }
}
```

#### Command Result

```json
{
  "type": "command_result",
  "data": {
    "command_id": "cmd-12345",
    "device_id": "fan-001",
    "status": "completed",
    "result": {
      "success": true,
      "message": "Speed set to 75%"
    },
    "timestamp": "2026-07-30T10:30:02Z"
  }
}
```

#### Error

```json
{
  "type": "error",
  "error": "Unauthorized",
  "message": "Invalid or expired token"
}
```

---

## Error Responses

All error responses follow this format:

```json
{
  "error": "ErrorCode",
  "message": "Human-readable error message",
  "details": {
    "field": "Additional error context"
  },
  "timestamp": "2026-07-30T10:30:00Z"
}
```

### Error Codes

| Code | HTTP Status | Description |
|------|-------------|-------------|
| `BadRequest` | 400 | Invalid request format or parameters |
| `Unauthorized` | 401 | Missing or invalid authentication |
| `Forbidden` | 403 | Insufficient permissions |
| `NotFound` | 404 | Resource not found |
| `Conflict` | 409 | Resource already exists |
| `InternalServerError` | 500 | Unexpected server error |
| `ServiceUnavailable` | 503 | Service temporarily unavailable |

---

## Rate Limiting

- **Default**: 100 requests per minute per IP
- **Authenticated**: 1000 requests per minute per user
- **WebSocket**: 100 messages per second per connection

**Response Headers:**
```
X-RateLimit-Limit: 1000
X-RateLimit-Remaining: 995
X-RateLimit-Reset: 1627646400
```

**Response 429:**
```json
{
  "error": "RateLimitExceeded",
  "message": "Too many requests. Please try again in 60 seconds.",
  "retry_after": 60
}
```

---

## Pagination

List endpoints support pagination:

**Query Parameters:**
- `limit`: Maximum number of results (default: 100, max: 1000)
- `offset`: Number of results to skip (default: 0)

**Response:**
```json
{
  "data": [ /* ... */ ],
  "total": 500,
  "limit": 100,
  "offset": 0,
  "has_next": true
}
```

---

## Versioning

API version is specified in the URL path:

```
http://localhost:8080/v1/devices
```

Current version: **v1**

---

## MQTT Topics (Device Side)

These topics are used by IoT devices communicating with the backend:

### Device → Backend

```
devices/{device_id}/sensors         # Sensor readings
devices/{device_id}/status          # Status updates (online/offline)
devices/{device_id}/heartbeat       # Keep-alive
devices/{device_id}/logs            # Device logs
```

### Backend → Device

```
devices/{device_id}/commands        # Commands to execute
devices/{device_id}/config          # Configuration updates
devices/{device_id}/firmware        # OTA firmware updates
```

### Message Formats

#### Sensor Reading
```json
{
  "sensor_type": "temperature",
  "value": 25.5,
  "unit": "celsius",
  "timestamp": "2026-07-30T10:30:00Z"
}
```

#### Status Update
```json
{
  "status": "online",
  "firmware_version": "v1.2.3",
  "battery_level": 85.5,
  "ip_address": "192.168.1.100"
}
```

#### Command
```json
{
  "command_id": "cmd-12345",
  "command": "set_speed",
  "parameters": {
    "speed": 75
  }
}
```

---

## Examples

### Full Device Lifecycle (HTTP)

```bash
# 1. Login
curl -X POST http://localhost:8080/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"password"}'

# 2. Register device
TOKEN="eyJhbGc..."
curl -X POST http://localhost:8080/devices \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"id":"temp-001","name":"Temp Sensor","type":"temperature"}'

# 3. List devices
curl http://localhost:8080/devices \
  -H "Authorization: Bearer $TOKEN"

# 4. Query sensors
curl "http://localhost:8080/sensors?device_id=temp-001&limit=10" \
  -H "Authorization: Bearer $TOKEN"

# 5. Send command
curl -X POST http://localhost:8080/commands \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"device_id":"fan-001","command":"turn_on","parameters":{"speed":75}}'

# 6. Delete device
curl -X DELETE http://localhost:8080/devices/temp-001 \
  -H "Authorization: Bearer $TOKEN"
```

### WebSocket Real-time Updates (JavaScript)

```javascript
const ws = new WebSocket('ws://localhost:8081');
const token = 'eyJhbGc...';

ws.onopen = () => {
  // Authenticate
  ws.send(JSON.stringify({ type: 'auth', token }));
  
  // Subscribe to sensor updates
  ws.send(JSON.stringify({
    type: 'subscribe',
    topic: 'sensors'
  }));
};

ws.onmessage = (event) => {
  const message = JSON.parse(event.data);
  
  switch (message.type) {
    case 'sensor_data':
      console.log('New sensor reading:', message.data);
      updateChart(message.data);
      break;
      
    case 'device_status':
      console.log('Device status changed:', message.data);
      updateDeviceStatus(message.data);
      break;
      
    case 'alert':
      console.log('Alert received:', message.data);
      showNotification(message.data);
      break;
  }
};

// Heartbeat every 30 seconds
setInterval(() => {
  ws.send(JSON.stringify({ type: 'ping' }));
}, 30000);
```

---

**See Also**: [ARCHITECTURE.md](ARCHITECTURE.md) for system design details.
