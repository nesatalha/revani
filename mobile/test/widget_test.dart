import 'package:flutter_test/flutter_test.dart';
import 'package:revani/models/status.dart';

void main() {
  test('RevaniStatus kanal haritasından okunur', () {
    final s = RevaniStatus.fromMap({
      'serviceRunning': true,
      'bleState': 'ready',
      'deviceName': 'Revani',
      'displayBattery': 80,
      'lastNav': {
        'maneuver': 8,
        'roundaboutExit': 2,
        'distanceM': 120,
        'etaMin': 9,
        'street': 'Bağdat Cd.',
      },
      'navActive': true,
      'gpsSpeed': 42,
      'mapFps': 0.0,
      'errors': ['hata'],
    });
    expect(s.bleState, BleState.ready);
    expect(s.lastNav!.maneuver, Maneuver.roundabout);
    expect(s.lastNav!.roundaboutExit, 2);
    expect(s.errors, ['hata']);
  });

  test('Bilinmeyen değerlerde varsayılan', () {
    final s = RevaniStatus.fromMap({'bleState': 'garip'});
    expect(s.bleState, BleState.idle);
    expect(s.lastNav, isNull);
    expect(Maneuver.fromCode(42), Maneuver.straight);
  });
}
