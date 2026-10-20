import 'package:flutter/services.dart';

import '../models/status.dart';

/// Kotlin servisine giden ince sarmalayıcı (docs/PLAN.md bölüm 3.2).
/// İş mantığı yok; her çağrı doğrudan `revani/control` kanalına gider.
class RevaniControl {
  RevaniControl._();

  static const _control = MethodChannel('revani/control');
  static const _status = EventChannel('revani/status');
  static const _scan = EventChannel('revani/scan');

  /// Son gelen durum. Servis yalnız değişince yayınladığı için sonradan
  /// dinlemeye başlayan ekranlar bununla başlar.
  static RevaniStatus lastStatus = const RevaniStatus();

  static final Stream<RevaniStatus> status = _status
      .receiveBroadcastStream()
      .map(
        (e) => lastStatus = RevaniStatus.fromMap(e as Map<Object?, Object?>),
      );

  static final Stream<List<ScannedDevice>> scan = _scan
      .receiveBroadcastStream()
      .map(
        (e) => (e as List<Object?>)
            .map((d) => ScannedDevice.fromMap(d as Map<Object?, Object?>))
            .toList(),
      );

  static Future<void> startService() => _control.invokeMethod('startService');
  static Future<void> stopService() => _control.invokeMethod('stopService');
  static Future<void> scanDevices() => _control.invokeMethod('scanDevices');
  static Future<void> stopScan() => _control.invokeMethod('stopScan');
  static Future<void> connect(String deviceId) =>
      _control.invokeMethod('connect', {'deviceId': deviceId});
  static Future<void> disconnect() => _control.invokeMethod('disconnect');

  static Future<RevaniSettings> getSettings() async => RevaniSettings.fromMap(
    (await _control.invokeMethod<Map<Object?, Object?>>('getSettings'))!,
  );
  static Future<RevaniSettings> setSettings(RevaniSettings s) async =>
      RevaniSettings.fromMap(
        (await _control.invokeMethod<Map<Object?, Object?>>(
          'setSettings',
          s.toMap(),
        ))!,
      );

  static Future<PermissionState> getPermissions() async =>
      PermissionState.fromMap(
        (await _control.invokeMethod<Map<Object?, Object?>>('getPermissions'))!,
      );
  static Future<PermissionState> requestRuntimePermissions() async =>
      PermissionState.fromMap(
        (await _control.invokeMethod<Map<Object?, Object?>>(
          'requestRuntimePermissions',
        ))!,
      );
  static Future<void> openNotificationAccessSettings() =>
      _control.invokeMethod('openNotificationAccessSettings');
  static Future<void> requestIgnoreBatteryOptimizations() =>
      _control.invokeMethod('requestIgnoreBatteryOptimizations');

  /// Masada test: Google Maps yerine sahte navigasyon.
  static Future<void> setFakeNav(bool enabled) =>
      _control.invokeMethod('setFakeNav', {'enabled': enabled});
}
