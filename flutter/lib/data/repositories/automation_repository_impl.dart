import 'package:dio/dio.dart';
import '../../core/constants/api_endpoints.dart';
import '../../core/utils/result.dart';
import '../../domain/entities/automation_rule.dart';
import '../../domain/repositories/automation_repository.dart';
import '../datasources/api_client.dart';

class AutomationRepositoryImpl implements AutomationRepository {
  AutomationRepositoryImpl({required this.dio});
  final Dio dio;

  @override
  Future<Result<List<AutomationRule>>> getRules() async {
    try {
      final response =
          await dio.get<Map<String, dynamic>>(ApiEndpoints.automationRules);
      final items = (response.data!['rules'] as List)
          .cast<Map<String, dynamic>>()
          .map(_fromJson)
          .toList();
      return Success(items);
    } on DioException catch (e) {
      return Failure(e.readableMessage, statusCode: e.response?.statusCode);
    }
  }

  @override
  Future<Result<AutomationRule>> createRule(AutomationRule rule) async {
    try {
      final response = await dio.post<Map<String, dynamic>>(
        ApiEndpoints.automationRules,
        data: _toJson(rule),
      );
      return Success(_fromJson(response.data!));
    } on DioException catch (e) {
      return Failure(e.readableMessage, statusCode: e.response?.statusCode);
    }
  }

  @override
  Future<Result<void>> deleteRule(String id) async {
    try {
      await dio.delete<void>(
        ApiEndpoints.automationRules,
        queryParameters: {'id': id},
      );
      return const Success(null);
    } on DioException catch (e) {
      return Failure(e.readableMessage, statusCode: e.response?.statusCode);
    }
  }

  static AutomationRule _fromJson(Map<String, dynamic> j) => AutomationRule(
        id: j['id'] as String,
        name: j['name'] as String,
        enabled: j['enabled'] as bool? ?? true,
        conditions: (j['conditions'] as List? ?? [])
            .cast<Map<String, dynamic>>()
            .map((c) => Condition(
                  sensorType: c['sensor_type'] as String,
                  operator: c['operator'] as String,
                  threshold: (c['threshold'] as num).toDouble(),
                ))
            .toList(),
        actions: (j['actions'] as List? ?? [])
            .cast<Map<String, dynamic>>()
            .map((a) => RuleAction(
                  deviceId: a['device_id'] as String,
                  command: a['command'] as String,
                  parameters: (a['parameters'] as Map<String, dynamic>?) ?? {},
                ))
            .toList(),
      );

  static Map<String, dynamic> _toJson(AutomationRule r) => {
        'id': r.id,
        'name': r.name,
        'enabled': r.enabled,
        'conditions': r.conditions
            .map((c) => {
                  'sensor_type': c.sensorType,
                  'operator': c.operator,
                  'threshold': c.threshold,
                })
            .toList(),
        'actions': r.actions
            .map((a) => {
                  'device_id': a.deviceId,
                  'command': a.command,
                  'parameters': a.parameters,
                })
            .toList(),
      };
}
