import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:dio/dio.dart';
import '../../data/datasources/api_client.dart';
import '../../data/repositories/device_repository_impl.dart';
import '../../domain/entities/device.dart';
import '../../domain/repositories/device_repository.dart';
import '../../core/utils/result.dart';
import 'auth_provider.dart';
import 'config_provider.dart';
import 'websocket_provider.dart';

// ── Repository ────────────────────────────────────────────────────────────────

final dioProvider = Provider<Dio>((ref) {
  final baseUrl = ref.watch(serverBaseUrlProvider);
  final token = ref.watch(authNotifierProvider).token;
  return createDio(baseUrl: baseUrl, accessToken: token);
});

final deviceRepositoryProvider = Provider<DeviceRepository>(
    (ref) => DeviceRepositoryImpl(dio: ref.watch(dioProvider)));

// ── Devices list ─────────────────────────────────────────────────────────────

final devicesProvider =
    AsyncNotifierProvider<DevicesNotifier, List<Device>>(DevicesNotifier.new);

class DevicesNotifier extends AsyncNotifier<List<Device>> {
  @override
  Future<List<Device>> build() => _fetch();

  Future<List<Device>> _fetch() async {
    final result = await ref.read(deviceRepositoryProvider).getDevices();
    return result.fold((list) => list, (err) => throw Exception(err));
  }

  Future<void> refresh() async {
    state = const AsyncLoading();
    state = await AsyncValue.guard(_fetch);
  }

  Future<Result<Device>> register({
    required String id,
    required String name,
    required String type,
    String protocol = 'mqtt',
    String address = '',
  }) async {
    final result = await ref.read(deviceRepositoryProvider).registerDevice(
          id: id,
          name: name,
          type: type,
          protocol: protocol,
          address: address,
        );
    if (result.isSuccess) await refresh();
    return result;
  }

  Future<Result<void>> unregister(String id) async {
    final result =
        await ref.read(deviceRepositoryProvider).unregisterDevice(id);
    if (result.isSuccess) await refresh();
    return result;
  }
}

// ── Live device status from WebSocket ────────────────────────────────────────

final deviceStatusStreamProvider =
    StreamProvider.autoDispose<Map<String, bool>>((ref) {
  final ws = ref.watch(webSocketClientProvider);
  return ws.messages.where((m) => m['type'] == 'device_status').map((m) {
    final data = m['data'] as Map<String, dynamic>;
    return {data['device_id'] as String: data['online'] as bool? ?? false};
  });
});
