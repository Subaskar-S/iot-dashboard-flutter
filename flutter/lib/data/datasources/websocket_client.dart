import 'dart:async';
import 'dart:convert';
import 'package:web_socket_channel/web_socket_channel.dart';
import '../../core/constants/app_constants.dart';

/// Manages a single WebSocket connection to the backend.
/// Handles ping/pong heartbeat and reconnection automatically.
class WebSocketClient {
  WebSocketClient({String? wsUrl})
      : _wsUrl = wsUrl ?? AppConstants.defaultWsUrl;

  final String _wsUrl;
  WebSocketChannel? _channel;
  StreamSubscription? _pingTimer;
  final _controller = StreamController<Map<String, dynamic>>.broadcast();
  bool _disposed = false;
  Timer? _reconnectTimer;

  Stream<Map<String, dynamic>> get messages => _controller.stream;

  void connect() {
    _reconnectTimer?.cancel();
    try {
      _channel = WebSocketChannel.connect(Uri.parse(_wsUrl));
      _channel!.stream.listen(
        (data) {
          if (data is String) {
            final json = jsonDecode(data) as Map<String, dynamic>;
            _controller.add(json);
          }
        },
        onError: (_) => _scheduleReconnect(),
        onDone: () => _scheduleReconnect(),
      );
      _startHeartbeat();
    } catch (_) {
      _scheduleReconnect();
    }
  }

  void send(Map<String, dynamic> message) {
    _channel?.sink.add(jsonEncode(message));
  }

  void subscribe(String topic) => send({'type': 'subscribe', 'topic': topic});
  void unsubscribe(String topic) =>
      send({'type': 'unsubscribe', 'topic': topic});

  void _startHeartbeat() {
    _pingTimer?.cancel();
    _pingTimer = Stream.periodic(
      const Duration(seconds: AppConstants.wsHeartbeatIntervalSeconds),
    ).listen((_) => send({'type': 'ping'}));
  }

  void _scheduleReconnect() {
    if (_disposed) return;
    _pingTimer?.cancel();
    _reconnectTimer = Timer(
      const Duration(seconds: AppConstants.wsReconnectDelaySeconds),
      connect,
    );
  }

  void dispose() {
    _disposed = true;
    _pingTimer?.cancel();
    _reconnectTimer?.cancel();
    _channel?.sink.close();
    _controller.close();
  }
}
