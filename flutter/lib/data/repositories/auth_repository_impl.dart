import 'package:dio/dio.dart';
import 'package:flutter_secure_storage/flutter_secure_storage.dart';
import '../../core/constants/api_endpoints.dart';
import '../../core/constants/app_constants.dart';
import '../../core/utils/result.dart';
import '../../domain/repositories/auth_repository.dart';
import '../datasources/api_client.dart';

class AuthRepositoryImpl implements AuthRepository {
  AuthRepositoryImpl({
    required this.storage,
    required this.baseUrl,
  });

  final FlutterSecureStorage storage;
  final String baseUrl;
  String? _cachedToken;

  @override
  String? get accessToken => _cachedToken;

  @override
  Future<bool> isLoggedIn() async {
    final token = await storage.read(key: AppConstants.accessTokenKey);
    _cachedToken = token;
    return token != null && token.isNotEmpty;
  }

  @override
  Future<Result<String>> login(String username, String password) async {
    try {
      final dio = createDio(baseUrl: baseUrl);
      final response = await dio.post<Map<String, dynamic>>(
        ApiEndpoints.login,
        data: {'username': username, 'password': password},
      );

      final data = response.data!;
      final accessToken = data['access_token'] as String;
      final refreshToken = data['refresh_token'] as String? ?? '';

      _cachedToken = accessToken;
      await storage.write(key: AppConstants.accessTokenKey, value: accessToken);
      await storage.write(
          key: AppConstants.refreshTokenKey, value: refreshToken);

      return Success(accessToken);
    } on DioException catch (e) {
      return Failure(e.readableMessage, statusCode: e.response?.statusCode);
    } catch (e) {
      return Failure(e.toString());
    }
  }

  @override
  Future<Result<void>> logout() async {
    try {
      if (_cachedToken != null) {
        final dio = createDio(baseUrl: baseUrl, accessToken: _cachedToken);
        await dio.post<void>(ApiEndpoints.logout);
      }
    } catch (_) {}

    _cachedToken = null;
    await storage.delete(key: AppConstants.accessTokenKey);
    await storage.delete(key: AppConstants.refreshTokenKey);
    return const Success(null);
  }
}
