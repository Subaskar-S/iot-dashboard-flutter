import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../../core/constants/app_constants.dart';

final sharedPreferencesProvider = FutureProvider<SharedPreferences>(
  (_) => SharedPreferences.getInstance(),
);

// ── Mutable config notifiers (replaces StateProvider from Riverpod v2) ────────

class StringNotifier extends Notifier<String> {
  StringNotifier(this._initial);
  final String _initial;

  @override
  String build() => _initial;

  void set(String value) => state = value;
}

class BoolNotifier extends Notifier<bool> {
  BoolNotifier(this._initial);
  final bool _initial;

  @override
  bool build() => _initial;

  void set(bool value) => state = value;
}

/// Base URL for REST API calls.
final serverBaseUrlProvider = NotifierProvider<StringNotifier, String>(
  () => StringNotifier(AppConstants.defaultBaseUrl),
);

/// WebSocket URL.
final serverWsUrlProvider = NotifierProvider<StringNotifier, String>(
  () => StringNotifier(AppConstants.defaultWsUrl),
);

/// Whether dark mode is enabled.
final darkModeProvider = NotifierProvider<BoolNotifier, bool>(
  () => BoolNotifier(false),
);
