import 'package:flutter/material.dart';

import '../channels/revani_channels.dart';
import '../models/status.dart';

/// İzin akışı: Bluetooth, konum, bildirim, bildirim erişimi, pil optimizasyonu.
class PermissionsScreen extends StatefulWidget {
  const PermissionsScreen({super.key});

  @override
  State<PermissionsScreen> createState() => _PermissionsScreenState();
}

class _PermissionsScreenState extends State<PermissionsScreen>
    with WidgetsBindingObserver {
  PermissionState _p = const PermissionState();

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
    _refresh();
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    super.dispose();
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state == AppLifecycleState.resumed) _refresh();
  }

  Future<void> _refresh() async {
    final p = await RevaniControl.getPermissions();
    if (mounted) setState(() => _p = p);
  }

  Future<void> _requestRuntime() async {
    final p = await RevaniControl.requestRuntimePermissions();
    if (mounted) setState(() => _p = p);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('İzinler')),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          _Step(
            title: 'Bluetooth, konum ve bildirim',
            body:
                'Ekrana bağlanmak, GPS hızını okumak ve arka planda çalıştığını göstermek için.',
            done: _p.runtimeGranted,
            action: 'İzin ver',
            onPressed: _requestRuntime,
          ),
          _Step(
            title: 'Bildirim erişimi',
            body:
                'Google Maps’in navigasyon bildirimini okumak için. Açılan listede Revani’yi etkinleştir.',
            done: _p.notificationAccess,
            action: 'Ayarları aç',
            onPressed: RevaniControl.openNotificationAccessSettings,
          ),
          _Step(
            title: 'Pil optimizasyonu muafiyeti',
            body: 'Telefon cepteyken servisin kapatılmaması için.',
            done: _p.batteryOptimizationIgnored,
            action: 'Muaf tut',
            onPressed: RevaniControl.requestIgnoreBatteryOptimizations,
          ),
        ],
      ),
    );
  }
}

class _Step extends StatelessWidget {
  const _Step({
    required this.title,
    required this.body,
    required this.done,
    required this.action,
    required this.onPressed,
  });

  final String title;
  final String body;
  final bool done;
  final String action;
  final VoidCallback onPressed;

  @override
  Widget build(BuildContext context) {
    return Card(
      child: ListTile(
        leading: Icon(
          done ? Icons.check_circle : Icons.radio_button_unchecked,
          color: done ? Colors.green : null,
        ),
        title: Text(title),
        subtitle: Text(body),
        isThreeLine: true,
        trailing: done
            ? null
            : FilledButton(onPressed: onPressed, child: Text(action)),
      ),
    );
  }
}
