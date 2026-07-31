import '../../core/utils/result.dart';

abstract interface class AuthRepository {
  Future<Result<String>> login(String username, String password);
  Future<Result<void>> logout();
  Future<bool> isLoggedIn();
  String? get accessToken;
}
