/// Application-wide constants.
class AppConstants {
  AppConstants._();

  // API
  //
  // Baked in at build time so release builds ship pointing at the real server:
  //
  //   flutter build appbundle --release \
  //     --dart-define=API_BASE_URL=https://api.example.com \
  //     --dart-define=WS_URL=wss://api.example.com/ws
  //
  // Release builds must use https:// and wss:// — Android blocks cleartext, and
  // the localhost defaults below resolve to the phone itself. The URLs remain
  // overridable at runtime from the Settings page.
  static const String defaultBaseUrl = String.fromEnvironment(
    'API_BASE_URL',
    defaultValue: 'http://localhost:8080',
  );
  static const String defaultWsUrl = String.fromEnvironment(
    'WS_URL',
    defaultValue: 'ws://localhost:8081',
  );
  static const int connectTimeoutMs = 10000;
  static const int receiveTimeoutMs = 30000;

  // Auth
  static const String accessTokenKey = 'access_token';
  static const String refreshTokenKey = 'refresh_token';

  // Persisted settings (SharedPreferences)
  static const String baseUrlPrefKey = 'server_base_url';
  static const String wsUrlPrefKey = 'server_ws_url';
  static const String darkModePrefKey = 'dark_mode';

  // Heartbeat
  static const int wsHeartbeatIntervalSeconds = 30;
  static const int wsReconnectDelaySeconds = 3;

  // Pagination
  static const int defaultPageSize = 50;

  // Chart
  static const int chartMaxDataPoints = 60;
  static const int sensorRefreshIntervalSeconds = 5;
}
