import 'package:flutter/material.dart';

import '../channels/revani_channels.dart';
import 'status_builder.dart';

/// Canlı debug: son NAV verisi, GPS, bağlantı ve hatalar.
class DebugScreen extends StatefulWidget {
  const DebugScreen({super.key});

  @override
  State<DebugScreen> createState() => _DebugScreenState();
}

class _DebugScreenState extends State<DebugScreen> {
  bool _fakeNav = false;

  @override
  Widget build(BuildContext context) {
    final mono = Theme.of(
      context,
    ).textTheme.bodyMedium?.copyWith(fontFamily: 'monospace');
    return Scaffold(
      appBar: AppBar(title: const Text('Debug')),
      body: StatusBuilder(
        builder: (context, s) {
          final nav = s.lastNav;
          final rows = <(String, String)>[
            ('servis', s.serviceRunning ? 'çalışıyor' : 'kapalı'),
            ('ble', s.bleState.name),
            ('köprü', s.bridgeState),
            ('cihaz', '${s.deviceName ?? '—'} ${s.deviceAddress ?? ''}'),
            ('ekran pili', s.displayBattery?.toString() ?? '—'),
            (
              'gps',
              s.gpsSpeed == null ? '—' : '${s.gpsSpeed} km/h, ${s.gpsHeading}°',
            ),
            ('nav aktif', s.navActive.toString()),
            if (nav != null) ...[
              (
                'manevra',
                '${nav.maneuver.index} ${nav.maneuver.label}'
                    '${nav.roundaboutExit > 0 ? ' (çıkış ${nav.roundaboutExit})' : ''}',
              ),
              ('mesafe', '${nav.distanceM} m'),
              ('varış', '${nav.etaMin} dk'),
              ('cadde', nav.street),
            ],
            ('harita fps', s.mapFps.toStringAsFixed(1)),
          ];
          return ListView(
            padding: const EdgeInsets.all(16),
            children: [
              SwitchListTile(
                contentPadding: EdgeInsets.zero,
                title: const Text('Sahte navigasyon'),
                subtitle: const Text(
                  'Google Maps olmadan ekrana örnek rota gönderir',
                ),
                value: _fakeNav,
                onChanged: (v) {
                  setState(() => _fakeNav = v);
                  RevaniControl.setFakeNav(v);
                },
              ),
              const Divider(),
              for (final (k, v) in rows)
                Padding(
                  padding: const EdgeInsets.symmetric(vertical: 2),
                  child: Row(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      SizedBox(width: 110, child: Text(k, style: mono)),
                      Expanded(child: Text(v, style: mono)),
                    ],
                  ),
                ),
              const Divider(),
              Text('Hatalar', style: Theme.of(context).textTheme.titleSmall),
              if (s.errors.isEmpty) Text('—', style: mono),
              for (final e in s.errors.reversed) Text(e, style: mono),
            ],
          );
        },
      ),
    );
  }
}
