import '../entities/automation_rule.dart';
import '../../core/utils/result.dart';

abstract interface class AutomationRepository {
  Future<Result<List<AutomationRule>>> getRules();
  Future<Result<AutomationRule>> createRule(AutomationRule rule);
  Future<Result<void>> deleteRule(String id);
}
