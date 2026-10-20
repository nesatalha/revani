import 'package:flutter/material.dart';

import '../channels/revani_channels.dart';
import '../models/status.dart';
import 'status_builder.dart';

/// Revani servis UUID'sini yayınlayan ekranları tarar ve seçileni bağlar.
class PairingScreen extends StatefulWidget {
  const PairingScreen({super.key});

  @override
  State<PairingScreen> createState() => _PairingScreenState();
}

class _PairingScreenState extends State<PairingScreen> {
  @override
  void initState() {
    super.initState();
    RevaniControl.scanDevices();
  }

  @override
  void dispose() {
    RevaniControl.stopScan();
    super.dispose();
  }

  Future<void> _connect(ScannedDevice d) async {
    await RevaniControl.connect(d.address);
    if (mounted) Navigator.of(context).pop();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Ekran eşleştir'),
        actions: [
          IconButton(
            tooltip: 'Yeniden tara',
            icon: const Icon(Icons.refresh),
            onPressed: RevaniControl.scanDevices,
          ),
        ],
      ),
      body: StatusBuilder(
        builder: (context, s) => StreamBuilder<List<ScannedDevice>>(
          stream: RevaniControl.scan,
          builder: (context, snap) {
            final devices = snap.data ?? const <ScannedDevice>[];
            return ListView(
              children: [
                if (s.bleState == BleState.scanning)
                  const LinearProgressIndicator(),
                if (devices.isEmpty)
                  const Padding(
                    padding: EdgeInsets.all(24),
                    child: Text(
                      'Ekran bulunamadı. Ekranın açık ve yakında olduğundan emin ol.',
                      textAlign: TextAlign.center,
                    ),
                  ),
                for (final d in devices)
                  ListTile(
                    leading: const Icon(Icons.radio_button_checked),
                    title: Text(d.name),
                    subtitle: Text(d.address),
                    trailing: Text('${d.rssi} dBm'),
                    selected: d.address == s.deviceAddress,
                    onTap: () => _connect(d),
                  ),
              ],
            );
          },
        ),
      ),
    );
  }
}
