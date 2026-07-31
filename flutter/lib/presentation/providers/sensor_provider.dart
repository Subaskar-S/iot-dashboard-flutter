import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../data/repositories/sensor_repository_impl.dart';
import '../../core/utils/result.dart';
import '../../domain/entities/sensor_reading.dart';
import '../../domain/repositories/sensor_repository.dart';
import 'device_provider.dart';
import 'websocket_provider.dart';

final sensorRepositoryProvider =
    Provider<SensorRepository>((ref) => SensorRepositoryImpl(
          dio: ref.watch(dioProvider),
          wsClient: ref.watch(webSocketClientProvider),
        ));

// ── Historical readings for a device ─────────────────────────────────────────

final sensorHistoryProvider = FutureProvider.autoDispose
    .family<List<SensorReading>, ({String deviceId, String? sensorType})>(
  (ref, params) async {
    final result = await ref.read(sensorRepositoryProvider).getSensorReadings(
          deviceId: params.deviceId,
          sensorType: params.sensorType,
        );
    return result.fold((list) => list, (err) => throw Exception(err));
  },
);

// ── Live sensor stream for a device ──────────────────────────────────────────

final liveSensorProvider =
    StreamProvider.autoDispose.family<SensorReading, String>((ref, deviceId) {
  return ref
      .watch(sensorRepositoryProvider)
      .watchSensorReadings(deviceId: deviceId);
});
