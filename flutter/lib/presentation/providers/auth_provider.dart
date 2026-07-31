import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_secure_storage/flutter_secure_storage.dart';
import '../../data/repositories/auth_repository_impl.dart';
import '../../domain/repositories/auth_repository.dart';
import '../../core/utils/result.dart';
import 'config_provider.dart';

// ── Infrastructure providers ─────────────────────────────────────────────────

final secureStorageProvider =
    Provider<FlutterSecureStorage>((_) => const FlutterSecureStorage());

final authRepositoryProvider = Provider<AuthRepository>((ref) {
  final baseUrl = ref.watch(serverBaseUrlProvider);
  return AuthRepositoryImpl(
    storage: ref.watch(secureStorageProvider),
    baseUrl: baseUrl,
  );
});

// ── Auth state ────────────────────────────────────────────────────────────────

enum AuthStatus { initial, checking, authenticated, unauthenticated }

class AuthState {
  const AuthState({
    this.status = AuthStatus.initial,
    this.token,
    this.error,
  });

  final AuthStatus status;
  final String? token;
  final String? error;

  bool get isAuthenticated => status == AuthStatus.authenticated;

  AuthState copyWith({
    AuthStatus? status,
    String? token,
    String? error,
  }) =>
      AuthState(
        status: status ?? this.status,
        token: token ?? this.token,
        error: error ?? this.error,
      );
}

class AuthNotifier extends Notifier<AuthState> {
  @override
  AuthState build() {
    // Check session on startup
    Future.microtask(_checkSession);
    return const AuthState(status: AuthStatus.checking);
  }

  Future<void> _checkSession() async {
    final repo = ref.read(authRepositoryProvider);
    final loggedIn = await repo.isLoggedIn();
    state = AuthState(
      status: loggedIn ? AuthStatus.authenticated : AuthStatus.unauthenticated,
      token: repo.accessToken,
    );
  }

  Future<bool> login(String username, String password) async {
    final repo = ref.read(authRepositoryProvider);
    final result = await repo.login(username, password);
    return result.fold(
      (token) {
        state = AuthState(
          status: AuthStatus.authenticated,
          token: token,
        );
        return true;
      },
      (error) {
        state = state.copyWith(
          status: AuthStatus.unauthenticated,
          error: error,
        );
        return false;
      },
    );
  }

  Future<void> logout() async {
    await ref.read(authRepositoryProvider).logout();
    state = const AuthState(status: AuthStatus.unauthenticated);
  }
}

final authNotifierProvider = NotifierProvider<AuthNotifier, AuthState>(
  AuthNotifier.new,
);
