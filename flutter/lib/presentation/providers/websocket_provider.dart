import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../data/datasources/websocket_client.dart';
import 'config_provider.dart';

final webSocketClientProvider = Provider<WebSocketClient>((ref) {
  final wsUrl = ref.watch(serverWsUrlProvider);
  final client = WebSocketClient(wsUrl: wsUrl);
  client.connect();
  ref.onDispose(client.dispose);
  return client;
});
