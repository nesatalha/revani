import 'package:flutter/widgets.dart';

import '../channels/revani_channels.dart';
import '../models/status.dart';

/// Servis durumunu (revani/status, ~2 Hz) dinleyip yeniden çizer.
class StatusBuilder extends StatelessWidget {
  const StatusBuilder({super.key, required this.builder});

  final Widget Function(BuildContext context, RevaniStatus status) builder;

  @override
  Widget build(BuildContext context) {
    return StreamBuilder<RevaniStatus>(
      stream: RevaniControl.status,
      builder: (context, snap) =>
          builder(context, snap.data ?? const RevaniStatus()),
    );
  }
}
