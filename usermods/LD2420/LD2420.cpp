/**
 * @file LD2420.cpp
 * @verbatim
  ____             ____       _     
 / ___| ___  ___  / ___| __ _| |__  
| |  _ / _ \/ _ \| |  _ / _` | '_ \ 
| |_| |  __/ (_) | |_| | (_| | |_) |
 \____|\___|\___/ \____|\__,_|_.__/ 
 * @endverbatim
 * @author Gabriel A. Sieben (GeoGab)
 * @brief WLED Usermod implementation for the Hi-Link LD2420 mmWave radar sensor.
 * @version 1.0.0
 * @date 2026-03
 *
 * @details
 * TODOs:
 *   - Bluetooth support (GUI: set password, info screen)
 *   - Serial speed auto-scan (GUI)
 *   - Change serial speed at runtime (GUI)
 */

#include "LD2420.h"

/************************************************************************************************************/
/********************************************* S E N S O R  I N S T A N C E *********************************/
/************************************************************************************************************/

/// Global LD2420GeoGab driver instance.
/// No constructor arguments — UART is configured inside begin() via initLD2420().
LD2420GeoGab radar;


/************************************************************************************************************/
/********************************************* W L E D  L I F E C Y C L E ***********************************/
/************************************************************************************************************/

/**
 * @brief Called once by WLED at boot.
 *
 * Initialises the LD2420 sensor if the usermod is enabled, then registers
 * the calibration web endpoints with the WLED HTTP server.
 */
void UmLD2420GeoGab::setup() {
  timer.startMillis = millis();
  UM_LOGI("setup() started");

  if (!settings.enabled) {
    UM_LOGW("setup(): usermod disabled by configuration — skipping init");
    error(F("LD2420 usermod disabled by configuration."));
    sflags.InitSuccessful = false;
    return;
  }

  initLD2420();
  registerEndpoints();

  UM_LOGI("setup() " GG_OK);
}

/**
 * @brief Called repeatedly by WLED.
 *
 * Always calls radar.update() to feed the UART ring buffer and fire callbacks.
 * Periodic tasks (HA publish, FlagProcessor) are throttled to settings.Interval.
 */
void UmLD2420GeoGab::loop() {
  if (!settings.enabled || strip.isUpdating() || !sflags.InitSuccessful) return;

  // Always drive the UART parser — callbacks fire here
  radar.update();

  // Throttle periodic tasks
  timer.actual = millis();

#if GG_DEBUG >= 1
  if (timer.actual - timer.lastRun < 500) return;   // slow down log spam in debug mode
#else
  if (timer.actual - timer.lastRun < (uint32_t)settings.Interval * 100) return;
#endif

  timer.lastRun = timer.actual;

  if (!sflags.CalibrationMode) haPublishState();

  FlagProcessor();
}


/************************************************************************************************************/
/********************************************* F L A G  P R O C E S S O R ***********************************/
/************************************************************************************************************/

/**
 * @brief Consume all pending pflags and execute the corresponding actions.
 *
 * No sensor operations run directly from web callbacks — they set a flag here
 * and the actual work happens safely in the loop() context.
 */
void UmLD2420GeoGab::FlagProcessor() {

  if (pflags.RestartModule) {
    pflags.RestartModule = false;
    UM_LOGW("FlagProcessor(): soft-rebooting sensor");
    radar.activateConfigMode();
    radar.restart();
  }

  if (pflags.FactoryReset) {
    pflags.FactoryReset = false;
    UM_LOGW("FlagProcessor(): factory reset requested");
    radar.activateConfigMode();
    radar.factoryReset();
  }

  if (pflags.ReInitialize) {
    pflags.ReInitialize = false;
    UM_LOGI("FlagProcessor(): reinitialising sensor");
    initLD2420();
  }

  if (pflags.StartCalibration) {
    pflags.StartCalibration = false;
    UM_LOGI("FlagProcessor(): entering calibration mode");
    enterCalibrationMode();
  }

  if (pflags.StopCalibration) {
    pflags.StopCalibration = false;
    UM_LOGI("FlagProcessor(): exiting calibration mode");
    exitCalibrationMode();
  }

  if (pflags.AutoCalibration) {
    pflags.AutoCalibration = false;
    UM_LOGI("FlagProcessor(): starting auto-calibration");
    startAutoCalibrationRoutine();
  }

  if (pflags.LoadCalibration) {
    pflags.LoadCalibration = false;
    UM_LOGI("FlagProcessor(): loading calibration from sensor");
    loadCalibrationFromSensor();
  }

  if (pflags.WriteCalibration) {
    pflags.WriteCalibration = false;
    UM_LOGI("FlagProcessor(): writing calibration to sensor");
    writeCalibrationToSensor();
  }
}


/************************************************************************************************************/
/********************************************* W L E D  C O N F I G  A P I **********************************/
/************************************************************************************************************/

uint16_t UmLD2420GeoGab::getId() {
  return USERMOD_ID_LD2420;
}

/**
 * @brief Load usermod settings from WLED cfg.json.
 *
 * Detects UART parameter changes (pin/baud) and sets the ReInitialize flag
 * so the sensor is reinitialised on the next loop() pass.
 */
bool UmLD2420GeoGab::readFromConfig(JsonObject& root) {
  UM_LOGI("readFromConfig(): reading config");

  JsonObject top = root["LD2420"];
  if (top.isNull()) {
    UM_LOGW("readFromConfig(): no LD2420 section found — using defaults");
    return false;
  }

  // Snapshot old UART settings to detect changes
  const int8_t   oldRx   = settings.rxpin;
  const int8_t   oldTx   = settings.txpin;
  const uint32_t oldBaud = settings.BaudRate;

  settings.enabled      = top["enabled"]      | LD2420_ENABLED;
  settings.rxpin        = top["rxpin"]        | LD2420_RXPIN;
  settings.txpin        = top["txpin"]        | LD2420_TXPIN;
  settings.BaudRate     = top["baudrate"]     | LD2420_BAUDRATE;
  settings.Interval     = top["interval"]     | LD2420_INTERVAL;
  settings.HADiscovery  = top["ha_discovery"] | LD2420_HA_DISCOVERY;

  if (oldRx != settings.rxpin || oldTx != settings.txpin || oldBaud != settings.BaudRate) {
    UM_LOGW("readFromConfig(): UART settings changed (rx=%d tx=%d baud=%u) — scheduling reinit",
            settings.rxpin, settings.txpin, settings.BaudRate);
    pflags.ReInitialize = true;
  }

  UM_LOGI("readFromConfig() " GG_OK);
  return true;
}

/**
 * @brief Save usermod settings to WLED cfg.json.
 */
void UmLD2420GeoGab::addToConfig(JsonObject& root) {
  UM_LOGI("addToConfig(): saving config");
  JsonObject top = root.createNestedObject("LD2420");
  top["enabled"]      = settings.enabled;
  top["rxpin"]        = settings.rxpin;
  top["txpin"]        = settings.txpin;
  top["baudrate"]     = settings.BaudRate;
  top["interval"]     = settings.Interval;
  top["ha_discovery"] = settings.HADiscovery;
  UM_LOGI("addToConfig() " GG_OK);
}

/**
 * @brief Populate the WLED Usermod settings page with labels and dropdowns.
 */
void UmLD2420GeoGab::appendConfigData() {
  oappend(F("dd=addDropdown('LD2420','enabled');"));
  oappend(F("addOption(dd,'Disabled',0);"));
  oappend(F("addOption(dd,'Enabled',1);"));
  oappend(F("addInfo('LD2420:rxpin',1,'ESP32 RX pin ← LD2420 TX');"));
  oappend(F("addInfo('LD2420:txpin',1,'ESP32 TX pin → LD2420 RX');"));
  oappend(F("dd=addDropdown('LD2420','baudrate');"));
  oappend(F("addOption(dd,'115200 (fw >= v1.5.3)',115200);"));
  oappend(F("addOption(dd,'256000 (fw < v1.5.3)',256000);"));
  oappend(F("addInfo('LD2420:interval',1,'Polling interval in 0.1 s units');"));
  oappend(F("dd=addDropdown('LD2420','ha_discovery');"));
  oappend(F("addOption(dd,'Disabled',0);"));
  oappend(F("addOption(dd,'Enabled',1);"));
}

/**
 * @brief Add sensor info to the WLED info page (/json/info).
 */
void UmLD2420GeoGab::addToJsonInfo(JsonObject& root) {
  JsonObject user = root["u"];
  if (user.isNull()) user = root.createNestedObject("u");

  // Toggle button
  JsonArray btnRow = user.createNestedArray(values.sensorType);
  if (settings.enabled) {
    btnRow.add("<button class='btn btn-xs' onclick='requestJson({\"lh24xx\":{\"active\":false}});'>"
               "<i class='icons on'>&#xe08f;</i></button>");
  } else {
    btnRow.add("<button class='btn btn-xs' onclick='requestJson({\"lh24xx\":{\"active\":true}});'>"
               "<i class='icons off'>&#xe08f;</i></button>");
  }

  // Link to calibration page
  JsonArray calRow = user.createNestedArray("Calibration");
  calRow.add("<a href='/LD2420' style='text-decoration:underline;'>Open Page</a>");

  if (!settings.enabled) return;

  if (sflags.Error) {
    JsonArray errRow = user.createNestedArray("Last Error");
    errRow.add(values.lastMessage);
    return;
  }

  // Presence
  JsonArray presRow = user.createNestedArray("Presence");
  presRow.add(values.presence ? "Detected" : "None");

  // Detection status
  JsonArray statRow = user.createNestedArray("Status");
  switch (values.lastStatus) {
    case LD2420DetectionStatus::Motion:   statRow.add("Motion");   break;
    case LD2420DetectionStatus::Presence: statRow.add("Presence"); break;
    default:                              statRow.add("None");     break;
  }

  // Distance
  JsonArray distRow = user.createNestedArray("Distance");
  distRow.add(values.lastDistance);
  distRow.add("cm");

  // Firmware
  JsonArray fwRow = user.createNestedArray("Firmware");
  fwRow.add(values.firmware);
}

/**
 * @brief Add sensor state to the WLED JSON state endpoint (/json/state).
 */
void UmLD2420GeoGab::addToJsonState(JsonObject& root) {
  JsonObject sensor = root.createNestedObject("LD2420");
  sensor["active"] = settings.enabled;

  if (!settings.enabled) return;

  sensor["presence"] = values.presence;
  sensor["distance"] = values.lastDistance;
  sensor["error"]    = sflags.Error;

  switch (values.lastStatus) {
    case LD2420DetectionStatus::Motion:   sensor["status"] = "motion";   break;
    case LD2420DetectionStatus::Presence: sensor["status"] = "presence"; break;
    default:                              sensor["status"] = "none";     break;
  }
}

/**
 * @brief React to JSON state updates from the WLED API.
 *
 * Handles the "lh24xx" object to toggle the usermod on/off at runtime.
 */
void UmLD2420GeoGab::readFromJsonState(JsonObject& root) {
  JsonObject usermod = root["lh24xx"];
  if (usermod.isNull()) return;

  if (usermod.containsKey("active")) {
    const bool newState = usermod["active"];
    if (newState != settings.enabled) {
      settings.enabled    = newState;
      pflags.ReInitialize = newState;   // reinit only when turning on
      UM_LOGI("readFromJsonState(): usermod %s", newState ? "enabled" : "disabled");
    }
  }
}


/************************************************************************************************************/
/********************************************* W L E D  N E T W O R K ***************************************/
/************************************************************************************************************/

void UmLD2420GeoGab::onWiFiConnect() {
  UM_LOGI("onWiFiConnect(): IP %s " GG_OK, Network.localIP().toString().c_str());
}

void UmLD2420GeoGab::onWiFiDisconnect() {
  UM_LOGW("onWiFiDisconnect(): connection lost");
  sflags.MqttInitialized = false;
}

void UmLD2420GeoGab::onMqttConnect(bool sessionPresent) {
  UM_LOGI("onMqttConnect() " GG_OK);
  sflags.MqttInitialized = true;
  if (settings.HADiscovery && !sflags.HADiscoverySent) {
    haDiscovery();
    sflags.HADiscoverySent = true;
  }
}

void UmLD2420GeoGab::onMqttDisconnect(int8_t reason) {
  UM_LOGW("onMqttDisconnect(): reason=%d", reason);
  sflags.MqttInitialized = false;
}


/************************************************************************************************************/
/********************************************* S E N S O R  I N I T *****************************************/
/************************************************************************************************************/

/**
 * @brief Register all data callbacks on the radar object.
 *
 * Called once from initLD2420(). All lambdas capture `this` so they can
 * write directly into the values struct.
 */
void UmLD2420GeoGab::setupCallbacks() {

  // Fired only on transitions: no target → target and back
  radar.setPresenceCallback([this](bool present) {
    values.presence = present;
    UM_LOGI("Presence: %s", present ? "detected" : "gone");
  });

  // Fired on every parsed frame — tracks fine-grained detection state
  radar.setStatusCallback([this](LD2420DetectionStatus status) {
    values.lastStatus = status;
  });

  // Fired on every frame that contains a non-zero distance
  radar.setDistanceCallback([this](uint16_t distanceCm) {
    values.lastDistance = distanceCm;
  });

  // Fired on every Energy frame — captures per-gate energies for calibration UI
  radar.setEnergyCallback([this](const LD2420EnergyFrame &frame) {
    for (uint8_t g = 0; g < LD2420_TOTAL_GATES; g++) {
      values.gateEnergy[g] = frame.gateEnergy[g];
    }
  });

  // Fired when startAutoCalibration() finishes or is cancelled
  radar.setCalibrationCompleteCallback([this](bool success, const LD2420ABDConfig &result) {
    if (success) {
      calibration = result;
      errorClear();
      UM_LOGI("Auto-calibration completed " GG_OK);
    } else {
      UM_LOGE("Auto-calibration failed or was cancelled");
      error(F("Auto-calibration failed or cancelled"));
    }
    strip.resume();
    sflags.CalibrationMode = false;
  });
}

/**
 * @brief Initialise the LD2420 sensor.
 *
 * Opens the UART, verifies sensor communication, reads firmware version,
 * switches to Energy mode and loads the current calibration from sensor flash.
 */
void UmLD2420GeoGab::initLD2420() {
  UM_LOGI("initLD2420(): TX=%d  RX=%d  baud=%u", settings.txpin, settings.rxpin, settings.BaudRate);

  setupCallbacks();

  if (!radar.begin(settings.txpin, settings.rxpin, settings.BaudRate)) {
    UM_LOGE("initLD2420(): sensor not detected on UART");
    error(F("Can't connect to the sensor"));
    sflags.InitSuccessful = false;
    return;
  }

  values.firmware = radar.getFirmwareVersion().versionStr;
  UM_LOGI("initLD2420(): firmware %s", values.firmware.c_str());

  // Energy mode gives us per-gate energies needed for the calibration UI
  radar.activateConfigMode();
  radar.setSystemMode(LD2420SystemMode::Energy);
  radar.deactivateConfigMode();

  loadCalibrationFromSensor();

  sflags.InitSuccessful = true;
  sflags.Error          = false;
  UM_LOGI("initLD2420() " GG_OK);
}


/************************************************************************************************************/
/********************************************* C A L I B R A T I O N ****************************************/
/************************************************************************************************************/

/**
 * @brief Enter calibration mode.
 *
 * Suspends WLED LED rendering to free CPU time for radar processing.
 * Energy mode is already active from initLD2420().
 */
void UmLD2420GeoGab::enterCalibrationMode() {
  if (sflags.CalibrationMode) {
    UM_LOGW("enterCalibrationMode(): already active — ignoring");
    return;
  }
  strip.suspend();
  sflags.CalibrationMode = true;
  UM_LOGI("enterCalibrationMode() " GG_OK);
}

/**
 * @brief Exit calibration mode and resume normal WLED operation.
 */
void UmLD2420GeoGab::exitCalibrationMode() {
  strip.resume();
  sflags.CalibrationMode = false;
  UM_LOGI("exitCalibrationMode() " GG_OK);
}

/**
 * @brief Start the non-blocking auto-calibration routine.
 *
 * Suspends WLED rendering for the duration.
 * Result is delivered via the calibrationCompleteCallback registered in setupCallbacks().
 */
void UmLD2420GeoGab::startAutoCalibrationRoutine() {
  if (radar.isCalibrating()) {
    UM_LOGW("startAutoCalibrationRoutine(): already running — ignoring");
    return;
  }

  strip.suspend();
  sflags.CalibrationMode = true;

  LD2420Error err = radar.startAutoCalibration(
    settings.autoCalibFrames,
    settings.autoCalibDelay,
    false,  // non-blocking — result arrives via calibrationCompleteCallback
    true    // skip gate 0 (strong near-field self-reflection)
  );

  if (err != LD2420Error::None) {
    UM_LOGE("startAutoCalibrationRoutine(): failed to start: %s",
            LD2420GeoGab::errorToString(err));
    error(F("Auto-calibration failed to start"));
    strip.resume();
    sflags.CalibrationMode = false;
  } else {
    UM_LOGI("startAutoCalibrationRoutine(): running — waiting for %u frames after %u ms delay",
            settings.autoCalibFrames, settings.autoCalibDelay);
  }
}

/**
 * @brief Write the local calibration struct to the sensor via writeABDConfig().
 */
void UmLD2420GeoGab::writeCalibrationToSensor() {
  LD2420Error err = radar.activateConfigMode();
  if (err != LD2420Error::None) {
    UM_LOGE("writeCalibrationToSensor(): cannot enter config mode: %s",
            LD2420GeoGab::errorToString(err));
    error(F("Cannot enter config mode"));
    return;
  }

  err = radar.writeABDConfig(calibration);
  radar.deactivateConfigMode();

  if (err != LD2420Error::None) {
    UM_LOGE("writeCalibrationToSensor(): writeABDConfig failed: %s",
            LD2420GeoGab::errorToString(err));
    error(F("writeABDConfig failed"));
  } else {
    UM_LOGI("writeCalibrationToSensor() " GG_OK);
  }
}

/**
 * @brief Load ABD config from sensor flash into the local calibration struct.
 */
void UmLD2420GeoGab::loadCalibrationFromSensor() {
  LD2420Error err = radar.activateConfigMode();
  if (err != LD2420Error::None) {
    UM_LOGE("loadCalibrationFromSensor(): cannot enter config mode: %s",
            LD2420GeoGab::errorToString(err));
    error(F("Cannot enter config mode"));
    return;
  }

  err = radar.readABDConfig(calibration);
  radar.deactivateConfigMode();

  if (err != LD2420Error::None) {
    UM_LOGE("loadCalibrationFromSensor(): readABDConfig failed: %s",
            LD2420GeoGab::errorToString(err));
    error(F("readABDConfig failed"));
    return;
  }

  UM_LOGI("loadCalibrationFromSensor(): roiMin=%lu  roiMax=%lu  delay=%lu s " GG_OK,
          calibration.roiMin, calibration.roiMax, calibration.delayTime);
}


/************************************************************************************************************/
/********************************************* U T I L I T Y *************************************************/
/************************************************************************************************************/

/**
 * @brief Try all known baud rates and return the first one that gets a valid response.
 * @return Detected baud rate, or -1 if no rate succeeded.
 */
int UmLD2420GeoGab::autoDetectBaudrate() {
  const uint32_t baudList[] = {115200, 256000, 57600, 38400, 19200, 9600};

  // Current setting first
  if (radar.begin(settings.txpin, settings.rxpin, settings.BaudRate)) {
    UM_LOGI("autoDetectBaudrate(): current baud %u works " GG_OK, settings.BaudRate);
    return (int)settings.BaudRate;
  }

  for (const uint32_t baud : baudList) {
    if (radar.begin(settings.txpin, settings.rxpin, baud)) {
      UM_LOGI("autoDetectBaudrate(): detected %u " GG_OK, baud);
      return (int)baud;
    }
  }

  UM_LOGE("autoDetectBaudrate(): no valid baud rate found");
  return -1;
}

String UmLD2420GeoGab::getMillisStamp() {
  char buf[16];
  snprintf(buf, sizeof(buf), "%8.3f", millis() / 1000.0f);
  return String(buf);
}

void UmLD2420GeoGab::error(const char* msg) {
  sflags.Error = true;
  values.errorCounter++;
  values.lastMessage = msg;
  UM_LOGE("error #%d: %s", values.errorCounter, msg);
}

void UmLD2420GeoGab::error(const __FlashStringHelper* msg) {
  char buffer[128];
  strncpy_P(buffer, (PGM_P)msg, sizeof(buffer));
  buffer[sizeof(buffer) - 1] = '\0';
  error(buffer);
}

void UmLD2420GeoGab::errorClear() {
  sflags.Error       = false;
  values.lastMessage = "";
  UM_LOGI("errorClear(): error state cleared " GG_OK);
}


/************************************************************************************************************/
/********************************************* M Q T T  /  H A *********************************************/
/************************************************************************************************************/

/**
 * @brief Publish Home Assistant MQTT discovery payloads.
 * Called once after the first MQTT connect (if ha_discovery is enabled).
 */
void UmLD2420GeoGab::haDiscovery() {
  if (!settings.HADiscovery || !sflags.MqttInitialized) {
    UM_LOGW("haDiscovery(): skipped (disabled or MQTT not ready)");
    return;
  }
  UM_LOGI("haDiscovery(): publishing");

  const char* base = HADISCOVERYBASE;

  struct SensorDef {
    const char* key;
    const char* name;
    const char* device_class;
    const char* unit;
    const char* icon;
  };

  static const SensorDef sensors[] = {
    { "presence", "LD2420 Presence", "presence", nullptr, "mdi:motion-sensor"       },
    { "status",   "LD2420 Status",   nullptr,    nullptr, "mdi:radar"                },
    { "distance", "LD2420 Distance", "distance", "cm",    "mdi:map-marker-distance"  },
  };

  DynamicJsonDocument doc(1024);
  for (const auto &s : sensors) {
    doc.clear();
    JsonObject p = doc.to<JsonObject>();
    p["name"]        = s.name;
    p["unique_id"]   = String("LD2420_") + s.key;
    p["state_topic"] = String("LD2420/") + s.key;
    if (s.device_class) p["device_class"]        = s.device_class;
    if (s.unit)         p["unit_of_measurement"]  = s.unit;
    if (s.icon)         p["icon"]                 = s.icon;

    JsonObject dev      = p.createNestedObject("device");
    dev["identifiers"]  = "LD2420_sensor";
    dev["name"]         = "LD2420 Radar Sensor";
    dev["manufacturer"] = "HiLink";
    dev["model"]        = "LD2420";

    String payload;
    serializeJson(p, payload);
    mqtt->publish((String(base) + "/" + s.key + "/config").c_str(), true, payload.c_str());
  }
  UM_LOGI("haDiscovery() " GG_OK);
}

/**
 * @brief Publish current sensor state to MQTT.
 * Called once per interval from loop() when not in calibration mode.
 */
void UmLD2420GeoGab::haPublishState() {
  if (!settings.HADiscovery || !sflags.MqttInitialized) return;

  mqtt->publish("LD2420/presence", true, values.presence ? "1" : "0");
  mqtt->publish("LD2420/distance", true, String(values.lastDistance).c_str());

  switch (values.lastStatus) {
    case LD2420DetectionStatus::Motion:   mqtt->publish("LD2420/status", true, "motion");   break;
    case LD2420DetectionStatus::Presence: mqtt->publish("LD2420/status", true, "presence"); break;
    default:                              mqtt->publish("LD2420/status", true, "none");     break;
  }
}


/************************************************************************************************************/
/********************************************* W E B S E R V E R ********************************************/
/************************************************************************************************************/

/**
 * @brief Register the GET and POST HTTP endpoints with the WLED webserver.
 */
void UmLD2420GeoGab::registerEndpoints() {
  UM_LOGI("registerEndpoints(): registering /LD2420 and /LD2420/json");

  // GET /LD2420         → HTML page (default) or JSON data (?json=<mode>)
  server.on("/LD2420", HTTP_GET,
    [this](AsyncWebServerRequest *request) {
      if (request->hasParam("json")) {
        this->handleJsonGet(request);
        return;
      }
      this->handleWebRequest(request);
    }
  );

  // POST /LD2420/json   → command API
  server.on("/LD2420/json", HTTP_POST,
    [this](AsyncWebServerRequest *request) {
      if (!request->hasParam("json", true)) {
        request->send(400, "application/json", "{\"error\":\"missing json body\"}");
        return;
      }
      DynamicJsonDocument doc(2048);
      if (deserializeJson(doc, request->getParam("json", true)->value())) {
        request->send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
      }
      this->handleJsonPost(request, doc.as<JsonVariant>());
    }
  );

  UM_LOGI("registerEndpoints() " GG_OK);
}

/**
 * @brief Serve the calibration HTML page from LittleFS.
 *
 * Falls back to an inline error page with a link to the WLED editor
 * if `/calibration.html` is not found on flash.
 */
void UmLD2420GeoGab::handleWebRequest(AsyncWebServerRequest *request) {
  if (!request->url().equals(F("/LD2420"))) return;

  const char* path = "/calibration.html";

  if (!LittleFS.exists(path)) {
    UM_LOGE("handleWebRequest(): /calibration.html not found on flash");
    String html;
    html += F("<!DOCTYPE html><html><head>");
    html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
    html += F("<link rel='stylesheet' href='/style.css'>");
    html += F("<title>LD2420 Setup</title></head>");
    html += F("<body class='back'><main class='container' style='text-align:center;padding-top:50px;'>");
    html += F("<h2>LD2420 Calibration</h2>");
    html += F("<p>The calibration page file is missing from flash.</p>");
    html += F("<p>Upload <code>calibration.html</code> via the WLED editor.</p>");
    html += F("<a href='/edit' class='btn btn-xs'>Open Editor</a>&nbsp;");
    html += F("<button onclick='history.back()' class='btn btn-xs'>Go Back</button>");
    html += F("</main></body></html>");
    request->send(200, "text/html", html);
    return;
  }

  request->send(LittleFS, path, "text/html");
  UM_LOGI("handleWebRequest(): served calibration.html " GG_OK);
}

/**
 * @brief JSON GET handler — supplies live data and calibration config to the UI.
 *
 * Query modes via ?json=<mode>:
 *  - live        → presence, status, distance, gate energies (fast-poll)
 *  - calibration → full ABD config (roiMin/Max, delayTime, thresholds)
 *  - stati       → internal status flags, firmware, error info
 *  - all         → all of the above combined
 */
void UmLD2420GeoGab::handleJsonGet(AsyncWebServerRequest* request) {
  String mode = "all";
  if (request->hasParam("json")) {
    const String p = request->getParam("json")->value();
    if (p.length() > 0) mode = p;
  }

  if (mode != "all" && mode != "live" && mode != "calibration" && mode != "stati") {
    UM_LOGW("handleJsonGet(): unknown mode '%s'", mode.c_str());
    request->send(400, "application/json", "{\"error\":\"unknown mode\"}");
    return;
  }

  DynamicJsonDocument doc(3072);
  JsonObject root = doc.to<JsonObject>();

  // ── live ──────────────────────────────────────────────────────────────────
  if (mode == "live" || mode == "all") {
    JsonObject live   = root.createNestedObject("live");
    live["presence"]  = values.presence;
    live["distance"]  = values.lastDistance;
    live["isCalib"]   = radar.isCalibrating();

    switch (values.lastStatus) {
      case LD2420DetectionStatus::Motion:   live["status"] = "motion";   break;
      case LD2420DetectionStatus::Presence: live["status"] = "presence"; break;
      default:                              live["status"] = "none";     break;
    }

    JsonArray ge = live.createNestedArray("gateEnergy");
    for (uint8_t i = 0; i < LD2420_TOTAL_GATES; i++) ge.add(values.gateEnergy[i]);
  }

  // ── calibration ───────────────────────────────────────────────────────────
  if (mode == "calibration" || mode == "all") {
    JsonObject cal   = root.createNestedObject("calibration");
    cal["roiMin"]    = calibration.roiMin;
    cal["roiMax"]    = calibration.roiMax;
    cal["delayTime"] = calibration.delayTime;

    JsonArray hi = cal.createNestedArray("highThresh");
    JsonArray lo = cal.createNestedArray("lowThresh");
    for (uint8_t i = 0; i < LD2420_TOTAL_GATES; i++) {
      hi.add(calibration.highThresh[i]);
      lo.add(calibration.lowThresh[i]);
    }
  }

  // ── stati ─────────────────────────────────────────────────────────────────
  if (mode == "stati" || mode == "all") {
    JsonObject st         = root.createNestedObject("stati");
    st["initSuccessful"]  = sflags.InitSuccessful;
    st["mqttInitialized"] = sflags.MqttInitialized;
    st["calibrationMode"] = sflags.CalibrationMode;
    st["isCalibrating"]   = radar.isCalibrating();
    st["error"]           = sflags.Error;
    st["lastMessage"]     = values.lastMessage;
    st["firmware"]        = values.firmware;
    st["errorCounter"]    = values.errorCounter;
  }

  String buffer;
  serializeJson(doc, buffer);
  request->send(200, "application/json", buffer);
  UM_LOGI("handleJsonGet(): mode=%s sent %u bytes", mode.c_str(), buffer.length());
}

/**
 * @brief JSON POST handler — receives commands from the calibration UI.
 *
 * All state-changing commands only set a pflag — the actual work happens
 * in FlagProcessor() on the next loop() pass.
 */
void UmLD2420GeoGab::handleJsonPost(AsyncWebServerRequest* request, JsonVariant json) {
  if (!json.is<JsonObject>()) {
    request->send(400, "application/json", "{\"error\":\"expected JSON object\"}");
    return;
  }

  JsonObject root   = json.as<JsonObject>();
  const char* cmd   = root["cmd"] | "";

  UM_LOGI("handleJsonPost(): cmd='%s'", cmd);

  // ── calibration mode control ──────────────────────────────────────────────
  if (!strcmp(cmd, "start_calibration")) {
    pflags.StartCalibration = true;

  } else if (!strcmp(cmd, "stop_calibration")) {
    pflags.StopCalibration = true;

  } else if (!strcmp(cmd, "auto_calibration")) {
    pflags.AutoCalibration = true;

  } else if (!strcmp(cmd, "cancel_calibration")) {
    radar.cancelAutoCalibration();
    strip.resume();
    sflags.CalibrationMode = false;
    UM_LOGW("handleJsonPost(): auto-calibration cancelled by user");

  } else if (!strcmp(cmd, "load_calibration")) {
    pflags.LoadCalibration = true;

  // ── apply full calibration block ──────────────────────────────────────────
  } else if (!strcmp(cmd, "apply_calibration")) {
    calibration.roiMin    = root["roiMin"]    | calibration.roiMin;
    calibration.roiMax    = root["roiMax"]    | calibration.roiMax;
    calibration.delayTime = root["delayTime"] | calibration.delayTime;

    if (root.containsKey("highThresh")) {
      JsonArray hi = root["highThresh"];
      for (uint8_t i = 0; i < LD2420_TOTAL_GATES && i < hi.size(); i++)
        calibration.highThresh[i] = hi[i] | calibration.highThresh[i];
    }
    if (root.containsKey("lowThresh")) {
      JsonArray lo = root["lowThresh"];
      for (uint8_t i = 0; i < LD2420_TOTAL_GATES && i < lo.size(); i++)
        calibration.lowThresh[i] = lo[i] | calibration.lowThresh[i];
    }
    pflags.WriteCalibration = true;
    UM_LOGI("handleJsonPost(): apply_calibration queued");

  // ── factory / restart ─────────────────────────────────────────────────────
  } else if (!strcmp(cmd, "factory_reset")) {
    UM_LOGW("handleJsonPost(): factory reset requested by user");
    pflags.FactoryReset = true;

  } else if (!strcmp(cmd, "restart_sensor")) {
    UM_LOGW("handleJsonPost(): sensor restart requested by user");
    pflags.RestartModule = true;

  // ── baud detection ────────────────────────────────────────────────────────
  } else if (!strcmp(cmd, "detect_baudrate")) {
    const int detected = autoDetectBaudrate();
    if (detected > 0) {
      settings.BaudRate = (uint32_t)detected;
      request->send(200, "application/json",
                    "{\"baudRate\":" + String(detected) + "}");
    } else {
      UM_LOGE("handleJsonPost(): baud rate detection failed");
      request->send(500, "application/json", "{\"error\":\"no sensor response\"}");
    }
    return;  // early return — response already sent

  // ── unknown ───────────────────────────────────────────────────────────────
  } else {
    UM_LOGW("handleJsonPost(): unknown command '%s'", cmd);
    request->send(400, "application/json", "{\"error\":\"unknown command\"}");
    return;
  }

  request->send(200, "application/json", "{\"ok\":true}");
}


/************************************************************************************************************/
/********************************************* R E G I S T R A T I O N **************************************/
/************************************************************************************************************/

static UmLD2420GeoGab LD2420_full;
REGISTER_USERMOD(LD2420_full);
