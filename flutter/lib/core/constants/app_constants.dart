/// Application-wide constants.
class AppConstants {
  AppConstants._();

  // API
  static const String defaultBaseUrl = 'http://localhost:8080';
  static const String defaultWsUrl = 'ws://localhost:8081';
  static const int connectTimeoutMs = 10000;
  static const int receiveTimeoutMs = 30000;

  // Auth
  static const String accessTokenKey = 'access_token';
  static const String refreshTokenKey = 'refresh_token';

  // Heartbeat
  static const int wsHeartbeatIntervalSeconds = 30;
  static const int wsReconnectDelaySeconds = 3;

  // Pagination
  static const int defaultPageSize = 50;

  // Chart
  static const int chartMaxDataPoints = 60;
  static const int sensorRefreshIntervalSeconds = 5;
}
