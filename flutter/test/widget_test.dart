import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:iot_dashboard/main.dart';

void main() {
  testWidgets('App renders MaterialApp without crashing',
      (WidgetTester tester) async {
    await tester.pumpWidget(
      const ProviderScope(child: IoTDashboardApp()),
    );
    // App should render a MaterialApp widget.
    expect(find.byType(MaterialApp), findsOneWidget);
  });
}
