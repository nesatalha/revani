import 'package:flutter/material.dart';

import '../channels/revani_channels.dart';
import '../models/status.dart';
import 'debug_screen.dart';
import 'pairing_screen.dart';
import 'permissions_screen.dart';
import 'settings_screen.dart';
import 'status_builder.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> with WidgetsBindingObserver {
  PermissionState? _permissions;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
    _refreshPermissions();
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    super.dispose();
  }

  // Ayarlardan (bildirim erişimi, pil) dönünce izinleri yeniden oku.
  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state == AppLifecycleState.resumed) _refreshPermissions();
  }

  Future<void> _refreshPermissions() async {
    final p = await RevaniControl.getPermissions();
    if (mounted) setState(() => _permissions = p);
  }

  Future<void> _push(Widget screen) async {
    await Navigator.of(
      context,
    ).push(MaterialPageRoute<void>(builder: (_) => screen));
    _refreshPermissions();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Revani'),
        actions: [
          IconButton(
            tooltip: 'Debug',
            icon: const Icon(Icons.bug_report_outlined),
            onPressed: () => _push(const DebugScreen()),
          ),
          IconButton(
            tooltip: 'Ayarlar',
            icon: const Icon(Icons.settings_outlined),
            onPressed: () => _push(const SettingsScreen()),
          ),
        ],
      ),
      body: StatusBuilder(
        builder: (context, s) => ListView(
          padding: const EdgeInsets.all(16),
          children: [
            if (_permissions != null && !_permissions!.allGranted)
              Card(
                color: Theme.of(context).colorScheme.errorContainer,
                child: ListTile(
                  leading: const Icon(Icons.warning_amber_rounded),
                  title: const Text('Eksik izinler var'),
                  subtitle: const Text(
                    'Navigasyonun ekrana gelmesi için gerekli.',
                  ),
                  trailing: const Icon(Icons.chevron_right),
                  onTap: () => _push(const PermissionsScreen()),
                ),
              ),
            if (_permissions?.gpsBlockedByPowerSave ?? false)
              const Card(
                child: ListTile(
                  leading: Icon(Icons.battery_saver),
                  title: Text('Güç tasarrufu açık'),
                  subtitle: Text(
                    'Ekran kapanınca GPS duruyor, sürüşte hız gelmez. '
                    'Sürüşten önce Güç tasarrufunu kapat.',
                  ),
                ),
              ),
            Card(
              child: SwitchListTile(
                title: const Text('Servis'),
                subtitle: Text(
                  s.serviceRunning ? 'Arka planda çalışıyor' : 'Kapalı',
                ),
                value: s.serviceRunning,
                onChanged: (on) => on
                    ? RevaniControl.startService()
                    : RevaniControl.stopService(),
              ),
            ),
            Card(
              child: Column(
                children: [
                  ListTile(
                    leading: Icon(_bleIcon(s.bleState)),
                    title: Text(_bleLabel(s.bleState)),
                    subtitle: Text(s.deviceName ?? 'Ekran seçilmedi'),
                    trailing: s.bleState == BleState.ready
                        ? TextButton(
                            onPressed: RevaniControl.disconnect,
                            child: const Text('Kes'),
                          )
                        : null,
                  ),
                  ListTile(
                    leading: const Icon(Icons.battery_std),
                    title: const Text('Ekran pili'),
                    trailing: Text(
                      s.displayBattery == null ? '—' : '%${s.displayBattery}',
                    ),
                  ),
                  ListTile(
                    leading: const Icon(Icons.bluetooth_searching),
                    title: const Text('Ekran eşleştir'),
                    trailing: const Icon(Icons.chevron_right),
                    onTap: () => _push(const PairingScreen()),
                  ),
                ],
              ),
            ),
            Card(
              child: ListTile(
                leading: const Icon(Icons.navigation_outlined),
                title: Text(
                  s.navActive && s.lastNav != null
                      ? '${s.lastNav!.maneuver.label} · ${s.lastNav!.distanceM} m'
                      : 'Navigasyon yok',
                ),
                subtitle: Text(
                  s.navActive
                      ? s.lastNav?.street ?? ''
                      : 'Google Maps’te rota başlat',
                ),
                trailing: Text(
                  s.gpsSpeed == null ? 'GPS yok' : '${s.gpsSpeed} km/h',
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  static IconData _bleIcon(BleState s) => switch (s) {
    BleState.ready => Icons.bluetooth_connected,
    BleState.idle => Icons.bluetooth_disabled,
    _ => Icons.bluetooth_searching,
  };

  static String _bleLabel(BleState s) => switch (s) {
    BleState.idle => 'Bağlı değil',
    BleState.scanning => 'Aranıyor…',
    BleState.connecting => 'Bağlanıyor…',
    BleState.ready => 'Bağlı',
    BleState.reconnecting => 'Yeniden bağlanıyor…',
  };
}
