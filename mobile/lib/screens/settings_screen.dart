import 'package:flutter/material.dart';

import '../channels/revani_channels.dart';
import '../models/status.dart';
import 'permissions_screen.dart';
import 'status_builder.dart';

class SettingsScreen extends StatefulWidget {
  const SettingsScreen({super.key});

  @override
  State<SettingsScreen> createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen> {
  RevaniSettings? _settings;

  @override
  void initState() {
    super.initState();
    RevaniControl.getSettings().then((s) {
      if (mounted) setState(() => _settings = s);
    });
  }

  Future<void> _update(RevaniSettings s) async {
    final saved = await RevaniControl.setSettings(s);
    if (mounted) setState(() => _settings = saved);
  }

  @override
  Widget build(BuildContext context) {
    final s = _settings;
    return Scaffold(
      appBar: AppBar(title: const Text('Ayarlar')),
      body: s == null
          ? const Center(child: CircularProgressIndicator())
          : ListView(
              children: [
                SwitchListTile(
                  title: const Text('Otomatik bağlan'),
                  subtitle: Text(
                    s.lastDeviceAddress == null
                        ? 'Servis başlayınca son ekrana bağlanır'
                        : 'Son ekran: ${s.lastDeviceAddress}',
                  ),
                  value: s.autoConnect,
                  onChanged: (v) => _update(s.copyWith(autoConnect: v)),
                ),
                StatusBuilder(
                  builder: (context, status) => SwitchListTile(
                    title: const Text('Simülatöre gönder (USB köprüsü)'),
                    subtitle: Text(
                      s.bridgeEnabled
                          ? _bridgeLabel(status.bridgeState)
                          : 'Mac’te simülatörü çalıştırıp USB ile bağla',
                    ),
                    value: s.bridgeEnabled,
                    onChanged: (v) => _update(s.copyWith(bridgeEnabled: v)),
                  ),
                ),
                ListTile(
                  title: const Text('İzinler'),
                  trailing: const Icon(Icons.chevron_right),
                  onTap: () => Navigator.of(context).push(
                    MaterialPageRoute<void>(
                      builder: (_) => const PermissionsScreen(),
                    ),
                  ),
                ),
              ],
            ),
    );
  }

  static String _bridgeLabel(String state) => switch (state) {
    'connected' => 'Simülatöre bağlı',
    'connecting' => 'Simülatör bekleniyor (servis açık olmalı)',
    _ => 'Servis kapalı',
  };
}
