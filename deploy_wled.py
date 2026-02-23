import subprocess
import re
import json
import sys
from pathlib import Path
from paramiko import SSHClient, AutoAddPolicy, RSAKey, Ed25519Key, ECDSAKey
from paramiko.ssh_exception import SSHException
from scp import SCPClient

# === Einstellungen ===
# Remote OpenWRT Ziel
HA_HOST = "10.2.2.253"          # "openwrt.geogab.lan"
HA_PORT = 7777
HA_USER = "root"
HA_PATH = "/www/wled"  # Remote-Zielordner

# SSH-Key für die Verbindung
SSH_KEY = Path.home() / ".ssh" / "openwrt"

# Build / Firmware Einstellungen
PLATFORMIO_ENV = "wled-GeoGab"
FIRMWARE_NAME = "firmware.bin"

FIRMWARE = Path(".pio") / "build" / PLATFORMIO_ENV / FIRMWARE_NAME
MANIFEST = Path("version.json")
MYCONFIG = Path("wled00") / "my_config.h"

# Manifest / URLs
MANIFEST_FILENAME = "version.json"
FIRMWARE_URL = f"http://{HA_HOST}:{HA_PORT}/{FIRMWARE_NAME}"

print("🔧 Versionsnummer wird erhöht...")

# === Version aus my_config.h lesen ===
if not MYCONFIG.exists():
    print(f"❌ my_config.h nicht gefunden: {MYCONFIG}")
    exit(1)

content = MYCONFIG.read_text(encoding="utf-8")

match = re.search(r'WLED_VERSION\s+"([^"]+)"', content)
if not match:
    print("❌ Konnte Version nicht finden!")
    exit(1)

current_version = match.group(1)
print(f"Aktuelle Version: {current_version}")

# === Versionsnummer erhöhen ===
m = re.match(r"(\d+\.\d+\.\d+)-(\d+)", current_version)
if m:
    base = m.group(1)
    num = int(m.group(2)) + 1
else:
    base = current_version
    num = 0

new_version = f"{base}-{num}"
print(f"➡️  Neue Version: {new_version}")

# === Version in my_config.h ersetzen ===
new_content = content.replace(current_version, new_version)

# === OTA URL in my_config.h setzen ===
print("🔗 Setze WLED_OTA_URL in my_config.h ...")

ota_url = f"http://{HA_HOST}:{HA_PORT}/{MANIFEST_FILENAME}"
ota_pattern = r'#define\s+WLED_OTA_URL\s+"[^"]*"'
new_ota_define = f'#define WLED_OTA_URL "{ota_url}"'

if re.search(ota_pattern, new_content):
    new_content = re.sub(ota_pattern, new_ota_define, new_content)
    print(f"➡️  WLED_OTA_URL aktualisiert: {ota_url}")
else:
    new_content += f'\n{new_ota_define}\n'
    print(f"➕ WLED_OTA_URL hinzugefügt: {ota_url}")

# Datei speichern
MYCONFIG.write_text(new_content, encoding="utf-8")

# === Manifest erzeugen ===
print("📄 Erzeuge Manifest...")

manifest_data = {
    "version": new_version,
    "url": FIRMWARE_URL
}

MANIFEST.write_text(json.dumps(manifest_data, indent=2), encoding="utf-8")

# === Firmware bauen ===
print(f"🛠️  Baue Firmware für Environment: {PLATFORMIO_ENV}")

try:
    subprocess.run(
        [sys.executable, "-m", "platformio", "run", "-e", PLATFORMIO_ENV],
        check=True
    )
except subprocess.CalledProcessError:
    print("❌ Build fehlgeschlagen! Bitte Fehler im PlatformIO-Output prüfen.")
    exit(1)

print("✅ Build erfolgreich abgeschlossen!")

# === Firmware prüfen ===
if not FIRMWARE.exists():
    print(f"❌ Firmware nicht gefunden: {FIRMWARE}")
    exit(1)

print(f"📦 Firmware gefunden: {FIRMWARE}")

# === SSH Key prüfen ===
if not SSH_KEY.exists():
    print(f"❌ SSH-Key nicht gefunden: {SSH_KEY}")
    exit(1)

print(f"🔑 Verwende SSH-Key: {SSH_KEY}")

# === Key laden ===
key = None
for key_class in (Ed25519Key, RSAKey, ECDSAKey):
    try:
        key = key_class.from_private_key_file(str(SSH_KEY))
        break
    except SSHException:
        continue

if key is None:
    print("❌ SSH-Key konnte nicht geladen werden. Falsches Format oder verschlüsselt?")
    exit(1)

# === SSH-Verbindung herstellen ===
print("🔌 Stelle SSH-Verbindung her...")

ssh = SSHClient()
ssh.set_missing_host_key_policy(AutoAddPolicy())

try:
    ssh.connect(HA_HOST, username=HA_USER, pkey=key)
except Exception as e:
    print(f"❌ SSH-Verbindung fehlgeschlagen: {e}")
    exit(1)

print("✅ SSH-Verbindung erfolgreich!")

# === Dateien übertragen ===
print("📤 Übertrage Dateien nach OpenWRT...")

try:
    with SCPClient(ssh.get_transport()) as scp:
        scp.put(str(FIRMWARE), f"{HA_PATH}/{FIRMWARE_NAME}")
        scp.put(str(MANIFEST), f"{HA_PATH}/{MANIFEST_FILENAME}")
except Exception as e:
    print(f"❌ SCP-Fehler: {e}")
    ssh.close()
    exit(1)

ssh.close()

print("🎉 Deployment erfolgreich abgeschlossen!")
print(f"📌 Neue Version: {new_version}")
print(f"📌 Firmware URL: {FIRMWARE_URL}")
