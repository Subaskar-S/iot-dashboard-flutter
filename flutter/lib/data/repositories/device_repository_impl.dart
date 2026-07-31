import 'package:dio/dio.dart';
import '../../core/constants/api_endpoints.dart';
import '../../core/utils/result.dart';
import '../../domain/entities/device.dart';
import '../../domain/repositories/device_repository.dart';
import '../datasources/api_client.dart';

class DeviceRepositoryImpl implements DeviceRepository {
  DeviceRepositoryImpl({required this.dio});
  final Dio dio;

  @override
  Future<Result<List<Device>>> getDevices({
    bool? onlineOnly,
    String? type,
  }) async {
    try {
      final response = await dio.get<Map<String, dynamic>>(
        ApiEndpoints.devices,
        queryParameters: {
          if (onlineOnly == true) 'online': 'true',
          if (type != null) 'type': type,
        },
      );

      final items = (response.data!['devices'] as List)
          .cast<Map<String, dynamic>>()
          .map(_fromJson)
          .toList();

      return Success(items);
    } on DioException catch (e) {
      return Failure(e.readableMessage, statusCode: e.response?.statusCode);
    }
  }

  @override
  Future<Result<Device>> registerDevice({
    required String id,
    required String name,
    required String type,
    String protocol = 'mqtt',
    String address = '',
  }) async {
    try {
      final response = await dio.post<Map<String, dynamic>>(
        ApiEndpoints.devices,
        data: {
          'id': id,
          'name': name,
          'type': type,
          'protocol': protocol,
          'address': address,
        },
      );
      return Success(_fromJson(response.data!));
    } on DioException catch (e) {
      return Failure(e.readableMessage, statusCode: e.response?.statusCode);
    }
  }

  @override
  Future<Result<void>> unregisterDevice(String id) async {
    try {
      await dio.delete<void>(ApiEndpoints.devices, queryParameters: {'id': id});
      return const Success(null);
    } on DioException catch (e) {
      return Failure(e.readableMessage, statusCode: e.response?.statusCode);
    }
  }

  @override
  Future<Result<void>> sendCommand({
    required String deviceId,
    required String command,
    Map<String, dynamic> parameters = const {},
  }) async {
    try {
      await dio.post<void>(ApiEndpoints.commands, data: {
        'device_id': deviceId,
        'command': command,
        'parameters': parameters,
      });
      return const Success(null);
    } on DioException catch (e) {
      return Failure(e.readableMessage, statusCode: e.response?.statusCode);
    }
  }

  static Device _fromJson(Map<String, dynamic> j) => Device(
        id: j['id'] as String,
        name: j['name'] as String,
        type: j['type'] as String,
        protocol: j['protocol'] as String? ?? 'mqtt',
        address: j['address'] as String? ?? '',
        online: j['online'] as bool? ?? false,
      );
}
