# Deployment Guide

## Overview

The IoT Dashboard backend is a single self-contained binary (`iot-dashboard`) that serves HTTP REST, WebSocket, and connects to an external MQTT broker. It requires only SQLite (embedded) and OpenSSL at runtime.

---

## Local / Development

```bash
mkdir -p build/OSX/Debug && cd build/OSX/Debug
cmake ../../.. -G Ninja -DCMAKE_BUILD_TYPE=Debug
ninja

# Start Mosquitto
brew services start mosquitto   # macOS
sudo systemctl start mosquitto  # Linux

# Start backend
./src/iot-dashboard --serve --db ./iot.db

# Start Flutter
cd flutter && flutter run -d macos
```

---

## Production Build

```bash
mkdir -p build/Linux/Release && cd build/Linux/Release
cmake ../../.. -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja

# Binary is at:
./src/iot-dashboard
```

---

## Configuration

All settings are passed via CLI flags. Key production flags:

```bash
./iot-dashboard \
  --serve \
  --db /var/lib/iot-dashboard/iot.db \
  --mqtt-broker tcp://your-broker:1883 \
  --port 8080 \
  --ws-port 8081 \
  --log-level info
```

> **Security note**: Change the default JWT secret by setting it in `AppConfig::m_jwtSecret` and recompiling, or add an `--jwt-secret` CLI flag as your first customization.

---

## Docker

### Dockerfile

```dockerfile
FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y \
    cmake ninja-build g++ libboost-all-dev \
    nlohmann-json3-dev libspdlog-dev libfmt-dev \
    libssl-dev libsqlite3-dev libpaho-mqtt-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN mkdir -p build/Linux/Release && cd build/Linux/Release && \
    cmake ../../.. -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF && \
    ninja

# ── Runtime image ──────────────────────────────────────────────────────────
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    libboost-system1.83.0 libssl3 libsqlite3-0 \
    libpaho-mqtt3as \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /src/build/Linux/Release/src/iot-dashboard .

RUN mkdir -p /var/lib/iot-dashboard

EXPOSE 8080 8081

CMD ["./iot-dashboard", \
     "--serve", \
     "--db", "/var/lib/iot-dashboard/iot.db", \
     "--mqtt-broker", "tcp://mosquitto:1883", \
     "--log-level", "info"]
```

### docker-compose.yml

```yaml
version: "3.9"

services:
  mosquitto:
    image: eclipse-mosquitto:2
    volumes:
      - ./mosquitto.conf:/mosquitto/config/mosquitto.conf
      - mosquitto-data:/mosquitto/data
    ports:
      - "1883:1883"

  backend:
    build: .
    depends_on:
      - mosquitto
    environment:
      - MQTT_BROKER=tcp://mosquitto:1883
    ports:
      - "8080:8080"
      - "8081:8081"
    volumes:
      - db-data:/var/lib/iot-dashboard

volumes:
  mosquitto-data:
  db-data:
```

**mosquitto.conf:**
```
listener 1883
allow_anonymous true
persistence true
persistence_location /mosquitto/data/
```

```bash
docker compose up -d
curl http://localhost:8080/health
```

---

## systemd (Linux)

```ini
# /etc/systemd/system/iot-dashboard.service
[Unit]
Description=IoT Dashboard Backend
After=network.target mosquitto.service
Requires=mosquitto.service

[Service]
Type=simple
User=iotdash
ExecStart=/usr/local/bin/iot-dashboard \
    --serve \
    --db /var/lib/iot-dashboard/iot.db \
    --mqtt-broker tcp://localhost:1883 \
    --log-level info
Restart=on-failure
RestartSec=5s
WorkingDirectory=/var/lib/iot-dashboard

# Security hardening
NoNewPrivileges=yes
ProtectSystem=strict
ReadWritePaths=/var/lib/iot-dashboard
PrivateTmp=yes

[Install]
WantedBy=multi-user.target
```

```bash
sudo useradd -r -s /bin/false iotdash
sudo mkdir -p /var/lib/iot-dashboard && sudo chown iotdash: /var/lib/iot-dashboard
sudo cp build/Linux/Release/src/iot-dashboard /usr/local/bin/
sudo systemctl enable --now iot-dashboard
sudo journalctl -u iot-dashboard -f
```

---

## Reverse Proxy (nginx)

Serve both HTTP and WebSocket behind a single domain:

```nginx
server {
    listen 443 ssl http2;
    server_name dashboard.example.com;

    ssl_certificate     /etc/letsencrypt/live/dashboard.example.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/dashboard.example.com/privkey.pem;

    # REST API
    location /api/ {
        proxy_pass         http://127.0.0.1:8080/;
        proxy_set_header   Host $host;
        proxy_set_header   X-Real-IP $remote_addr;
    }

    # WebSocket
    location /ws {
        proxy_pass         http://127.0.0.1:8081;
        proxy_http_version 1.1;
        proxy_set_header   Upgrade $http_upgrade;
        proxy_set_header   Connection "upgrade";
        proxy_read_timeout 86400s;
    }

    # Flutter web app
    location / {
        root   /var/www/iot-dashboard/web;
        try_files $uri $uri/ /index.html;
    }
}
```

---

## Flutter Deployment

### macOS App

```bash
cd flutter
flutter build macos --release
# Output: build/macos/Build/Products/Release/iot_dashboard.app
```

### Android APK / AAB

```bash
flutter build apk --release
flutter build appbundle --release
```

### iOS

```bash
flutter build ios --release
# Then open Xcode → archive → distribute
```

### Web

```bash
flutter build web --release
# Output: build/web/  → deploy to nginx / S3 / Firebase Hosting
```

### Update backend URL for production

In `flutter/lib/core/constants/app_constants.dart`:

```dart
static const String defaultBaseUrl = 'https://dashboard.example.com/api';
static const String defaultWsUrl   = 'wss://dashboard.example.com/ws';
```

Users can also change the URL at runtime via **Settings → Server**.

---

## MQTT Broker Options

| Broker | Notes |
|--------|-------|
| Mosquitto | Lightweight, best for single-host |
| EMQX | Clustered, built-in dashboard |
| HiveMQ | Enterprise, cloud-managed |
| AWS IoT Core | Managed, IAM auth |

---

## Environment Checklist

- [ ] MQTT broker reachable from backend host
- [ ] Ports 8080 (HTTP) and 8081 (WebSocket) open in firewall
- [ ] SQLite database directory writable by service user
- [ ] JWT secret changed from default (`admin123` password changed)
- [ ] TLS certificates configured for HTTPS/WSS in production
- [ ] Log rotation configured (`/var/log/iot-dashboard/` or journald)
- [ ] Database backup scheduled (`sqlite3 iot.db ".backup backup.db"`)

---

## Health Check Endpoint

Used by load balancers and orchestration systems:

```bash
curl http://localhost:8080/health
# {"status":"healthy","version":"1.0.0","uptime_seconds":3600}
```

Returns `200 OK` when the server is ready. No authentication required.
