import '../entities/device.dart';
import '../../core/utils/result.dart';

abstract interface class DeviceRepository {
  Future<Result<List<Device>>> getDevices({bool? onlineOnly, String? type});
  Future<Result<Device>> registerDevice({
    required String id,
    required String name,
    required String type,
    String protocol = 'mqtt',
    String address = '',
  });
  Future<Result<void>> unregisterDevice(String id);
  Future<Result<void>> sendCommand({
    required String deviceId,
    required String command,
    Map<String, dynamic> parameters = const {},
  });
}
