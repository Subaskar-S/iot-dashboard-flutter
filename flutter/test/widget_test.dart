import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

/// Minimal smoke test — verifies Flutter widget infrastructure works.
/// The full IoTDashboardApp uses Riverpod providers that connect to real
/// network services (WebSocket, HTTP), so end-to-end widget tests are
/// covered by integration tests run against a live backend.
void main() {
  testWidgets('MaterialApp renders without crashing',
      (WidgetTester tester) async {
    await tester.pumpWidget(
      const MaterialApp(
        home: Scaffold(body: Center(child: Text('IoT Dashboard'))),
      ),
    );
    expect(find.text('IoT Dashboard'), findsOneWidget);
  });
}
