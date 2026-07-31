import '../entities/sensor_reading.dart';
import '../../core/utils/result.dart';

abstract interface class SensorRepository {
  Future<Result<List<SensorReading>>> getSensorReadings({
    required String deviceId,
    String? sensorType,
    int limit = 60,
  });

  /// Returns a stream of live sensor updates from the WebSocket.
  Stream<SensorReading> watchSensorReadings({String? deviceId});
}
