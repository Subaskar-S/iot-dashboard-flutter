import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:iot_dashboard/main.dart';

void main() {
  testWidgets('App renders without crashing', (WidgetTester tester) async {
    await tester.pumpWidget(
      const ProviderScope(child: IoTDashboardApp()),
    );
    // App starts in AuthStatus.checking — shows a loading/routing state.
    expect(find.byType(MaterialApp), findsNothing);
  });
}
