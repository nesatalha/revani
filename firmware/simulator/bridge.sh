#!/usr/bin/env bash
# USB köprüsü (adım 1.7): telefonu USB ile bağla, sonra bu betiği çalıştır.
# Simülatörü derler, adb reverse kurar ve köprü modunda açar.
#
#   firmware/simulator/bridge.sh            # varsayılan port 7878
#   PORT=7900 firmware/simulator/bridge.sh
#
# Ekran görüntüsü: başka bir terminalde `pkill -USR1 revani_sim`
# (dosya: firmware/simulator/build/bridge.bmp).
set -euo pipefail
cd "$(dirname "$0")"

PORT="${PORT:-7878}"
ADB="${ADB:-$(command -v adb || echo "$HOME/Library/Android/sdk/platform-tools/adb")}"

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build -j >/dev/null

if ! "$ADB" get-state >/dev/null 2>&1; then
  echo "Telefon bulunamadı: USB ile bağlayıp hata ayıklamaya izin ver." >&2
  exit 1
fi
"$ADB" reverse "tcp:$PORT" "tcp:$PORT" >/dev/null
echo "adb reverse tcp:$PORT hazır. Telefonda: Ayarlar → Simülatöre gönder, servis açık."
trap '"$ADB" reverse --remove "tcp:$PORT" >/dev/null 2>&1 || true' EXIT

./build/revani_sim --bridge --port "$PORT" --shot build/bridge.bmp
