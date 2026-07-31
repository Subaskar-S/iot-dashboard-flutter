import 'package:flutter/foundation.dart';

@immutable
class Condition {
  const Condition({
    required this.sensorType,
    required this.operator,
    required this.threshold,
  });

  final String sensorType;
  final String operator; // >, <, ==, !=, >=, <=
  final double threshold;
}

@immutable
class RuleAction {
  const RuleAction({
    required this.deviceId,
    required this.command,
    this.parameters = const {},
  });

  final String deviceId;
  final String command;
  final Map<String, dynamic> parameters;
}

@immutable
class AutomationRule {
  const AutomationRule({
    required this.id,
    required this.name,
    required this.enabled,
    required this.conditions,
    required this.actions,
  });

  final String id;
  final String name;
  final bool enabled;
  final List<Condition> conditions;
  final List<RuleAction> actions;

  AutomationRule copyWith({
    String? id,
    String? name,
    bool? enabled,
    List<Condition>? conditions,
    List<RuleAction>? actions,
  }) =>
      AutomationRule(
        id: id ?? this.id,
        name: name ?? this.name,
        enabled: enabled ?? this.enabled,
        conditions: conditions ?? this.conditions,
        actions: actions ?? this.actions,
      );
}
