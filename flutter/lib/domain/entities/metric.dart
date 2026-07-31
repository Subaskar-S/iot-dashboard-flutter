import 'package:flutter/foundation.dart';

@immutable
class SystemMetrics {
  const SystemMetrics({
    required this.uptimeSeconds,
    required this.activeWsClients,
    required this.httpRequestsTotal,
    required this.mqttMessagesTotal,
  });

  final int uptimeSeconds;
  final int activeWsClients;
  final int httpRequestsTotal;
  final int mqttMessagesTotal;

  String get uptimeFormatted {
    final h = uptimeSeconds ~/ 3600;
    final m = (uptimeSeconds % 3600) ~/ 60;
    final s = uptimeSeconds % 60;
    return '${h}h ${m}m ${s}s';
  }
}
