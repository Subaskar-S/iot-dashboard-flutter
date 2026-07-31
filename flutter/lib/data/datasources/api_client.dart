import 'package:dio/dio.dart';
import '../../core/constants/app_constants.dart';

/// Creates and configures a [Dio] instance for the IoT backend.
Dio createDio({String? baseUrl, String? accessToken}) {
  final dio = Dio(
    BaseOptions(
      baseUrl: baseUrl ?? AppConstants.defaultBaseUrl,
      connectTimeout:
          const Duration(milliseconds: AppConstants.connectTimeoutMs),
      receiveTimeout:
          const Duration(milliseconds: AppConstants.receiveTimeoutMs),
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
        if (accessToken != null) 'Authorization': 'Bearer $accessToken',
      },
    ),
  );

  dio.interceptors.add(
    InterceptorsWrapper(
      onError: (err, handler) {
        // Surface readable error messages
        handler.next(err);
      },
    ),
  );

  return dio;
}

extension DioResultX on DioException {
  String get readableMessage {
    if (response != null) {
      final data = response!.data;
      if (data is Map<String, dynamic>) {
        return data['message']?.toString() ??
            data['error']?.toString() ??
            message ??
            'Unknown error';
      }
      return data?.toString() ?? message ?? 'Unknown error';
    }
    return message ?? 'Network error';
  }
}
