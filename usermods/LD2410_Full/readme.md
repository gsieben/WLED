# LD2410 Full 

Gute Beschreibung des Moduls. Aber ohne den Softwareteil und ohne Details über Communication Protocol: https://www.waveshare.com/wiki/HMMD_mmWave_Sensor


| Model     | Supported | Bluetooth | Light Sensor | Gate Count | Special Features                         |
|-----------|-----------|-----------|--------------|------------|------------------------------------------|
| LD2410    | ✔ Yes     | ❌ No     | ❌ No        | 9          | Standard model, no BLE                  |
| LD2410B   | ✔ Yes     | ✔ Yes     | ❌ No        | 9          | LD2410 with Bluetooth                   |
| LD2410C   | ✔ Yes     | ✔ Yes     | ✔ Yes        | 9          | Bluetooth + integrated light sensor     |
| LD2420    | ✔ Yes     | ❌ No     | ❌ No        | 15         | Higher resolution, more sensitive       |
| LD2450    | ❌ No     | ✔ Yes     | ❌ No        | 24 (3D)    | 3D radar, multi‑target tracking, BLE    |


String fw = radar.getFirmware();
String model;

if (fw.indexOf("2410C") >= 0) model = "LD2410C";
else if (fw.indexOf("2410B") >= 0) model = "LD2410B";
else if (fw.indexOf("2410") >= 0)  model = "LD2410";
else if (fw.indexOf("2420") >= 0)  model = "LD2420";
else if (fw.indexOf("2450") >= 0)  model = "LD2450";
else model = "Unknown";


bool hasBluetooth = (model == "LD2410B" || model == "LD2410C" || model == "LD2450");
bool hasLightSensor = (model == "LD2410C");

