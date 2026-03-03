// INITIALISIERUNG & BETRIEB
bool begin(); // Initialize sensor
void end(); // Gracefully close sensor
void debugOn(); // Enable debug flag
void debugOff(); // Disable debug flag
Response check(); // Process incoming frames (DATA/ACK/FAIL)

// MODUS-ABFRAGEN
bool inConfigMode(); // Sensor in config mode?
bool inBasicMode(); // Sensor in basic mode?
bool inEnhancedMode(); // Sensor in enhanced mode?

// PRÄSENZ & STATUS
byte getStatus(); // Presence status code
const char* statusString(); // Presence status as string
bool presenceDetected(); // Any presence detected?
bool stationaryTargetDetected(); // Stationary target detected?
bool movingTargetDetected(); // Moving target detected?
unsigned long detectedDistance(); // Detected distance [cm]

// STATIONARY TARGET
unsigned long stationaryTargetDistance(); // Distance [cm]
byte stationaryTargetSignal(); // Signal [0–100]
const ValuesArray& getStationarySignals(); // Gate signals

// MOVING TARGET
unsigned long movingTargetDistance(); // Distance [cm]
byte movingTargetSignal(); // Signal [0–100]
const ValuesArray& getMovingSignals(); // Gate signals

// ZEIT & ZÄHLER
unsigned long getTimestamp(); // Timestamp of last frame
unsigned long getFrameCount(); // Number of frames received

// MAC, FIRMWARE, VERSION
const byte* getMAC(); // Bluetooth MAC
String getMACstr(); // Bluetooth MAC as String
String getFirmware(); // Firmware as String
byte getFirmwareMajor(); // Firmware major
byte getFirmwareMinor(); // Firmware minor
unsigned long getVersion(); // Protocol version

// SENSOR-DATEN & AUFLÖSUNG
const SensorData& getSensorData(); // Full sensor data
byte getResolution(); // Gate width (20/75 cm)

// PARAMETER – GATES, RANGE, THRESHOLDS
const ValuesArray& getMovingThresholds(); // Moving thresholds
const ValuesArray& getStationaryThresholds(); // Stationary thresholds
byte getRange(); // Max detection gate
unsigned long getRange_cm(); // Max detection range [cm]
byte getNoOneWindow(); // No-presence timeout [s]
byte getMaxMovingGate(); // Max moving gate
byte getMaxStationaryGate(); // Max stationary gate

// REQUESTS / COMMANDS – MODUS
bool configMode(bool enable = true); // Enter/exit config mode
bool enhancedMode(bool enable = true); // Enter/exit enhanced mode

// REQUESTS / COMMANDS – AUTO-THRESHOLDS
bool autoThresholds(byte _timeout = 10); // Start auto-threshold routine
AutoStatus getAutoStatus(); // Status of auto-threshold routine

// REQUESTS / COMMANDS – INFORMATION
bool requestAuxConfig(); // Request auxiliary config
bool requestMAC(); // Request Bluetooth MAC
bool requestFirmware(); // Request firmware
bool requestResolution(); // Request resolution
bool requestParameters(); // Request all parameters

// REQUESTS / COMMANDS – GATE PARAMETER SETZEN
bool setGateParameters(byte gate, byte movingThreshold, byte stationaryThreshold); // One or all gates
bool setMovingThreshold(byte gate, byte movingThreshold); // Moving threshold
bool setStationaryThreshold(byte gate, byte stationaryThreshold); // Stationary threshold
bool setGateParameters(const ValuesArray& moving_thresholds, const ValuesArray& stationary_thresholds, byte noOneWindow = 5); // All gates + window
bool setMaxGate(byte movingGate, byte stationaryGate, byte noOneWindow = 5); // Max gates + window
bool setMaxMovingGate(byte movingGate); // Max moving gate
bool setMaxStationaryGate(byte stationaryGate); // Max stationary gate
bool setNoOneWindow(byte noOneWindow); // No-presence timeout
bool setResolution(bool fine = false); // Set resolution

// REQUESTS / COMMANDS – RESET & REBOOT
bool requestReset(); // Factory reset
bool requestReboot(); // Reboot sensor

// REQUESTS / COMMANDS – BLUETOOTH
bool requestBTon(); // Bluetooth ON
bool requestBToff(); // Bluetooth OFF
bool setBTpassword(const char *passwd); // Set BT password (c-string)
bool setBTpassword(const String &passwd); // Set BT password (String)
bool resetBTpassword(); // Reset BT password

// REQUESTS / COMMANDS – BAUDRATE
bool setBaud(byte baud); // Set baud rate (reboots sensor)

// AUXILIARY CONTROL / LIGHT CONTROL
byte getLightLevel(); // Light level
LightControl getLightControl(); // Light control enum
byte getLightThreshold(); // Light threshold
OutputControl getOutputControl(); // Output control enum
byte getOutLevel(); // Output light level
bool setAuxControl(LightControl light_control, byte light_threshold, OutputControl output_control); // Set aux control
bool resetAuxControl(); // Reset aux control