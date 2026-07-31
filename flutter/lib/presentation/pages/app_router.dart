import 'package:go_router/go_router.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../providers/auth_provider.dart';
import 'auth/login_page.dart';
import 'dashboard/dashboard_page.dart';
import 'devices/devices_page.dart';
import 'sensors/sensors_page.dart';
import 'automation/automation_page.dart';
import 'settings/settings_page.dart';
import '../widgets/common/main_scaffold.dart';

final routerProvider = Provider<GoRouter>((ref) {
  final authState = ref.watch(authNotifierProvider);

  return GoRouter(
    initialLocation: '/',
    redirect: (context, state) {
      final isAuth = authState.isAuthenticated;
      final isChecking = authState.status == AuthStatus.checking ||
          authState.status == AuthStatus.initial;
      final isLoginPage = state.matchedLocation == '/login';

      if (isChecking) return null;
      if (!isAuth && !isLoginPage) return '/login';
      if (isAuth && isLoginPage) return '/';
      return null;
    },
    routes: [
      GoRoute(
        path: '/login',
        builder: (_, __) => const LoginPage(),
      ),
      ShellRoute(
        builder: (context, state, child) => MainScaffold(child: child),
        routes: [
          GoRoute(
            path: '/',
            builder: (_, __) => const DashboardPage(),
          ),
          GoRoute(
            path: '/devices',
            builder: (_, __) => const DevicesPage(),
          ),
          GoRoute(
            path: '/sensors/:deviceId',
            builder: (_, state) =>
                SensorsPage(deviceId: state.pathParameters['deviceId']!),
          ),
          GoRoute(
            path: '/automation',
            builder: (_, __) => const AutomationPage(),
          ),
          GoRoute(
            path: '/settings',
            builder: (_, __) => const SettingsPage(),
          ),
        ],
      ),
    ],
  );
});
