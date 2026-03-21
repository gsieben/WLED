#pragma once
/**
 * @file LD2420.h
 * @verbatim
  ____             ____       _     
 / ___| ___  ___  / ___| __ _| |__  
| |  _ / _ \/ _ \| |  _ / _` | '_ \ 
| |_| |  __/ (_) | |_| | (_| | |_) |
 \____|\___|\___/ \____|\__,_|_.__/ 
 * @endverbatim
 * @author Gabriel A. Sieben (GeoGab)
 * @brief WLED Usermod class declaration for the Hi-Link LD2420 mmWave radar sensor.
 * @version 1.0.0
 * @date 2026-03
 *
 * @details
 * ## File structure
 * | File               | Purpose                                              |
 * |--------------------|------------------------------------------------------|
 * | LD2420_config.h    | Pins, baud rate, debug level, logging macros         |
 * | LD2420.h           | Usermod class declaration (this file)                |
 * | LD2420.cpp         | Implementation                                       |
 *
 * ## Typical integration
 * Drop all three files into `wled/usermods/LD2420/` and add the usermod
 * to your WLED build. The calibration web UI (`calibration.html`) must be
 * uploaded to the device's LittleFS flash via the WLED editor.
 */

#include "wled.h"
#include <LD2420GeoGab.h>
#include "LD2420_config.h"


/**
 * @class UmLD2420GeoGab
 * @brief WLED Usermod driver for the Hi-Link LD2420 24 GHz mmWave radar sensor.
 *
 * @details
 * Integrates the LD2420GeoGab library into WLED. Provides:
 * - Presence and distance detection exposed on the WLED info page
 * - Home Assistant MQTT auto-discovery
 * - A web-based calibration UI served from LittleFS
 * - JSON GET/POST API for the calibration UI
 * - Automatic or manual ABD threshold configuration
 */
class UmLD2420GeoGab : public Usermod {

// =============================================================================
// PUBLIC — WLED interface
// =============================================================================
public:

  // ── Lifecycle ───────────────────────────────────────────────────────────────

  /** @brief Called once by WLED at boot. Initialises sensor and registers endpoints. */
  void setup();

  /** @brief Called repeatedly by WLED. Drives radar.update() and periodic tasks. */
  void loop();


  // ── WLED Config API ─────────────────────────────────────────────────────────

  /** @brief Returns the unique usermod ID used by WLED. */
  uint16_t getId();

  /**
   * @brief Load usermod settings from WLED cfg.json.
   * @details Detects UART changes and sets ReInitialize flag if needed.
   * @return true if the LD2420 config section was found and read successfully.
   */
  bool readFromConfig(JsonObject& root);

  /**
   * @brief Save usermod settings to WLED cfg.json.
   * @details Writes: enabled, rxpin, txpin, baudrate, interval, ha_discovery.
   */
  void addToConfig(JsonObject& root);

  /**
   * @brief Populate the WLED Usermod settings page with labels and dropdowns.
   */
  void appendConfigData();

  /**
   * @brief Add sensor info to the WLED info page (/json/info).
   * @details Shows presence status, distance, detection mode and firmware version.
   *          Shows last error message when in error state.
   *          Includes toggle button and link to calibration page.
   */
  void addToJsonInfo(JsonObject& root);

  /**
   * @brief Add sensor state to the WLED JSON state endpoint (/json/state).
   * @details Publishes: active, presence, distance, status (none/motion/presence), error.
   */
  void addToJsonState(JsonObject& root);

  /**
   * @brief React to JSON state updates from the WLED API.
   * @details Handles the "lh24xx" object to toggle the usermod on/off at runtime.
   */
  void readFromJsonState(JsonObject& root);


  // ── WLED Network Callbacks ──────────────────────────────────────────────────

  /** @brief Called by WLED when WiFi connects. Logs the assigned IP address. */
  void onWiFiConnect();

  /** @brief Called by WLED when WiFi disconnects. Clears MqttInitialized flag. */
  void onWiFiDisconnect();

  /**
   * @brief Called by WLED when MQTT connects.
   * @details Publishes HA discovery on the first connection (if enabled).
   * @param sessionPresent True if the broker resumed an existing session.
   */
  void onMqttConnect(bool sessionPresent) override;

  /**
   * @brief Called by WLED when MQTT disconnects.
   * @param reason Disconnect reason code from AsyncMqttClient.
   */
  void onMqttDisconnect(int8_t reason);


  // ── WLED Webserver Callbacks ─────────────────────────────────────────────────

  /**
   * @brief Serve the calibration HTML page from LittleFS.
   * @details Falls back to an inline 404 page with editor link if file is missing.
   */
  void handleWebRequest(AsyncWebServerRequest *request);

  /**
   * @brief JSON GET handler — supplies live data and calibration config to the UI.
   * @details Query modes via ?json=<mode>: all | live | calibration | stati
   */
  void handleJsonGet(AsyncWebServerRequest *request);

  /**
   * @brief JSON POST handler — receives commands from the calibration UI.
   * @details Supported commands: start_calibration, stop_calibration,
   *          auto_calibration, cancel_calibration, load_calibration,
   *          apply_calibration, factory_reset, restart_sensor, detect_baudrate.
   */
  void handleJsonPost(AsyncWebServerRequest *request, JsonVariant json);


// =============================================================================
// PRIVATE — internal implementation
// =============================================================================
private:

  // ── Internal Functions ──────────────────────────────────────────────────────

  /** @brief Process all pending pflags — called once per interval from loop(). */
  void FlagProcessor();

  /** @brief Open UART, call radar.begin(), read firmware, load calibration. */
  void initLD2420();

  /**
   * @brief Register all data callbacks on the radar object.
   * @details Sets up: presenceCb, statusCb, distanceCb, energyCb, calibrationCb.
   *          Called once from initLD2420().
   */
  void setupCallbacks();

  /** @brief Publish HA MQTT discovery payloads (once per boot). */
  void haDiscovery();

  /** @brief Publish current sensor state to MQTT topics. */
  void haPublishState();

  /**
   * @brief Register sensor error with a RAM string message.
   * @param msg Null-terminated error description.
   */
  void error(const char* msg);

  /**
   * @brief Register sensor error with a flash string message.
   * @param msg Flash-resident error description (F() macro).
   */
  void error(const __FlashStringHelper* msg);

  /** @brief Clear error state and empty lastMessage. */
  void errorClear();

  /**
   * @brief Try all known baud rates and return the first one that works.
   * @return Detected baud rate, or -1 if no rate produced a valid response.
   */
  int autoDetectBaudrate();

  /** @brief Format millis() as a fixed-width seconds string for log output. */
  String getMillisStamp();

  /** @brief Suspend WLED rendering and mark calibration mode active. */
  void enterCalibrationMode();

  /** @brief Resume WLED rendering and mark calibration mode inactive. */
  void exitCalibrationMode();

  /**
   * @brief Start the non-blocking auto-calibration routine.
   * @details Suspends WLED rendering. The calibrationCompleteCallback
   *          (registered in setupCallbacks) resumes it and stores the result.
   */
  void startAutoCalibrationRoutine();

  /**
   * @brief Write the local calibration struct to the sensor via writeABDConfig().
   * @details Enters and leaves config mode internally.
   */
  void writeCalibrationToSensor();

  /**
   * @brief Load ABD config from sensor flash into the local calibration struct.
   * @details Enters and leaves config mode internally.
   */
  void loadCalibrationFromSensor();

  /** @brief Register the GET and POST HTTP endpoints with the WLED webserver. */
  void registerEndpoints();


  // ── Settings ────────────────────────────────────────────────────────────────

  /**
   * @brief Persistent usermod settings — saved to / loaded from WLED cfg.json.
   */
  struct settings_t {
    bool     enabled      = LD2420_ENABLED;             ///< Usermod enabled/disabled
    uint8_t  Interval     = LD2420_INTERVAL;            ///< Loop interval in 0.1 s units
    uint32_t BaudRate     = LD2420_BAUDRATE;            ///< UART baud rate
    int8_t   rxpin        = LD2420_RXPIN;               ///< ESP32 RX pin (← sensor TX)
    int8_t   txpin        = LD2420_TXPIN;               ///< ESP32 TX pin (→ sensor RX)
    bool     HADiscovery  = LD2420_HA_DISCOVERY;        ///< Publish HA discovery on MQTT connect
    uint16_t autoCalibFrames = LD2420_CALIB_FRAMES;     ///< Frames to average for auto-calibration
    uint16_t autoCalibDelay  = LD2420_CALIB_DELAY_MS;   ///< Pre-calibration delay in ms
  } settings;


  // ── Status Flags ────────────────────────────────────────────────────────────

  /**
   * @brief Runtime status flags — reflect the current operational state.
   */
  struct sflags_t {
    bool InitSuccessful  = false; ///< True after a successful initLD2420()
    bool MqttInitialized = false; ///< True while MQTT client is connected
    bool HADiscoverySent = false; ///< True after HA discovery was published (once per boot)
    bool CalibrationMode = false; ///< True while calibration / Energy mode view is active
    bool Error           = false; ///< True while an error condition is registered
  } sflags;


  // ── Process Flags ───────────────────────────────────────────────────────────

  /**
   * @brief Deferred action flags — set from callbacks / web handlers, consumed by FlagProcessor().
   *
   * @details
   * No sensor or WLED operations are performed inside web callbacks.
   * Instead the appropriate flag is set and the action runs safely in loop().
   */
  struct pflags_t {
    bool RestartModule    = false; ///< Soft-reboot the LD2420 sensor
    bool FactoryReset     = false; ///< Perform a full factory reset on the sensor
    bool ReInitialize     = false; ///< Re-run initLD2420() (e.g. after UART settings change)
    bool AutoCalibration  = false; ///< Start the non-blocking auto-calibration routine
    bool StartCalibration = false; ///< Enter calibration / Energy mode view
    bool StopCalibration  = false; ///< Exit calibration / Energy mode view
    bool LoadCalibration  = false; ///< Reload ABD config from sensor into local struct
    bool WriteCalibration = false; ///< Write local calibration struct to sensor
  } pflags;


  // ── Runtime Values ──────────────────────────────────────────────────────────

  /**
   * @brief Live sensor values — updated by callbacks on every radar frame.
   */
  struct values_t {
    String   firmware     = "";            ///< Sensor firmware version string, e.g. "v1.6.1"
    String   sensorType   = LD2420_NAME;   ///< Sensor type label shown in WLED info page
    String   lastMessage  = "";            ///< Last error message (empty when no error)
    int      errorCounter = 0;             ///< Total number of errors since boot

    bool     presence     = false;                              ///< True if any target is currently detected
    LD2420DetectionStatus lastStatus = LD2420DetectionStatus::None; ///< Detection state: None / Motion / Presence
    uint16_t lastDistance = 0;                                  ///< Distance to strongest target in cm

    uint16_t gateEnergy[LD2420_TOTAL_GATES] = {};  ///< Per-gate signal energy from the latest Energy frame (Energy mode only)
  } values;


  // ── Measurement Timers ──────────────────────────────────────────────────────

  /**
   * @brief Timing state for the loop() interval throttle.
   */
  struct timer_t {
    uint32_t startMillis = 0; ///< millis() at setup() entry — for uptime calculation
    uint32_t actual      = 0; ///< millis() at the current loop() iteration
    uint32_t lastRun     = 0; ///< millis() at the last interval boundary
  } timer;


  // ── Calibration Data ────────────────────────────────────────────────────────

  /**
   * @brief Full ABD configuration block — mirrors the sensor's stored settings.
   *
   * @details
   * Populated by loadCalibrationFromSensor() via radar.readABDConfig().
   * Written back to the sensor by writeCalibrationToSensor() via radar.writeABDConfig().
   * Also updated automatically by the calibrationCompleteCallback after auto-calibration.
   *
   * Fields:
   * - roiMin / roiMax   — active detection gate range (each gate = 70 cm)
   * - delayTime         — presence hold-off in seconds after last detection
   * - highThresh[0..15] — per-gate motion trigger threshold
   * - lowThresh[0..15]  — per-gate presence maintain threshold
   */
  LD2420ABDConfig calibration;
};
