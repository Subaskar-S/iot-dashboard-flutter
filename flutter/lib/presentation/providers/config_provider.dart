import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../../core/constants/app_constants.dart';

/// Backing store for the settings below. Populated by [initConfigStorage]
/// before `runApp`, which lets the notifiers read and write synchronously.
/// Stays null in widget tests that never call it, in which case the settings
/// simply fall back to their defaults instead of throwing.
SharedPreferences? _prefs;

/// Loads persisted settings. Call once from `main()` before `runApp`.
Future<void> initConfigStorage() async {
  _prefs = await SharedPreferences.getInstance();
}

// ── Mutable config notifiers (replaces StateProvider from Riverpod v2) ────────

class StringNotifier extends Notifier<String> {
  StringNotifier(this._prefKey, this._fallback);
  final String _prefKey;
  final String _fallback;

  @override
  String build() => _prefs?.getString(_prefKey) ?? _fallback;

  void set(String value) {
    state = value;
    _prefs?.setString(_prefKey, value);
  }
}

class BoolNotifier extends Notifier<bool> {
  BoolNotifier(this._prefKey, this._fallback);
  final String _prefKey;
  final bool _fallback;

  @override
  bool build() => _prefs?.getBool(_prefKey) ?? _fallback;

  void set(bool value) {
    state = value;
    _prefs?.setBool(_prefKey, value);
  }
}

/// Base URL for REST API calls.
final serverBaseUrlProvider = NotifierProvider<StringNotifier, String>(
  () =>
      StringNotifier(AppConstants.baseUrlPrefKey, AppConstants.defaultBaseUrl),
);

/// WebSocket URL.
final serverWsUrlProvider = NotifierProvider<StringNotifier, String>(
  () => StringNotifier(AppConstants.wsUrlPrefKey, AppConstants.defaultWsUrl),
);

/// Whether dark mode is enabled.
final darkModeProvider = NotifierProvider<BoolNotifier, bool>(
  () => BoolNotifier(AppConstants.darkModePrefKey, false),
);
