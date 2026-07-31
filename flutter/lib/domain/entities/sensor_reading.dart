import 'package:flutter/foundation.dart';

@immutable
class SensorReading {
  const SensorReading({
    required this.deviceId,
    required this.sensorType,
    required this.value,
    required this.unit,
    required this.timestamp,
    this.quality,
  });

  final String deviceId;
  final String sensorType;
  final double value;
  final String unit;
  final DateTime timestamp;
  final String? quality;

  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      other is SensorReading &&
          other.deviceId == deviceId &&
          other.sensorType == sensorType &&
          other.timestamp == timestamp;

  @override
  int get hashCode => Object.hash(deviceId, sensorType, timestamp);
}
