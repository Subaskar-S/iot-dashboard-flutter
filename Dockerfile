# ── Build stage ─────────────────────────────────────────────────────────────
FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
        cmake ninja-build g++ pkg-config \
        libboost-all-dev nlohmann-json3-dev libspdlog-dev libfmt-dev \
        libssl-dev libsqlite3-dev libpaho-mqtt-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY CMakeLists.txt ./
COPY cmake ./cmake
COPY src ./src

# BUILD_TESTING=OFF so GTest is not required in the image.
RUN cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTING=OFF \
    && cmake --build build

# ── Runtime stage ───────────────────────────────────────────────────────────
FROM ubuntu:24.04

# Runtime shared libraries only. Versions are resolved by apt rather than
# pinned, so this keeps working across Ubuntu 24.04 point releases.
RUN apt-get update && apt-get install -y --no-install-recommends \
        libboost-thread1.83.0 libssl3 libsqlite3-0 libpaho-mqtt1.3 \
        libspdlog1.12 libfmt9 ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Run unprivileged; the volume is chowned to this uid in docker-compose.
RUN useradd --system --uid 10001 --create-home iot

WORKDIR /app
COPY --from=builder /src/build/src/iot-dashboard /usr/local/bin/iot-dashboard

RUN mkdir -p /var/lib/iot-dashboard && chown -R iot:iot /var/lib/iot-dashboard
USER iot

EXPOSE 8080 8081

ENTRYPOINT ["/usr/local/bin/iot-dashboard"]
CMD ["--serve", \
     "--db", "/var/lib/iot-dashboard/iot.db", \
     "--mqtt-broker", "tcp://mosquitto:1883", \
     "--log-level", "info"]
