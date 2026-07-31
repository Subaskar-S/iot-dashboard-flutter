import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../data/repositories/automation_repository_impl.dart';
import '../../domain/entities/automation_rule.dart';
import '../../domain/repositories/automation_repository.dart';
import '../../core/utils/result.dart';
import 'device_provider.dart';

final automationRepositoryProvider = Provider<AutomationRepository>(
  (ref) => AutomationRepositoryImpl(dio: ref.watch(dioProvider)),
);

final automationRulesProvider =
    AsyncNotifierProvider<AutomationNotifier, List<AutomationRule>>(
  AutomationNotifier.new,
);

class AutomationNotifier extends AsyncNotifier<List<AutomationRule>> {
  @override
  Future<List<AutomationRule>> build() => _fetch();

  Future<List<AutomationRule>> _fetch() async {
    final result = await ref.read(automationRepositoryProvider).getRules();
    return result.fold((list) => list, (err) => throw Exception(err));
  }

  Future<void> refresh() async {
    state = const AsyncLoading();
    state = await AsyncValue.guard(_fetch);
  }

  Future<Result<AutomationRule>> create(AutomationRule rule) async {
    final result =
        await ref.read(automationRepositoryProvider).createRule(rule);
    if (result.isSuccess) await refresh();
    return result;
  }

  Future<Result<void>> delete(String id) async {
    final result = await ref.read(automationRepositoryProvider).deleteRule(id);
    if (result.isSuccess) await refresh();
    return result;
  }
}
