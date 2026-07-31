/// REST API endpoint paths.
class ApiEndpoints {
  ApiEndpoints._();

  static const String health = '/health';
  static const String login = '/auth/login';
  static const String refresh = '/auth/refresh';
  static const String logout = '/auth/logout';
  static const String devices = '/devices';
  static const String commands = '/commands';
  static const String sensors = '/sensors';
  static const String automationRules = '/automation/rules';
  static const String metrics = '/metrics';
}
