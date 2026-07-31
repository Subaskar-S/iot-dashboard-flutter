import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../../providers/device_provider.dart';
import '../../providers/auth_provider.dart';
import '../../../core/theme/app_theme.dart';

class DashboardPage extends ConsumerWidget {
  const DashboardPage({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final devicesAsync = ref.watch(devicesProvider);
    final wsStatus = ref.watch(deviceStatusStreamProvider);
    final isDark = Theme.of(context).brightness == Brightness.dark;

    return Scaffold(
      appBar: AppBar(
        title: const Text('Dashboard'),
        actions: [
          IconButton(
            icon: const Icon(Icons.logout_outlined),
            tooltip: 'Sign out',
            onPressed: () async {
              await ref.read(authNotifierProvider.notifier).logout();
              if (context.mounted) context.go('/login');
            },
          ),
        ],
      ),
      body: RefreshIndicator(
        onRefresh: () => ref.read(devicesProvider.notifier).refresh(),
        child: devicesAsync.when(
          loading: () => const Center(child: CircularProgressIndicator()),
          error: (e, _) => Center(
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                const Icon(Icons.error_outline, size: 48, color: Colors.red),
                const SizedBox(height: 8),
                Text('$e'),
                const SizedBox(height: 16),
                ElevatedButton(
                  onPressed: () => ref.read(devicesProvider.notifier).refresh(),
                  child: const Text('Retry'),
                ),
              ],
            ),
          ),
          data: (devices) {
            // Apply live status updates
            final liveStatus = wsStatus.value ?? {};
            final updatedDevices = devices
                .map((d) => liveStatus.containsKey(d.id)
                    ? d.copyWith(online: liveStatus[d.id])
                    : d)
                .toList();

            final online = updatedDevices.where((d) => d.online).length;
            final offline = updatedDevices.length - online;

            return Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  // Summary cards
                  Row(
                    children: [
                      _SummaryCard(
                        label: 'Total Devices',
                        value: '${updatedDevices.length}',
                        icon: Icons.devices,
                        color: Theme.of(context).colorScheme.primary,
                      ),
                      const SizedBox(width: 12),
                      _SummaryCard(
                        label: 'Online',
                        value: '$online',
                        icon: Icons.wifi,
                        color: AppTheme.statusOnline(isDark),
                      ),
                      const SizedBox(width: 12),
                      _SummaryCard(
                        label: 'Offline',
                        value: '$offline',
                        icon: Icons.wifi_off,
                        color: AppTheme.statusOffline(isDark),
                      ),
                    ],
                  ),
                  const SizedBox(height: 24),
                  Text('All Devices',
                      style: Theme.of(context)
                          .textTheme
                          .titleMedium
                          ?.copyWith(fontWeight: FontWeight.bold)),
                  const SizedBox(height: 8),
                  Expanded(
                    child: updatedDevices.isEmpty
                        ? const Center(child: Text('No devices registered yet'))
                        : ListView.separated(
                            itemCount: updatedDevices.length,
                            separatorBuilder: (_, __) =>
                                const SizedBox(height: 8),
                            itemBuilder: (ctx, i) {
                              final d = updatedDevices[i];
                              return Card(
                                child: ListTile(
                                  leading: CircleAvatar(
                                    backgroundColor: d.online
                                        ? AppTheme.statusOnline(isDark)
                                            .withValues(alpha: 0.15)
                                        : Colors.grey.withValues(alpha: 0.15),
                                    child: Icon(
                                      Icons.sensors,
                                      color: d.online
                                          ? AppTheme.statusOnline(isDark)
                                          : AppTheme.statusOffline(isDark),
                                    ),
                                  ),
                                  title: Text(d.name),
                                  subtitle: Text(
                                    '${d.type} • ${d.protocol}',
                                    style: const TextStyle(fontSize: 12),
                                  ),
                                  trailing: Row(
                                    mainAxisSize: MainAxisSize.min,
                                    children: [
                                      Container(
                                        padding: const EdgeInsets.symmetric(
                                            horizontal: 8, vertical: 4),
                                        decoration: BoxDecoration(
                                          color: d.online
                                              ? AppTheme.statusOnline(isDark)
                                                  .withValues(alpha: 0.15)
                                              : Colors.grey.withValues(alpha: 0.15),
                                          borderRadius:
                                              BorderRadius.circular(12),
                                        ),
                                        child: Text(
                                          d.online ? 'Online' : 'Offline',
                                          style: TextStyle(
                                            fontSize: 11,
                                            fontWeight: FontWeight.w600,
                                            color: d.online
                                                ? AppTheme.statusOnline(isDark)
                                                : AppTheme.statusOffline(
                                                    isDark),
                                          ),
                                        ),
                                      ),
                                      const SizedBox(width: 8),
                                      const Icon(Icons.chevron_right),
                                    ],
                                  ),
                                  onTap: () => ctx.go('/sensors/${d.id}'),
                                ),
                              );
                            },
                          ),
                  ),
                ],
              ),
            );
          },
        ),
      ),
    );
  }
}

class _SummaryCard extends StatelessWidget {
  const _SummaryCard({
    required this.label,
    required this.value,
    required this.icon,
    required this.color,
  });

  final String label;
  final String value;
  final IconData icon;
  final Color color;

  @override
  Widget build(BuildContext context) => Expanded(
        child: Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Row(
              children: [
                CircleAvatar(
                  backgroundColor: color.withValues(alpha: 0.15),
                  child: Icon(icon, color: color),
                ),
                const SizedBox(width: 12),
                Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(value,
                        style: Theme.of(context)
                            .textTheme
                            .headlineSmall
                            ?.copyWith(fontWeight: FontWeight.bold)),
                    Text(label,
                        style:
                            const TextStyle(fontSize: 12, color: Colors.grey)),
                  ],
                ),
              ],
            ),
          ),
        ),
      );
}
