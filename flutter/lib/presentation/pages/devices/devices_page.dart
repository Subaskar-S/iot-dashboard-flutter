import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../../providers/device_provider.dart';
import '../../../domain/entities/device.dart';
import '../../../core/theme/app_theme.dart';
import '../../../core/utils/result.dart';

class DevicesPage extends ConsumerWidget {
  const DevicesPage({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final devicesAsync = ref.watch(devicesProvider);
    final isDark = Theme.of(context).brightness == Brightness.dark;

    return Scaffold(
      appBar: AppBar(
        title: const Text('Devices'),
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: () => ref.read(devicesProvider.notifier).refresh(),
          ),
        ],
      ),
      floatingActionButton: FloatingActionButton.extended(
        onPressed: () => _showRegisterDialog(context, ref),
        icon: const Icon(Icons.add),
        label: const Text('Register'),
      ),
      body: devicesAsync.when(
        loading: () => const Center(child: CircularProgressIndicator()),
        error: (e, _) => Center(child: Text('$e')),
        data: (devices) => devices.isEmpty
            ? const Center(
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Icon(Icons.devices_outlined, size: 64, color: Colors.grey),
                    SizedBox(height: 12),
                    Text('No devices yet. Tap + to register one.'),
                  ],
                ),
              )
            : ListView.builder(
                padding: const EdgeInsets.all(16),
                itemCount: devices.length,
                itemBuilder: (ctx, i) =>
                    _DeviceTile(device: devices[i], isDark: isDark),
              ),
      ),
    );
  }

  void _showRegisterDialog(BuildContext context, WidgetRef ref) {
    showDialog<void>(
      context: context,
      builder: (_) => _RegisterDeviceDialog(ref: ref),
    );
  }
}

class _DeviceTile extends StatelessWidget {
  const _DeviceTile({required this.device, required this.isDark});
  final Device device;
  final bool isDark;

  @override
  Widget build(BuildContext context) => Card(
        margin: const EdgeInsets.only(bottom: 8),
        child: ListTile(
          leading: CircleAvatar(
            backgroundColor: device.online
                ? AppTheme.statusOnline(isDark).withOpacity(0.15)
                : Colors.grey.withOpacity(0.15),
            child: Icon(Icons.sensors,
                color: device.online
                    ? AppTheme.statusOnline(isDark)
                    : AppTheme.statusOffline(isDark)),
          ),
          title: Text(device.name,
              style: const TextStyle(fontWeight: FontWeight.w600)),
          subtitle: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text('ID: ${device.id}',
                  style: const TextStyle(fontSize: 11, color: Colors.grey)),
              Text('${device.type} • ${device.protocol} • ${device.address}',
                  style: const TextStyle(fontSize: 12)),
            ],
          ),
          isThreeLine: true,
          trailing: Row(
            mainAxisSize: MainAxisSize.min,
            children: [
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 3),
                decoration: BoxDecoration(
                  color: device.online
                      ? AppTheme.statusOnline(isDark).withOpacity(0.15)
                      : Colors.grey.withOpacity(0.15),
                  borderRadius: BorderRadius.circular(12),
                ),
                child: Text(
                  device.online ? 'Online' : 'Offline',
                  style: TextStyle(
                    fontSize: 11,
                    fontWeight: FontWeight.w600,
                    color: device.online
                        ? AppTheme.statusOnline(isDark)
                        : AppTheme.statusOffline(isDark),
                  ),
                ),
              ),
              const SizedBox(width: 4),
              IconButton(
                icon: const Icon(Icons.show_chart, size: 20),
                tooltip: 'View sensors',
                onPressed: () => context.go('/sensors/${device.id}'),
              ),
            ],
          ),
        ),
      );
}

class _RegisterDeviceDialog extends ConsumerStatefulWidget {
  const _RegisterDeviceDialog({required this.ref});
  final WidgetRef ref;

  @override
  ConsumerState<_RegisterDeviceDialog> createState() =>
      _RegisterDeviceDialogState();
}

class _RegisterDeviceDialogState extends ConsumerState<_RegisterDeviceDialog> {
  final _formKey = GlobalKey<FormState>();
  final _idCtrl = TextEditingController();
  final _nameCtrl = TextEditingController();
  final _addressCtrl = TextEditingController();
  String _type = 'sensor';
  String _protocol = 'mqtt';
  bool _loading = false;

  @override
  void dispose() {
    _idCtrl.dispose();
    _nameCtrl.dispose();
    _addressCtrl.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) => AlertDialog(
        title: const Text('Register Device'),
        content: Form(
          key: _formKey,
          child: SingleChildScrollView(
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                TextFormField(
                  controller: _idCtrl,
                  decoration: const InputDecoration(labelText: 'Device ID'),
                  validator: (v) => v == null || v.isEmpty ? 'Required' : null,
                ),
                const SizedBox(height: 12),
                TextFormField(
                  controller: _nameCtrl,
                  decoration: const InputDecoration(labelText: 'Name'),
                  validator: (v) => v == null || v.isEmpty ? 'Required' : null,
                ),
                const SizedBox(height: 12),
                DropdownButtonFormField<String>(
                  initialValue: _type,
                  decoration: const InputDecoration(labelText: 'Type'),
                  items: ['sensor', 'actuator', 'gateway']
                      .map((t) => DropdownMenuItem(value: t, child: Text(t)))
                      .toList(),
                  onChanged: (v) => setState(() => _type = v!),
                ),
                const SizedBox(height: 12),
                DropdownButtonFormField<String>(
                  initialValue: _protocol,
                  decoration: const InputDecoration(labelText: 'Protocol'),
                  items: ['mqtt', 'modbus']
                      .map((p) => DropdownMenuItem(value: p, child: Text(p)))
                      .toList(),
                  onChanged: (v) => setState(() => _protocol = v!),
                ),
                const SizedBox(height: 12),
                TextFormField(
                  controller: _addressCtrl,
                  decoration:
                      const InputDecoration(labelText: 'Address (optional)'),
                ),
              ],
            ),
          ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: _loading ? null : _submit,
            child: _loading
                ? const SizedBox(
                    height: 16,
                    width: 16,
                    child: CircularProgressIndicator(
                        strokeWidth: 2, color: Colors.white))
                : const Text('Register'),
          ),
        ],
      );

  Future<void> _submit() async {
    if (!_formKey.currentState!.validate()) return;
    setState(() => _loading = true);

    final result = await ref.read(devicesProvider.notifier).register(
          id: _idCtrl.text.trim(),
          name: _nameCtrl.text.trim(),
          type: _type,
          protocol: _protocol,
          address: _addressCtrl.text.trim(),
        );

    if (mounted) {
      setState(() => _loading = false);
      if (result.isSuccess) {
        Navigator.pop(context);
      } else {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
              content: Text(result.error),
              backgroundColor: Colors.red.shade700),
        );
      }
    }
  }
}
