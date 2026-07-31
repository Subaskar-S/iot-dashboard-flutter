import 'package:fl_chart/fl_chart.dart';
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:intl/intl.dart';
import '../../../domain/entities/sensor_reading.dart';
import '../../providers/sensor_provider.dart';
import '../../../core/constants/app_constants.dart';

class SensorsPage extends ConsumerStatefulWidget {
  const SensorsPage({super.key, required this.deviceId});
  final String deviceId;

  @override
  ConsumerState<SensorsPage> createState() => _SensorsPageState();
}

class _SensorsPageState extends ConsumerState<SensorsPage> {
  final List<SensorReading> _liveBuffer = [];
  static const int _maxPoints = AppConstants.chartMaxDataPoints;

  @override
  Widget build(BuildContext context) {
    // Listen to live stream and accumulate
    ref.listen(liveSensorProvider(widget.deviceId), (_, next) {
      next.whenData((reading) {
        setState(() {
          _liveBuffer.add(reading);
          if (_liveBuffer.length > _maxPoints) {
            _liveBuffer.removeAt(0);
          }
        });
      });
    });

    final historyAsync = ref.watch(
      sensorHistoryProvider((deviceId: widget.deviceId, sensorType: null)),
    );

    return Scaffold(
      appBar: AppBar(title: Text('Sensors – ${widget.deviceId}')),
      body: historyAsync.when(
        loading: () => const Center(child: CircularProgressIndicator()),
        error: (e, _) => Center(child: Text('$e')),
        data: (history) {
          final allReadings = [
            ...history.reversed,
            ..._liveBuffer,
          ];

          // Group by sensor type
          final Map<String, List<SensorReading>> grouped = {};
          for (final r in allReadings) {
            grouped.putIfAbsent(r.sensorType, () => []).add(r);
          }

          if (grouped.isEmpty) {
            return const Center(
                child: Text('No sensor data yet for this device.'));
          }

          return ListView(
            padding: const EdgeInsets.all(16),
            children: grouped.entries
                .map((e) => _SensorChart(
                    sensorType: e.key,
                    readings: e.value.take(_maxPoints).toList()))
                .toList(),
          );
        },
      ),
    );
  }
}

class _SensorChart extends StatelessWidget {
  const _SensorChart({required this.sensorType, required this.readings});

  final String sensorType;
  final List<SensorReading> readings;

  @override
  Widget build(BuildContext context) {
    if (readings.isEmpty) return const SizedBox.shrink();

    final spots = readings
        .asMap()
        .entries
        .map((e) => FlSpot(e.key.toDouble(), e.value.value))
        .toList();

    final latest = readings.last;
    final unit = latest.unit;
    final minY = readings.map((r) => r.value).reduce((a, b) => a < b ? a : b);
    final maxY = readings.map((r) => r.value).reduce((a, b) => a > b ? a : b);
    final padding = (maxY - minY) * 0.1 + 0.5;

    return Card(
      margin: const EdgeInsets.only(bottom: 16),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Text(sensorType.toUpperCase(),
                    style: const TextStyle(
                        fontWeight: FontWeight.bold,
                        fontSize: 13,
                        letterSpacing: 0.5)),
                Text(
                  '${latest.value.toStringAsFixed(1)} $unit',
                  style: TextStyle(
                    fontWeight: FontWeight.bold,
                    fontSize: 20,
                    color: Theme.of(context).colorScheme.primary,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 4),
            Text(
              'Last: ${DateFormat('HH:mm:ss').format(latest.timestamp)}',
              style: const TextStyle(fontSize: 11, color: Colors.grey),
            ),
            const SizedBox(height: 16),
            SizedBox(
              height: 140,
              child: LineChart(
                LineChartData(
                  minY: minY - padding,
                  maxY: maxY + padding,
                  gridData: const FlGridData(show: false),
                  borderData: FlBorderData(show: false),
                  titlesData: const FlTitlesData(
                    leftTitles: AxisTitles(
                        sideTitles:
                            SideTitles(showTitles: true, reservedSize: 36)),
                    bottomTitles:
                        AxisTitles(sideTitles: SideTitles(showTitles: false)),
                    topTitles:
                        AxisTitles(sideTitles: SideTitles(showTitles: false)),
                    rightTitles:
                        AxisTitles(sideTitles: SideTitles(showTitles: false)),
                  ),
                  lineBarsData: [
                    LineChartBarData(
                      spots: spots,
                      isCurved: true,
                      color: Theme.of(context).colorScheme.primary,
                      barWidth: 2,
                      dotData: const FlDotData(show: false),
                      belowBarData: BarAreaData(
                        show: true,
                        color: Theme.of(context)
                            .colorScheme
                            .primary
                            .withValues(alpha: 0.1),
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
