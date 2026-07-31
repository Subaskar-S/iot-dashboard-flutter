import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../../domain/entities/automation_rule.dart';
import '../../providers/automation_provider.dart';
import '../../../core/utils/result.dart';

class AutomationPage extends ConsumerWidget {
  const AutomationPage({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final rulesAsync = ref.watch(automationRulesProvider);

    return Scaffold(
      appBar: AppBar(
        title: const Text('Automation Rules'),
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: () =>
                ref.read(automationRulesProvider.notifier).refresh(),
          ),
        ],
      ),
      floatingActionButton: FloatingActionButton.extended(
        onPressed: () => _showCreateDialog(context, ref),
        icon: const Icon(Icons.add),
        label: const Text('Add Rule'),
      ),
      body: rulesAsync.when(
        loading: () => const Center(child: CircularProgressIndicator()),
        error: (e, _) => Center(child: Text('$e')),
        data: (rules) => rules.isEmpty
            ? const Center(
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Icon(Icons.auto_awesome_outlined,
                        size: 64, color: Colors.grey),
                    SizedBox(height: 12),
                    Text('No automation rules yet.'),
                  ],
                ),
              )
            : ListView.builder(
                padding: const EdgeInsets.all(16),
                itemCount: rules.length,
                itemBuilder: (ctx, i) => _RuleTile(rule: rules[i]),
              ),
      ),
    );
  }

  void _showCreateDialog(BuildContext context, WidgetRef ref) {
    showDialog<void>(
      context: context,
      builder: (_) => _CreateRuleDialog(ref: ref),
    );
  }
}

class _RuleTile extends ConsumerWidget {
  const _RuleTile({required this.rule});
  final AutomationRule rule;

  @override
  Widget build(BuildContext context, WidgetRef ref) => Card(
        margin: const EdgeInsets.only(bottom: 8),
        child: ExpansionTile(
          leading: Icon(
            Icons.bolt,
            color: rule.enabled ? Colors.amber : Colors.grey,
          ),
          title: Text(rule.name,
              style: const TextStyle(fontWeight: FontWeight.w600)),
          subtitle: Text(
            rule.enabled ? 'Enabled' : 'Disabled',
            style: TextStyle(
              color: rule.enabled ? Colors.green : Colors.grey,
              fontSize: 12,
            ),
          ),
          trailing: IconButton(
            icon: const Icon(Icons.delete_outline, color: Colors.red),
            onPressed: () => _confirmDelete(context, ref),
          ),
          children: [
            Padding(
              padding: const EdgeInsets.fromLTRB(16, 0, 16, 12),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text('Conditions:',
                      style:
                          TextStyle(fontWeight: FontWeight.bold, fontSize: 12)),
                  ...rule.conditions.map((c) => Padding(
                        padding: const EdgeInsets.only(left: 8, top: 4),
                        child: Text(
                          '• ${c.sensorType} ${c.operator} ${c.threshold}',
                          style: const TextStyle(fontSize: 13),
                        ),
                      )),
                  const SizedBox(height: 8),
                  const Text('Actions:',
                      style:
                          TextStyle(fontWeight: FontWeight.bold, fontSize: 12)),
                  ...rule.actions.map((a) => Padding(
                        padding: const EdgeInsets.only(left: 8, top: 4),
                        child: Text(
                          '• ${a.command} → ${a.deviceId}',
                          style: const TextStyle(fontSize: 13),
                        ),
                      )),
                ],
              ),
            ),
          ],
        ),
      );

  void _confirmDelete(BuildContext context, WidgetRef ref) {
    showDialog<void>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Delete Rule'),
        content: Text('Delete "${rule.name}"?'),
        actions: [
          TextButton(
              onPressed: () => Navigator.pop(ctx), child: const Text('Cancel')),
          ElevatedButton(
            style: ElevatedButton.styleFrom(backgroundColor: Colors.red),
            onPressed: () async {
              Navigator.pop(ctx);
              await ref.read(automationRulesProvider.notifier).delete(rule.id);
            },
            child: const Text('Delete'),
          ),
        ],
      ),
    );
  }
}

class _CreateRuleDialog extends ConsumerStatefulWidget {
  const _CreateRuleDialog({required this.ref});
  final WidgetRef ref;

  @override
  ConsumerState<_CreateRuleDialog> createState() => _CreateRuleDialogState();
}

class _CreateRuleDialogState extends ConsumerState<_CreateRuleDialog> {
  final _formKey = GlobalKey<FormState>();
  final _idCtrl = TextEditingController();
  final _nameCtrl = TextEditingController();
  final _sensorCtrl = TextEditingController();
  final _thresholdCtrl = TextEditingController();
  final _deviceCtrl = TextEditingController();
  final _commandCtrl = TextEditingController();
  String _op = '>';
  bool _loading = false;

  @override
  void dispose() {
    _idCtrl.dispose();
    _nameCtrl.dispose();
    _sensorCtrl.dispose();
    _thresholdCtrl.dispose();
    _deviceCtrl.dispose();
    _commandCtrl.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) => AlertDialog(
        title: const Text('Create Automation Rule'),
        content: Form(
          key: _formKey,
          child: SingleChildScrollView(
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                TextFormField(
                    controller: _idCtrl,
                    decoration: const InputDecoration(labelText: 'Rule ID'),
                    validator: (v) =>
                        v == null || v.isEmpty ? 'Required' : null),
                const SizedBox(height: 8),
                TextFormField(
                    controller: _nameCtrl,
                    decoration: const InputDecoration(labelText: 'Name'),
                    validator: (v) =>
                        v == null || v.isEmpty ? 'Required' : null),
                const Divider(height: 24),
                const Text('Condition',
                    style: TextStyle(fontWeight: FontWeight.bold)),
                const SizedBox(height: 8),
                TextFormField(
                    controller: _sensorCtrl,
                    decoration:
                        const InputDecoration(labelText: 'Sensor Type')),
                const SizedBox(height: 8),
                DropdownButtonFormField<String>(
                  initialValue: _op,
                  decoration: const InputDecoration(labelText: 'Operator'),
                  items: ['>', '<', '>=', '<=', '==', '!=']
                      .map((o) => DropdownMenuItem(value: o, child: Text(o)))
                      .toList(),
                  onChanged: (v) => setState(() => _op = v!),
                ),
                const SizedBox(height: 8),
                TextFormField(
                    controller: _thresholdCtrl,
                    decoration: const InputDecoration(labelText: 'Threshold'),
                    keyboardType: TextInputType.number),
                const Divider(height: 24),
                const Text('Action',
                    style: TextStyle(fontWeight: FontWeight.bold)),
                const SizedBox(height: 8),
                TextFormField(
                    controller: _deviceCtrl,
                    decoration:
                        const InputDecoration(labelText: 'Target Device ID')),
                const SizedBox(height: 8),
                TextFormField(
                    controller: _commandCtrl,
                    decoration: const InputDecoration(labelText: 'Command')),
              ],
            ),
          ),
        ),
        actions: [
          TextButton(
              onPressed: () => Navigator.pop(context),
              child: const Text('Cancel')),
          ElevatedButton(
            onPressed: _loading ? null : _submit,
            child: _loading
                ? const SizedBox(
                    height: 16,
                    width: 16,
                    child: CircularProgressIndicator(
                        strokeWidth: 2, color: Colors.white))
                : const Text('Create'),
          ),
        ],
      );

  Future<void> _submit() async {
    if (!_formKey.currentState!.validate()) return;
    setState(() => _loading = true);

    final rule = AutomationRule(
      id: _idCtrl.text.trim(),
      name: _nameCtrl.text.trim(),
      enabled: true,
      conditions: [
        Condition(
          sensorType: _sensorCtrl.text.trim(),
          operator: _op,
          threshold: double.tryParse(_thresholdCtrl.text.trim()) ?? 0,
        ),
      ],
      actions: [
        RuleAction(
          deviceId: _deviceCtrl.text.trim(),
          command: _commandCtrl.text.trim(),
        ),
      ],
    );

    final result =
        await ref.read(automationRulesProvider.notifier).create(rule);

    if (mounted) {
      setState(() => _loading = false);
      if (result.isSuccess) {
        Navigator.pop(context);
      } else {
        ScaffoldMessenger.of(context).showSnackBar(SnackBar(
            content: Text(result.error), backgroundColor: Colors.red.shade700));
      }
    }
  }
}
