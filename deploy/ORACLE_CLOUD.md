# Deploying the Backend to Oracle Cloud (Always Free)

Target: one Always Free Ampere A1 VM running three containers — the backend,
Mosquitto, and Caddy. Caddy terminates TLS and gives the Flutter app a single
HTTPS origin, which is what the Android release build requires (Android blocks
cleartext traffic, and the backend has no TLS of its own).

```
Play Store app
  │  https://api.example.com        → Caddy :443 → backend :8080   (REST)
  │  wss://api.example.com/ws       → Caddy :443 → backend :8081   (WebSocket)
  ▼
Caddy (automatic Let's Encrypt certificates)
  ├── iot-dashboard   named volume for iot.db
  └── mosquitto       internal network only, 1883 not published
```

## What you need first

- An Oracle Cloud account (credit card required for identity verification; the
  Always Free tier is not charged).
- A domain name you control. Caddy needs one to issue a certificate — an IP
  address cannot get a public TLS cert. A free DuckDNS subdomain works.

---

## 1. Create the VM

Console → **Compute → Instances → Create instance**

| Setting | Value |
|---|---|
| Image | Canonical Ubuntu 24.04 |
| Shape | `VM.Standard.A1.Flex` (Ampere ARM) |
| OCPUs / Memory | 4 / 24 GB — the full Always Free allocation |
| SSH keys | Upload your public key |

Save the assigned **public IP**.

> **Capacity note:** `VM.Standard.A1.Flex` is frequently out of stock in busy
> regions, and you will get `Out of host capacity`. Retry, or pick a less busy
> home region. This is the most common thing that stalls an Oracle free-tier
> setup.

The A1 shape is ARM (aarch64). The Dockerfile builds natively on it — no
cross-compilation or `--platform` flag needed.

## 2. Open ports 80 and 443

This takes **two** steps on Oracle. Missing the second one is the classic
gotcha — the security list looks correct but connections still time out.

**a. VCN security list.** Networking → Virtual Cloud Networks → your VCN →
Subnet → Security List → **Add Ingress Rules**:

| Source CIDR | Protocol | Destination port |
|---|---|---|
| `0.0.0.0/0` | TCP | 80 |
| `0.0.0.0/0` | TCP | 443 |

**b. The instance firewall.** Oracle's Ubuntu images ship iptables rules that
drop everything except SSH. SSH in and run:

```bash
sudo iptables -I INPUT 6 -m state --state NEW -p tcp --dport 80 -j ACCEPT
sudo iptables -I INPUT 6 -m state --state NEW -p tcp --dport 443 -j ACCEPT
sudo netfilter-persistent save
```

Do **not** open 1883 (MQTT). The broker runs on the internal Docker network
only. If real ESP32/Raspberry Pi devices must reach it from outside, enable
authentication in `deploy/mosquitto.conf` first — see the comments in that file.

## 3. Point DNS at the VM

Create an **A record** for `api.example.com` → your public IP. Verify before
continuing, because Caddy's certificate request will fail if DNS has not
propagated:

```bash
dig +short api.example.com
```

## 4. Install Docker

```bash
sudo apt-get update
sudo apt-get install -y docker.io docker-compose-v2 git
sudo usermod -aG docker $USER
newgrp docker
```

## 5. Configure and start the stack

```bash
git clone https://github.com/Subaskar-S/iot-dashboard-flutter.git
cd iot-dashboard-flutter

cp .env.example .env
```

Edit `.env`:

```bash
DOMAIN=api.example.com
ACME_EMAIL=you@example.com
IOT_JWT_SECRET=<paste output of: openssl rand -base64 48>
IOT_ADMIN_USER=admin
IOT_ADMIN_PASSWORD=<a long random password>
```

`docker compose` refuses to start if `IOT_JWT_SECRET` or `IOT_ADMIN_PASSWORD`
is empty, so these cannot be forgotten by accident.

```bash
docker compose up -d --build
docker compose logs -f backend
```

The first build compiles the whole C++ backend and takes several minutes.

## 6. Verify

```bash
# Health check over HTTPS, from your laptop
curl https://api.example.com/health

# Login returns a JWT pair
curl -X POST https://api.example.com/auth/login \
  -d '{"username":"admin","password":"<your password>"}'
```

A `200` from both means the backend, Caddy, and the certificate are all working.
If `/health` hangs, it is almost always step 2b (the instance iptables rules).

## 7. Point the Flutter app at it

In [`flutter/lib/core/constants/app_constants.dart`](../flutter/lib/core/constants/app_constants.dart):

```dart
static const String defaultBaseUrl = 'https://api.example.com';
static const String defaultWsUrl = 'wss://api.example.com/ws';
```

`https` and `wss` are mandatory for the Play Store build. With these, you do not
need a `usesCleartextTraffic` exception or a network security config.

---

## Operating it

```bash
docker compose logs -f backend     # follow logs
docker compose restart backend     # restart
docker compose down                # stop (named volumes are preserved)
git pull && docker compose up -d --build   # deploy an update
```

Back up the SQLite database:

```bash
docker compose exec backend sqlite3 /var/lib/iot-dashboard/iot.db ".backup '/tmp/b.db'"
docker compose cp backend:/tmp/b.db ./iot-backup-$(date +%F).db
```

## Known limitations of this deployment

These are properties of the current backend, not of the hosting setup:

- **Users are held in memory**, not in SQLite
  ([`authentication_service.hpp`](../src/security/authentication_service.hpp)).
  The admin account is re-seeded from `IOT_ADMIN_PASSWORD` on every restart, and
  any user created at runtime is lost on restart. Changing the password means
  editing `.env` and restarting.
- **No rate limiting on `/auth/login`.** The endpoint is exposed to the
  internet, so use a long random admin password. PBKDF2 at 600k iterations makes
  each attempt expensive, which helps, but is not a substitute.
- **Single instance only.** SQLite plus in-memory state means you cannot run
  two replicas behind a load balancer.
