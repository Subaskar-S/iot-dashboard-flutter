import 'package:dio/dio.dart';
import '../../core/constants/api_endpoints.dart';
import '../../core/utils/result.dart';
import '../../domain/entities/sensor_reading.dart';
import '../../domain/repositories/sensor_repository.dart';
import '../datasources/api_client.dart';
import '../datasources/websocket_client.dart';

class SensorRepositoryImpl implements SensorRepository {
  SensorRepositoryImpl({required this.dio, required this.wsClient});
  final Dio dio;
  final WebSocketClient wsClient;

  @override
  Future<Result<List<SensorReading>>> getSensorReadings({
    required String deviceId,
    String? sensorType,
    int limit = 60,
  }) async {
    try {
      final response = await dio.get<Map<String, dynamic>>(
        ApiEndpoints.sensors,
        queryParameters: {
          'device_id': deviceId,
          if (sensorType != null) 'sensor_type': sensorType,
          'limit': limit,
        },
      );

      final items = (response.data!['readings'] as List)
          .cast<Map<String, dynamic>>()
          .map(_fromJson)
          .toList();

      return Success(items);
    } on DioException catch (e) {
      return Failure(e.readableMessage, statusCode: e.response?.statusCode);
    }
  }

  @override
  Stream<SensorReading> watchSensorReadings({String? deviceId}) {
    wsClient.subscribe(
      deviceId != null ? 'sensors/$deviceId' : 'sensors',
    );

    return wsClient.messages
        .where((m) => m['type'] == 'sensor_data')
        .map((m) => _fromWsJson(m['data'] as Map<String, dynamic>))
        .where((r) => deviceId == null || r.deviceId == deviceId);
  }

  static SensorReading _fromJson(Map<String, dynamic> j) => SensorReading(
        deviceId: j['device_id'] as String,
        sensorType: j['sensor_type'] as String,
        value: (j['value'] as num).toDouble(),
        unit: j['unit'] as String? ?? '',
        timestamp: DateTime.fromMillisecondsSinceEpoch(
          ((j['timestamp'] as num) * 1000).toInt(),
        ),
        quality: j['quality'] as String?,
      );

  static SensorReading _fromWsJson(Map<String, dynamic> j) => SensorReading(
        deviceId: j['device_id'] as String,
        sensorType: j['sensor_type'] as String,
        value: (j['value'] as num).toDouble(),
        unit: j['unit'] as String? ?? '',
        timestamp: DateTime.now(),
      );
}
