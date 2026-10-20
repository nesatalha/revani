/// Kotlin servisinden gelen veri modelleri (revani/status, revani/scan).
library;

enum BleState { idle, scanning, connecting, ready, reconnecting }

/// Manevra kodları, BLE protokolüyle aynı (docs/PLAN.md bölüm 4).
enum Maneuver {
  straight('Düz'),
  slightRight('Hafif sağ'),
  right('Sağ'),
  sharpRight('Keskin sağ'),
  uTurn('U dönüşü'),
  slightLeft('Hafif sol'),
  left('Sol'),
  sharpLeft('Keskin sol'),
  roundabout('Dönel kavşak'),
  arrive('Varış');

  const Maneuver(this.label);
  final String label;

  static Maneuver fromCode(int code) =>
      code >= 0 && code < values.length ? values[code] : straight;
}

class NavInfo {
  const NavInfo({
    required this.maneuver,
    required this.roundaboutExit,
    required this.distanceM,
    required this.etaMin,
    required this.street,
  });

  factory NavInfo.fromMap(Map<Object?, Object?> m) => NavInfo(
    maneuver: Maneuver.fromCode(m['maneuver'] as int? ?? 0),
    roundaboutExit: m['roundaboutExit'] as int? ?? 0,
    distanceM: m['distanceM'] as int? ?? 0,
    etaMin: m['etaMin'] as int? ?? 0,
    street: m['street'] as String? ?? '',
  );

  final Maneuver maneuver;
  final int roundaboutExit;
  final int distanceM;
  final int etaMin;
  final String street;
}

class RevaniStatus {
  const RevaniStatus({
    this.serviceRunning = false,
    this.bleState = BleState.idle,
    this.deviceName,
    this.deviceAddress,
    this.displayBattery,
    this.bridgeState = 'off',
    this.lastNav,
    this.navActive = false,
    this.gpsSpeed,
    this.gpsHeading,
    this.mapFps = 0,
    this.errors = const [],
  });

  factory RevaniStatus.fromMap(Map<Object?, Object?> m) => RevaniStatus(
    serviceRunning: m['serviceRunning'] as bool? ?? false,
    bleState: BleState.values.firstWhere(
      (s) => s.name == m['bleState'],
      orElse: () => BleState.idle,
    ),
    deviceName: m['deviceName'] as String?,
    deviceAddress: m['deviceAddress'] as String?,
    displayBattery: m['displayBattery'] as int?,
    bridgeState: m['bridgeState'] as String? ?? 'off',
    lastNav: m['lastNav'] is Map
        ? NavInfo.fromMap(m['lastNav'] as Map<Object?, Object?>)
        : null,
    navActive: m['navActive'] as bool? ?? false,
    gpsSpeed: m['gpsSpeed'] as int?,
    gpsHeading: m['gpsHeading'] as int?,
    mapFps: (m['mapFps'] as num?)?.toDouble() ?? 0,
    errors: (m['errors'] as List<Object?>? ?? const []).cast<String>(),
  );

  final bool serviceRunning;
  final BleState bleState;
  final String? deviceName;
  final String? deviceAddress;
  final int? displayBattery;

  /// USB köprüsü (1.7): off, connecting, connected
  final String bridgeState;
  final NavInfo? lastNav;
  final bool navActive;
  final int? gpsSpeed;
  final int? gpsHeading;
  final double mapFps;
  final List<String> errors;
}

class ScannedDevice {
  const ScannedDevice({
    required this.name,
    required this.address,
    required this.rssi,
  });

  factory ScannedDevice.fromMap(Map<Object?, Object?> m) => ScannedDevice(
    name: m['name'] as String? ?? 'Revani',
    address: m['address'] as String,
    rssi: m['rssi'] as int? ?? 0,
  );

  final String name;
  final String address;
  final int rssi;
}

class PermissionState {
  const PermissionState({
    this.bluetooth = false,
    this.location = false,
    this.notifications = false,
    this.notificationAccess = false,
    this.batteryOptimizationIgnored = false,
    this.gpsBlockedByPowerSave = false,
  });

  factory PermissionState.fromMap(Map<Object?, Object?> m) => PermissionState(
    bluetooth: m['bluetooth'] as bool? ?? false,
    location: m['location'] as bool? ?? false,
    notifications: m['notifications'] as bool? ?? false,
    notificationAccess: m['notificationAccess'] as bool? ?? false,
    batteryOptimizationIgnored:
        m['batteryOptimizationIgnored'] as bool? ?? false,
    gpsBlockedByPowerSave: m['gpsBlockedByPowerSave'] as bool? ?? false,
  );

  final bool bluetooth;
  final bool location;
  final bool notifications;
  final bool notificationAccess;
  final bool batteryOptimizationIgnored;

  /// Güç tasarrufu açık ve ekran kapanınca GPS'i kapatıyor.
  final bool gpsBlockedByPowerSave;

  bool get runtimeGranted => bluetooth && location && notifications;
  bool get allGranted =>
      runtimeGranted && notificationAccess && batteryOptimizationIgnored;
}

class RevaniSettings {
  const RevaniSettings({
    this.autoConnect = true,
    this.lastDeviceAddress,
    this.bridgeEnabled = false,
  });

  factory RevaniSettings.fromMap(Map<Object?, Object?> m) => RevaniSettings(
    autoConnect: m['autoConnect'] as bool? ?? true,
    lastDeviceAddress: m['lastDeviceAddress'] as String?,
    bridgeEnabled: m['bridgeEnabled'] as bool? ?? false,
  );

  final bool autoConnect;
  final String? lastDeviceAddress;

  /// USB köprüsü: mesajları simülatöre de gönder (1.7).
  final bool bridgeEnabled;

  RevaniSettings copyWith({bool? autoConnect, bool? bridgeEnabled}) =>
      RevaniSettings(
        autoConnect: autoConnect ?? this.autoConnect,
        lastDeviceAddress: lastDeviceAddress,
        bridgeEnabled: bridgeEnabled ?? this.bridgeEnabled,
      );

  Map<String, Object?> toMap() => {
    'autoConnect': autoConnect,
    'bridgeEnabled': bridgeEnabled,
  };
}
