/**
 * @file usermod_GeoGab.cpp
 * @author Gabriel A. Sieben (GeoGab)
 * @brief WLED Usermod implementing the Hi-Link LD2410 mmWave radar sensor
 * @version 1.0.0
 * @date 25 Feb 2026
 */
 /* 
╔══════════╦═══════════╦═══════════╦══════════════╦════════════╦════════════════════════════════════════════╗
║ Model    ║ Supported ║ Bluetooth ║ Light Sensor ║ Gate Count ║ Special Features                           ║
╠══════════╬═══════════╬═══════════╬══════════════╬════════════╬════════════════════════════════════════════╣
║ LD2410   ║ ✔         ║ ❌       ║ ❌           ║ 9          ║ Standard model, no BLE                     ║
║ LD2410B  ║ ✔         ║ ✔        ║ ❌           ║ 9          ║ LD2410 with Bluetooth                      ║
║ LD2410C  ║ ✔         ║ ✔        ║ ✔            ║ 9          ║ Bluetooth + integrated light sensor        ║
║ LD2420   ║ ✔         ║ ❌       ║❌            ║ 15         ║ Higher resolution, more sensitive          ║
║ LD2450   ║ ❌        ║ ✔        ║ ❌           ║ 24 (3D)    ║ 3D radar, multi‑target tracking, BLE       ║
╚══════════╩═══════════╩═══════════╩══════════════╩════════════╩════════════════════════════════════════════╝
*/
 /* 
  TODOs: 
    - Auto Threshholds auch über save config aktivieren. 
    - AUXILIARY CONTROL einbauen
    - Bluetooth einbauen (GUI: password setzen... ) und info screen
    - Nach serial speed suchen (gui)
    - evl. anderen Serialkanal verwenden (gui) wenn man das MyLD2410 radar(Serial1); flexiebel machen kann
    - Serielle geschwindigkeit ändern (gui) 
    - ins info   String getMACstr();
    - ins info  String getFirmware();
    - Gates auslesen
*/

#include "wled.h"
#include "LD2410_full.h"
#include <MyLD2410.h>

class UsermodLD24xxGeoGab: public Usermod {
	public:
    /* Public: WLED Functions */
    void setup();								                      // * Setup of the user module called by wled main
    void loop();								                      // * Loop of the user module called by wled main in loop
    void addToConfig(JsonObject& root);               // * Add config entries to WLED config
    bool readFromConfig(JsonObject& root);            // * Read config from JSON
    void appendConfigData();                          // * Add config descriptions
    void addToJsonInfo(JsonObject& root);             // * Add info to WLED info page
    void readFromJsonState(JsonObject& root);         // * Reads button clicks 
    void addToJsonState(JsonObject& root);            // * Add State page

    /* Public: WLED Webserver Functions */
    void handleWebRequest(AsyncWebServerRequest *request);    // Schickt die HTML Seite die auf das flash gespeichert ist
    void handleJsonGet(AsyncWebServerRequest *request);       // Schickt Daten zur Calibration GUI
    void handleJsonPost(AsyncWebServerRequest *request, JsonVariant json);  // Erhält Daten von der Calibraton GUI
    uint16_t getId();                                 // * Get unique usermod ID

  private:
  	/* Private: Functions */
    void initLD24xx();                                // * Initialize sensor
    void checkLightControl();                         // * TODO: noch in write and read sensor config aufnehmen
    void readSensorData();                            // * Read sensor values
    void startEnhancedMode();                         // * Activates the calibration mode
    void stopEnhancedMode();                          // * De-activates the calibration mode
    void FlagProcessor();                             // * Subroutinen die auf Bedarf über die pflags aufgerufen werden
    void calculateAutoThresholds();                   // * Berechnet die Thresholds automaitsch
    void onMqttConnect(bool sessionPresent) override; // * Erst beim Connect den MQTT Nutzen
    void onMqttDisconnect(int8_t reason); // * Disconnect registirieren
    void haDiscovery();                               // * HomeAssistant Discovery mqtt push
    void haPublishState();                            // * Publich the sendor data.
    void error(const char* msg);                      // * Error Handling ran strings
    void error(const __FlashStringHelper* msg);       // * Overload für flash strings
    void errorClear();                                // * Clear the error status
    int autoDetectBaudrate();                         // * Detects the Baudrate 
    String getMillisStamp();                          // * Timestamp for the log

    /* Calibration Page Specials */
    void enterCalibrationMode();                      // * Enter the calibration mode
    void exitCalibrationMode();                       // * Exit he calibration mode
    void autoThresholds();                            // * Find auto thresholds
    void writeCalibrationToSensor();                  // * schreibt live auf den Sensor
    void loadCalibrationFromSensor();                 // * Load calibration values from sensor

    void registerEndpoints();                         // * Webserver: Endpukte registrieren

    /*** V A R I A B L E s  &  C O N S T A N T s ***/
    /* Private: Settings of Usermod BME68X wicht can be adapted in WLED usermods */
    struct settings_t {
      bool enabled = LD2410_ENABLED;                              // * Whether this usermod is enabled
		  uint8_t Interval = LD2410_INTERVAL; 	                      // * Interval of reading sensor data in seconds
      uint BaudRate = LD2410_BAUDRATE;                            // * Baud Rate of the Device 
      int8_t rxpin = LD2410_RXPIN;                                // * RX PIN > TX of LD2410
      int8_t txpin = LD2410_TXPIN;                                // * TX PIN > RX of LD2410
      bool HADiscovery = LD2410_HA_DISCOVERY;                     // * Publish Home Assistant Discovery messages
      /* Fixed settings (not in the GUI Setting page)             */
      bool autoThresholdsTimeout = LD2410_AUTOTRESHOLDS_TIMEOUT;  // * Setting timeout for AutoTrasholds
    } settings;

	  /* Private: Status Flags */
	  struct sflags_t {
		  bool InitSuccessful = false;  				// * Initialation was un-/successful
      bool MqttInitialized = false; 				// * MQTT Initialation done flag (first MQTT Connect)
      bool HADiscoverySent = false;         // * HA Dsicovery sucessfuly send
      bool CalibrationMode = false;         // * Calibration mode active
      bool EngineeringMode = false;         // * Engineering mode active
      bool AutoThresholdsRun = false;       // * Auto Thresholds is running
      bool HasLightControl = false;         // TDOD
      bool HasBluetooth = false;            // TODO
      bool Error = false;                   // * Da ist ein Fehler
    } sflags;

    /* Private: Processing flags that perform tasks in the loop */
    struct pflags_t {
      bool RestartModule       = false;   // * Restart WLED module
      bool FactoryReset        = false;   // * Factory reset the device
      bool ReInitialize        = false;   // * Reinitialize sensor (UART changes)
      bool AutoThresholds      = false;   // * Do Autotrashold until some result
      bool StartCalibration    = false;   // *
      bool StopCalibration     = false;   // *
      bool LoadCalibration     = false;   // * Load the Calibration Data from the sensor
      bool WriteCalibration    = false;   // * Write the Calibration Data to the sensor
    } pflags;

    /* Private: Internal Values and values of the Sensor */
    struct values_t {
      String firmware = "";               // TODO Firmware of the module
      String sensorType = LD24XX_NAME;    // TODO Sensor type LD2410, LD2410... (LD2450 not supported message)
      String version = "";                // TODO Software version -> update firmware message
      String BTmac = "";                  // Bluetooth MAC Adress
      int maxGates = 0;                   // Maximum existing gates
      
      String lastMessage = "";
      int errorCounter = 0;

      //bool hasBLE = false;            // Will be set once the device has a bluetooth function
      bool presence = false;
      bool moving = false;
      bool stationary = false;

      uint16_t presentsDistance = 0;
      uint16_t movingDistance = 0;
      uint16_t stationaryDistance = 0;

      byte movingSignal = 0;
      byte stationarySignale = 0;

      uint8_t movingEnergy[9];        // Nur für den EnhancedMode bei Calibration
      uint8_t staticEnergy[9];        // Nur für den EnhancedMode bei Calibration
    } values;

    /* Private: Measurement timers */
    struct timer_t {
      uint32_t ld24xx_startMillis = 0;    // Start time.
      uint32_t actual;  									// Actual time stamp
      uint32_t lastRun; 									// Last measurement time stamp
    } timer;

    struct calibration_t {
      uint8_t movingThresholds[9];        // Moving thresholds
      uint8_t stationaryThresholds[9];    // Stationary thresholds
      uint8_t maxMovingGate;              // 0–8
      uint8_t maxStationaryGate;          // 0–8
      uint8_t noOneWindow;                // Sekunden
      uint8_t resolution;                 // 0 = 75cm, 1 = 20cm (je nach Library)
    } calibration;

};    
/************************************************************************************************************/
/********************************************* M A I N  C O D E *********************************************/
/************************************************************************************************************/

// Constructor
MyLD2410 radar(SENSORSERIAL);

/**
 * @brief Called once by WLED during startup
 *
 * Initializes the LD2410 radar sensor if the usermod is enabled.
 * Any failure here prevents further processing in loop().
 */
void UsermodLD24xxGeoGab::setup() {
  timer.ld24xx_startMillis = millis();

  #ifdef LD24XX_DEBUG
    LD24XX_DBG.begin(115200);
    LD24XX_DBG.println(LD24XX_DNAME "Debug mode" GG_GREEN " active" GG_RES);
  #endif

  LD24XX_DPRINT("setup() started");

  if (!settings.enabled) {
    LD24XX_DPRINT("setup(): LD24xx usermod disabled by configuration.");
    error(F("LD24xx usermod disabled by configuration."));
    sflags.InitSuccessful = false;
    return;
  }

  initLD24xx();           // Init Sensor
  registerEndpoints();    // usermod Webside Calibration Page Endpoints

  LD24XX_DPRINT("setup():" GG_OK);
}

/**
 * @brief Main loop called repeatedly by WLED
 *
 */
void UsermodLD24xxGeoGab::loop()
{
  // Abort early if usermod is inactive or WLED is updating LEDs
  if (!settings.enabled || strip.isUpdating() || !sflags.InitSuccessful) return;

  // Normal Mode → Intervallbasiert
  if (!sflags.CalibrationMode) {
    timer.actual = millis();
    if (timer.actual - timer.lastRun < settings.Interval * 100) return;   // 0.1s units
    timer.lastRun = timer.actual;
  }

  #ifdef LD24XX_DEBUG
    timer.actual = millis();
    if (timer.actual - timer.lastRun < 500) return;      // In debug mode just every 500 second to not spam the serial port
    timer.lastRun = timer.actual;
  #endif

  // Read sensor data (includes radar.check)
  readSensorData();

  // Publish HA state (optional)
  if (!sflags.CalibrationMode) haPublishState();

  // Flags verarbeiten
  FlagProcessor();
}

void UsermodLD24xxGeoGab::FlagProcessor()
{
  // --- Restart module ------------------------------------------------------
  if (pflags.RestartModule) {
    pflags.RestartModule = false;
    LD24XX_DPRINT("FlagProcessor(): Sensor reboot requested by user");
    radar.requestReboot();
  }

  // --- Factory reset -------------------------------------------------------
  if (pflags.FactoryReset) {
    pflags.FactoryReset = false;
    LD24XX_DPRINT("FlagProcessor(): Sensor factory reset requested by user");
    radar.requestReset();
  }

  // --- Reinitialize sensor -------------------------------------------------
  if (pflags.ReInitialize) {
    pflags.ReInitialize = false;
    LD24XX_DPRINT("FlagProcessor(): Reinitialisation of the sensor");
    initLD24xx();
  }

  // --- Auto thresholds -----------------------------------------------------
  if (pflags.AutoThresholds) {
    // pflags.AutoThresholds = false;     // Darf hier nicht gesetzt werden!
    LD24XX_DPRINT("FlagProcessor(): Auto thresholds requested");
    calculateAutoThresholds();
  }

  // --- Calibration Mode Start ---------------------------------------------
  if (pflags.StartCalibration) {
    pflags.StartCalibration = false;
    LD24XX_DPRINT("FlagProcessor(): Entering calibration mode");
    enterCalibrationMode();
  }

  // --- Calibration Mode Stop ----------------------------------------------
  if (pflags.StopCalibration) {
    pflags.StopCalibration = false;
    LD24XX_DPRINT("FlagProcessor(): Exiting calibration mode");
    exitCalibrationMode();
  }

  if (pflags.LoadCalibration) {
    pflags.LoadCalibration = false;
    LD24XX_DPRINT("FlagProcessor(): Load Calibration Settings");
    loadCalibrationFromSensor();

  }
}

/*********************************************************************************************************/
/****************************************** WLED relatet routines ****************************************/
/*********************************************************************************************************/

/**
* @brief Called by WLED: Returns the unique usermod ID
*/
uint16_t UsermodLD24xxGeoGab::getId() {
  LD24XX_DPRINT("getId(): Was called.");
  return USERMOD_ID_LD2410_FULL;
}

/**
 * @brief Load LD2410 usermod configuration from JSON.
 *
 * Called:
 *  - At boot (WLED_FS_READY = false)
 *  - When saving settings in the Web-UI (WLED_FS_READY = true)
 *
 * Responsibilities:
 *  - Load settings
 *  - Detect changes
 *  - Set process flags (NO actions here!)
 */
bool UsermodLD24xxGeoGab::readFromConfig(JsonObject& root)
{
  LD24XX_DPRINT("readFromConfig(): Reading LD2410 config");

  JsonObject top = root["LD2410"];
  if (top.isNull()) {
    LD24XX_DPRINT("readFromConfig(): No LD2410 section found");
    return false;
  }

  // Backup old values to detect changes
  int8_t   oldRx   = settings.rxpin;
  int8_t   oldTx   = settings.txpin;
  uint32_t oldBaud = settings.BaudRate;

  // Load values with fallback defaults
  settings.enabled      = top["enabled"]      | LD2410_ENABLED;
  settings.rxpin        = top["rxpin"]        | LD2410_RXPIN;
  settings.txpin        = top["txpin"]        | LD2410_TXPIN;
  settings.BaudRate     = top["baudrate"]     | LD2410_BAUDRATE;
  settings.Interval     = top["interval"]     | LD2410_INTERVAL;
  settings.HADiscovery  = top["ha_discovery"] | LD2410_HA_DISCOVERY;

  // Detect UART changes (only when changed via Web-UI)
  if ((oldRx != settings.rxpin ||
       oldTx != settings.txpin ||
       oldBaud != settings.BaudRate))
  {
    LD24XX_DPRINT("readFromConfig(): UART settings changed -> Reinitialize sensor");
    pflags.ReInitialize = true;
  }

  LD24XX_DPRINT("readFromConfig():" GG_OK);
  return true;
}

/**
 * @brief Called by WLED: Add LD2410 usermod settings to cfg.json
 *
 * This function creates the configuration fields that appear
 * in the WLED Usermod settings page. Only persistent settings
 * belong here — NOT calibration data.
 */
void UsermodLD24xxGeoGab::addToConfig(JsonObject& root)
{
  LD24XX_DPRINT("addToConfig(): Adding LD2410 config entries");

  JsonObject top = root.createNestedObject("LD2410");

  top["enabled"]      = settings.enabled;
  top["rxpin"]        = settings.rxpin;
  top["txpin"]        = settings.txpin;
  top["baudrate"]     = settings.BaudRate;
  top["interval"]     = settings.Interval;
  top["ha_discovery"] = settings.HADiscovery;

  LD24XX_DPRINT("addToConfig():" GG_OK);
}


/**
 * @brief Called by WLED: Add configuration descriptions and dropdowns
 *
 * This function provides human‑readable labels and descriptions
 * for the config fields created in addToConfig().
 */
void UsermodLD24xxGeoGab::appendConfigData()
{
  LD24XX_DPRINT("appendConfigData(): Appending LD2410 config UI");

  oappend(F("dd=addDropdown('LD2410','enabled');"));
  oappend(F("addOption(dd,'Disabled',0);"));
  oappend(F("addOption(dd,'Enabled',1);"));
  oappend(F("addInfo('LD2410:RX Pin',1,'ESP32 UART RX Pin (LD24xx TX)');"));
  oappend(F("addInfo('LD2410:TX Pin',1,'ESP32 UART TX Pin (LD24xx RX)');"));
  oappend(F("dd=addDropdown('LD2410','Baudrate');"));
  oappend(F("addOption(dd,'9600',9600);"));
  oappend(F("addOption(dd,'19200',19200);"));
  oappend(F("addOption(dd,'38400',38400);"));
  oappend(F("addOption(dd,'57600',57600);"));
  oappend(F("addOption(dd,'115200',115200);"));
  oappend(F("addOption(dd,'256000',256000);"));
  oappend(F("addInfo('LD2410:Baudrate',1,'Serial baudrate (default 115200)');"));
  oappend(F("addInfo('LD2410:interval',1,'Polling interval in 0.1 sec');"));
  oappend(F("dd=addDropdown('LD2410','ha_discovery');"));
  oappend(F("addOption(dd,'Disabled',0);"));
  oappend(F("addOption(dd,'Enabled',1);"));

  LD24XX_DPRINT("appendConfigData():" GG_OK);
}

/**
 * @brief Called by WLED: Add usermod info to the WLED info page (/json/info)
 *
 */
void UsermodLD24xxGeoGab::addToJsonInfo(JsonObject& root)
{
  LD24XX_DPRINT("addToJsonInfo(): Adding to info page");
  JsonObject user = root["u"];
  if (user.isNull()) user = root.createNestedObject("u");

  /* Button */
  JsonArray line1 = user.createNestedArray(values.sensorType);  
  if (settings.enabled) {
    line1.add("<button class='btn btn-xs' onclick='requestJson({\"lh24xx\":{\"active\":false}});'><i class='icons on'>&#xe08f;</i></button>");
  } else {
    line1.add("<button class='btn btn-xs' onclick='requestJson({\"lh24xx\":{\"active\":true}});'><i class='icons off'>&#xe08f;</i></button>");
  }

  /* Link to the calibration page */
  JsonArray calibRow = user.createNestedArray("Calibration Page");
  calibRow.add("<a href='/ld24xx' style='text-decoration:underline;'>Click Here</a>");

  if (!settings.enabled) return;

  if (sflags.Error) {
    /* Not Enabled */
    JsonArray line2 = user.createNestedArray("Last Message");
    line2.add(values.lastMessage);
  } else {
    /* Presens */
    JsonArray line2 = user.createNestedArray("Presence");
    line2.add(values.presence ? "Erkannt" : "Keine");

    JsonArray line3 = user.createNestedArray("Presents Distance");
    line3.add(values.presentsDistance);
    line3.add("cm"); 

    /* Static */
    JsonArray line4 = user.createNestedArray("Static");
    line4.add(values.stationary ? "Erkannt" : "Keine");

    JsonArray line5 = user.createNestedArray("Static Distance");
    line5.add(values.stationaryDistance);
    line5.add("cm"); 

    JsonArray line6 = user.createNestedArray("Static Signal Strength");
    line6.add(values.stationarySignale);
    line6.add("%"); 

    /* Moving */
    JsonArray line7 = user.createNestedArray("Moving");
    line7.add(values.moving ? "Erkannt" : "Keine");

    JsonArray line8 = user.createNestedArray("Moving Distance");
    line8.add(values.movingDistance);
    line8.add("cm"); 

    JsonArray line9 = user.createNestedArray("Moving Signal Strength");
    line9.add(values.movingSignal);
    line9.add("%"); 
  }
  LD24XX_DPRINT("addToJsonInfo():" GG_OK);
}


/**
 * @brief Reads custom state data from the JSON API.
 * 
 * This method is called whenever WLED receives a JSON state update.
 * It checks if the "LH24xx" object exists and updates the enabled status.
 * 
 * @param root The root JSON object containing the state update.
 */
void UsermodLD24xxGeoGab::readFromJsonState(JsonObject& root) 
{
  LD24XX_DPRINT("readFromJsonState(): Reading state");
  // The key must match the label used in addToJsonInfo exactly
  JsonObject usermod = root["lh24xx"]; 
  if (!usermod.isNull()) {
    if (usermod.containsKey("active")) {
      bool newState = usermod["active"];
      if (newState != settings.enabled) {
        settings.enabled = newState;      
        pflags.ReInitialize = newState;   // if active reinitialize
        LD24XX_DPRINT_VAR("readFromJsonState(): Setting usermod active", newState ? GG_GREEN "on" GG_RES : GG_YELLOW "off" GG_RES);
      }
    }
  } 
  LD24XX_DPRINT("readFromJsonState():" GG_OK);
}


/**
 * @brief Called by WLED: Add LD2410 sensor data to the JSON state (/json/state)
 *
 * Provides:
 *  - Presence / Moving / Stationary flags
 *  - Moving & Static distances
 *  - Usermod enable/disable toggle button for Info page
 *
 * Uses only values stored in the `values` struct (deterministic).
 */
void UsermodLD24xxGeoGab::addToJsonState(JsonObject& root)
{
  JsonObject sensor = root.createNestedObject("ld24xx");

  if (settings.enabled) {
    LD24XX_DPRINT("addToJsonState(): Adding sensor date so /json/state");
    sensor["active"]            = settings.enabled;

    sensor["Presence"]          = values.presence;
    sensor["Presents Distance"] = values.presentsDistance;

    sensor["Moving"]            = values.moving;
    sensor["Moving Distance"]   = values.movingDistance;
    sensor["moving Signal"]     = values.movingSignal;

    sensor["Static"]            = values.stationary;
    sensor["Static Distance"]   = values.stationaryDistance;
    sensor["Static Signale"]    = values.stationarySignale;

    sensor["error"]             = sflags.Error;
  } else {
    sensor["active"]            = settings.enabled;
    LD24XX_DPRINT("addToJsonState(): LD24xx nicht aktive");
  }
  LD24XX_DPRINT("addToJsonState(): done" GG_OK);
}

/************************************************************************************************************/
/********************************************** S U B  C O D E **********************************************/
/************************************************************************************************************/
/**
 * @brief Simple Time Stamp
 *
*/
String UsermodLD24xxGeoGab::getMillisStamp() {
  uint32_t now = millis();
  // Sekunden mit 3 Nachkommastellen
  float sec = now / 1000.0f;

  char buf[16];
  snprintf(buf, sizeof(buf), "%8.3f", sec);  // z.B. "  12.347"
  return String(buf);
}


/**
 * @brief Initialize LD2410 / LD2420 radar sensor
 *
 * - Starts UART interface
 * - Initializes the MyLD2410 driver
 * - Reads firmware information
 * - Requests current sensor configuration
 *
 */
void UsermodLD24xxGeoGab::initLD24xx() {
  LD24XX_DPRINT("initLD24xx() Re/Initializing LD2410 radar");

  /* --- UART setup ---------------------------------------------------- */
  LD24XX_DPRINT_VAR("UART RX pin: ", settings.rxpin);
  LD24XX_DPRINT_VAR("UART TX pin: ", settings.txpin);

  SENSORSERIAL.begin(settings.BaudRate, SERIAL_8N1, settings.rxpin, settings.txpin);

  /* --- Sensor initialization ----------------------------------------- */
  if (!radar.begin()) {
    LD24XX_DPRINT("initLD24xx(): D2410 not detected on UART");
    error(F("Can't connect to the sensor"));
    sflags.InitSuccessful = false;

  } else {
    /* --- Firmware information ---------------------------------------- */
    values.firmware = radar.getFirmware();
    values.BTmac = radar.getMACstr();
    //values.hasBLE = radar._mac[0];

    LD24XX_DPRINT("initLD24xx(): Connected successfully to the sensor" GG_OK);
    LD24XX_DPRINT_VAR("initLD24xx(): Firmware version: ", values.firmware);

    loadCalibrationFromSensor();
    sflags.InitSuccessful = true;
    sflags.Error = false;
  }
  LD24XX_DPRINT("initLD24xx():" GG_OK);
}

/**
 * @brief Checks if there is an light control
 * 
 * TODO: Daten auslesen in read sensor
 * TODO: Eigentlich das in "write to sensor" und "read form sensor" aufnehmen
 */
void UsermodLD24xxGeoGab::checkLightControl() {
  radar.configMode();

  if (!radar.requestAuxConfig()) {
    LD24XX_DPRINT("readLightControlConfig(): AuxConfig request failed");
    radar.configMode(false);
    return;
  }

  // LightControl auswerten
  LightControl lc = radar.getLightControl();

  switch (lc) {
    case LightControl::NO_LIGHT_CONTROL:
      LD24XX_DPRINT("LightControl: none");
      sflags.HasLightControl = false;
      break;

    case LightControl::LIGHT_BELOW_THRESHOLD:
      LD24XX_DPRINT_ARG("LightControl: below threshold (%d)", radar.getLightThreshold());
      sflags.HasLightControl = true;
      break;

    case LightControl::LIGHT_ABOVE_THRESHOLD:
      LD24XX_DPRINT_ARG("LightControl: above threshold (%d)", radar.getLightThreshold());
      sflags.HasLightControl = true;
      break;

    default:
      LD24XX_DPRINT("LightControl: unknown");
      sflags.HasLightControl = false;
      break;
  }
  radar.configMode(false);
}


/**
 * @brief Read a complete LD2410 sensor frame and update all runtime values.
 *
 * This function processes both normal sensor data and enhanced-mode data.
 * It must be called frequently from loop() to keep the usermod state updated.
 *
 */
void UsermodLD24xxGeoGab::readSensorData()
{
  LD24XX_DPRINT("readSensorData(): Reading data");
  // 1) UART frame lesen
  MyLD2410::Response resp = radar.check();

  if (resp == MyLD2410::Response::FAIL) {
    LD24XX_DPRINT("readSensorData(): Kein gültiges Frame" GG_FAIL);
    return;
  }

  if (resp == MyLD2410::Response::ACK) {
    LD24XX_DPRINT("readSensorData(): ACK – Steuer-/Konfigurationsantwort" GG_FAIL);
    return;
  }

  // Ab hier: DATA-Frame
  LD24XX_DPRINT("readSensorData(): DATA – Sensordaten aktualisiert" GG_OK);

  // --- Normale Basisdaten ---------------------------------------------------
  values.presence   = radar.presenceDetected();
  values.moving     = radar.movingTargetDetected();
  values.stationary = radar.stationaryTargetDetected();

  values.movingDistance   = radar.movingTargetDistance();
  values.stationaryDistance   = radar.stationaryTargetDistance();
  values.presentsDistance = radar.detectedDistance();

  values.stationarySignale = radar.stationaryTargetSignal();
  values.movingSignal  = radar.movingTargetSignal();

  // --- Enhanced Mode: Gate-Energien ----------------------------------------
  if (radar.inEnhancedMode()) {
    const auto& mv = radar.getMovingSignals();
    const auto& st = radar.getStationarySignals();

    for (uint8_t gate = 0; gate < 9; gate++) {
      values.movingEnergy[gate]  = mv.values[gate];
      values.staticEnergy[gate]  = st.values[gate];
    }

    LD24XX_DPRINT("readSensorData();: Enhanced mode: gate energies updated");
  }

  // Debug-Ausgabe (optional)
  LD24XX_DPRINT("readSensorData(): Sensor data update:");
  LD24XX_DPRINT_VAR(GG_CC(10) "Presence:" GG_CC(10),         values.presence);
  LD24XX_DPRINT_VAR(GG_CC(10) "Moving:" GG_CC(10),            values.moving);
  LD24XX_DPRINT_VAR(GG_CC(10) "Stationary: " GG_CC(10),       values.stationary);
  LD24XX_DPRINT_VAR(GG_CC(10) "Moving dist:" GG_CC(10),       values.movingDistance);
  LD24XX_DPRINT_VAR(GG_CC(10) "Static dist:" GG_CC(10),       values.stationaryDistance);
  LD24XX_DPRINT_VAR(GG_CC(10) "Present dist:" GG_CC(10),      values.presentsDistance);
  LD24XX_DPRINT_VAR(GG_CC(10) "Moving signal:" GG_CC(10),    values.movingSignal);
  LD24XX_DPRINT_VAR(GG_CC(10) "Static signal:" GG_CC(10),    values.stationarySignale);
}


/**
 * @brief Called by WLED when the MQTT client has successfully connected.
 *
 * This is the correct moment to:
 *  - publish Home Assistant MQTT Discovery
 *  - send initial MQTT state values
 *  - mark internal flags indicating that MQTT is ready
 *
 * WLED guarantees that:
 *  - the global `mqtt` pointer is initialized
 *  - the MQTT client is fully connected
 *
 * Discovery is published only once per boot, controlled by sflags.HADiscoverySent.
 *
 * @param sessionPresent True if an existing MQTT session was resumed.
 */
void UsermodLD24xxGeoGab::onMqttConnect(bool sessionPresent)
{
    LD24XX_DPRINT("onMqttConnect(): MQTT Connected." GG_OK);
    sflags.MqttInitialized = true;
    if (!settings.HADiscovery) return;

    if (!sflags.HADiscoverySent) {
        haDiscovery();
        sflags.HADiscoverySent = true;
    }
}

/**
 * @brief Called when the MQTT connection is lost.
 *
 * This callback is invoked whenever the MQTT client disconnects from the
 * broker, regardless of the underlying cause. It is the correct place to
 * reset internal MQTT state flags so the usermod can safely reinitialize
 * or re-publish information on the next successful connection.
 *
 * Typical reasons for disconnection include:
 *  - network interruptions
 *  - broker restart
 *  - authentication failure
 *  - TCP timeout or transport error
 *
 * @param reason Disconnect reason code provided by AsyncMqttClient.
 */
void UsermodLD24xxGeoGab::onMqttDisconnect(int8_t reason)
{
    LD24XX_DPRINT("onMqttDisconnect(): MQTT Disconnected." GG_FAIL);
    // MQTT is no longer ready
    sflags.MqttInitialized = false;

    // Optional: allow HA discovery to be sent again on reconnect
    // sflags.HADiscoverySent = false;

    LD24XX_DPRINT_ARG("onMqttDisconnect(): Reason=%d", reason);
}


/**
 * @brief Publish Home Assistant MQTT Discovery configuration.
 *
 * Publishes one HA sensor per LD2410 value:
 */
void UsermodLD24xxGeoGab::haDiscovery()
{
  if (!settings.HADiscovery || !sflags.MqttInitialized) {
    LD24XX_DPRINT("haDiscovery(): HA Discovery not active");
    return;
  }
  
  LD24XX_DPRINT("haDiscovery(): Sending discovery block");

  // if (!mqtt) return;
  // if (!mqtt->connected()) return;

  const char* base = HADISCOVERYBASE;

  struct {
    const char* key;
    const char* name;
    const char* device_class;
    const char* unit;
    const char* icon;
  } sensors[] = {

    // --- Präsenz-Flags ------------------------------------------------------
    { "presence",        "LD2410 Presence",         "presence", nullptr, "mdi:motion-sensor" },
    { "moving",          "LD2410 Moving",           nullptr,    nullptr, "mdi:run" },
    { "stationary",      "LD2410 Stationary",       nullptr,    nullptr, "mdi:human-handsdown" },

    // --- Distanzen ----------------------------------------------------------
    { "movingDistance",  "LD2410 Moving Distance",  "distance", "cm",    "mdi:arrow-right-bold" },
    { "stationaryDistance",  "LD2410 Static Distance",  "distance", "cm",    "mdi:arrow-right" },
    { "presentsDistance","LD2410 Presence Distance","distance", "cm",    "mdi:map-marker-distance" },

    // --- Signalstärken ------------------------------------------------------
    { "movingSignal",    "LD2410 Moving Signal",    nullptr,    nullptr, "mdi:signal" },
    { "staticSignal",    "LD2410 Static Signal",    nullptr,    nullptr, "mdi:signal" },

    // --- Engineering Mode ---------------------------------------------------
    { "engMode",         "LD2410 Engineering Mode", nullptr,    nullptr, "mdi:cog" }
  };

  DynamicJsonDocument doc(2048);

  for (auto &s : sensors) {
    doc.clear();
    JsonObject p = doc.to<JsonObject>();

    p["name"]        = s.name;
    p["unique_id"]   = String("ld2410_") + s.key;
    p["state_topic"] = String("ld2410/") + s.key;

    if (s.device_class) p["device_class"] = s.device_class;
    if (s.unit)         p["unit_of_measurement"] = s.unit;
    if (s.icon)         p["icon"] = s.icon;

    JsonObject dev = p.createNestedObject("device");
    dev["identifiers"]  = "ld2410_sensor";
    dev["name"]         = "LD2410 Radar Sensor";
    dev["manufacturer"] = "HiLink";
    dev["model"]        = "LD2410";

    String payload;
    serializeJson(p, payload);

    String topic = String(base) + "/" + s.key + "/config";
    mqtt->publish(topic.c_str(), true, payload.c_str());

     
  }
  LD24XX_DPRINT("haDiscovery(): Published" GG_OK);
}


/**
 * @brief Publish current LD2410 sensor state values to MQTT.
 *
 * This function is intended to be called after each sensor update,
 * provided that Home Assistant discovery is enabled and MQTT has been
 * successfully initialized.
 */
void UsermodLD24xxGeoGab::haPublishState()
{
  if (!settings.HADiscovery) return;
  if (!sflags.MqttInitialized) return;

  LD24XX_DPRINT("haPublishState(): Publishing Sensor Data.");

  mqtt->publish("ld2410/presence",   true, values.presence   ? "1" : "0");
  mqtt->publish("ld2410/moving",     true, values.moving     ? "1" : "0");
  mqtt->publish("ld2410/stationary", true, values.stationary ? "1" : "0");

  String movingStr  = String(values.movingDistance);
  String staticStr  = String(values.stationaryDistance);

  mqtt->publish("ld2410/movingDistance", true, movingStr.c_str());
  mqtt->publish("ld2410/stationaryDistance", true, staticStr.c_str());

  mqtt->publish("ld2410/engMode", true, sflags.EngineeringMode ? "1" : "0");
}


/******************************************************************************************************** */
/************************************ Usermod Webpage relatet routines ************************************/
/******************************************************************************************************** */
/**
 * @brief Register an error condition with message (RAM string).
 */
void UsermodLD24xxGeoGab::error(const char* msg)
{
    sflags.Error = true;
    values.errorCounter++;
    values.lastMessage = msg;
    LD24XX_DPRINT_ARG("error(): %s (%d)" GG_FAIL, msg, values.errorCounter);
}

/**
 * @brief Register an error condition with message (Flash string).
 */
void UsermodLD24xxGeoGab::error(const __FlashStringHelper* msg)
{
    char buffer[128];
    strncpy_P(buffer, (PGM_P)msg, sizeof(buffer));
    buffer[sizeof(buffer)-1] = '\0';
    error(buffer);  // forward to RAM version
}

/**
 * @brief Clear the error status
 */
void UsermodLD24xxGeoGab::errorClear()
{
    sflags.Error = false;
    values.lastMessage = "";
    LD24XX_DPRINT("errorClear(): Error status cleared" GG_OK);
}


/**
 * @brief Enter calibration mode.
 *
 * Aktiviert einen Hochleistungsmodus für die Kalibrierung:
 *  - WLED Rendering deaktivieren (CPU frei)
 *  - Sensor-Polling beschleunigen
 *  - Engineering Mode aktivieren
 *  - HA-Publishing pausieren
 *  - Loop-Frequenz maximal
 *
 * Persistente Werte werden NICHT verändert.
 */
void UsermodLD24xxGeoGab::enterCalibrationMode()
{
  // Engineering Mode aktivieren
  LD24XX_DPRINT("enterCalibrationMode(): Activating calibration mode");

  // Already active?
  if (sflags.EngineeringMode) {
    LD24XX_DPRINT("enterCalibrationMode() Already active" GG_OK);
  } else {
    // Try to enable enhanced mode on the sensor
    if (!radar.enhancedMode(true)) {
      error(F("enterCalibrationMode() Activating enhancedMode failed." GG_FAIL));
      return;
    }
  }

  // Success
  strip.suspend();      // Strop WLED Effects to save processing power
  sflags.CalibrationMode = true;
  LD24XX_DPRINT("enterCalibrationMode(): Enhanced mode active" GG_OK);
}

/**
 * @brief Exit calibration mode.
 *
 * Stellt den Normalbetrieb wieder her:
 *  - WLED Rendering aktivieren
 *  - Sensor-Polling zurücksetzen
 *  - Engineering Mode deaktivieren
 *  - HA-Publishing wieder aktivieren
 */
void UsermodLD24xxGeoGab::exitCalibrationMode()
{
  LD24XX_DPRINT("exitCalibrationMode(): Deactivating enhanced mode");

  // Already inactive?
  if (!sflags.EngineeringMode) {
    LD24XX_DPRINT("exitCalibrationMode() Already inactive" GG_OK);
  } else {
    // Try to disable enhanced mode on the sensor
    if (!radar.enhancedMode(false)) {
      LD24XX_DPRINT("exitCalibrationMode(): Failed to deactivate enhanced mode" GG_FAIL) ;
      error("Failed to deactivate enhanced mode");
      return;
    }
  }
  
  strip.resume();        // WLED wieder aktivieren
  sflags.CalibrationMode = false;
  LD24XX_DPRINT("exitCalibrationMode(): Enhanced mode disabled" GG_FAIL) ;
}

/**
 * @brief Execute the LD2410 automatic threshold detection routine.
 *
 * This function is called exclusively from the FlagProcessor when the
 * AutoThresholds flag is set. It performs the complete auto-threshold
 * workflow synchronously:
 *
 *  - Starts the auto-threshold routine on the LD2410 sensor
 *    (supported only on firmware >= 2.44)
 *
 *  - Polls radar.getAutoStatus() until:
 *        * AutoStatus::COMPLETED  → success
 *        * AutoStatus::NOT_IN_PROGRESS / NOT_SET → sensor aborted → failure
 *        * Timeout (settings.autoThresholdsTimeout) → failure
 *
 *  - On success:
 *        * sflags.AutoThresholdsDone = true
 *
 *  - On failure:
 *        * error("ATr failed") is raised
 *
 *  - In all cases:
 *        * pflags.AutoThresholds is cleared before returning
 *
 * This function blocks until the routine finishes. It does not run in the
 * background and does not rely on loop() polling.
 */
void UsermodLD24xxGeoGab::calculateAutoThresholds()
{
    LD24XX_DPRINT("calculateAutoThresholds(): starting auto-threshold routine");

    // --- Start the auto-threshold routine -----------------------------------
    if(!sflags.AutoThresholdsRun) {
      if (!radar.autoThresholds(settings.autoThresholdsTimeout)) {
          LD24XX_DPRINT("calculateAutoThresholds(): rejected (Firmware < 2.44?)" GG_FAIL);
          error(F("Auto-Threshold rejected (Firmware < 2.44?)"));
          pflags.AutoThresholds = false;                // Stop the flag processor
          sflags.AutoThresholdsRun = true;              // AutoThresholds stoped
          return;
      } else {
          sflags.AutoThresholdsRun = true;
      }
    } else {
        AutoStatus st = radar.getAutoStatus();

        if (st == AutoStatus::NOT_IN_PROGRESS || st == AutoStatus::NOT_SET) return;

        if (st == AutoStatus::COMPLETED) {
            LD24XX_DPRINT("calculateAutoThresholds(): completed successfully" GG_OK);
            pflags.AutoThresholds = false;                // Stop the flag processor
            sflags.AutoThresholdsRun = true;              // AutoThresholds stoped
            pflags.LoadCalibration = true;                // Load the new calibration data
            errorClear();
            return;
        }
    }
}

/**
 * @brief Überträgt die komplette Kalibrierungsstruktur in den LD2410.
 *
 * Diese Funktion:
 *   1. Wechselt in den Konfigurationsmodus.
 *   2. Schreibt alle 9 Moving- und Stationary-Thresholds.
 *   3. Schreibt maxMovingGate, maxStationaryGate und noOneWindow.
 *   4. Schreibt die Auflösung.
 *   5. Verlässt den Konfigurationsmodus wieder.
 *   6. Optional: Führt einen Sensor-Neustart aus, falls die Library dies verlangt.
 *
 * Die Funktion aktiviert NICHT automatisch den Enhanced Mode.
 * Der Enhanced Mode wird ausschließlich durch Start/Stop-Calibration gesteuert.
 */
void UsermodLD24xxGeoGab::writeCalibrationToSensor()
{
  LD24XX_DPRINT("writeCalibrationToSensor(): begin");

  // 1) In den Konfigurationsmodus wechseln
  if (!radar.configMode(true)) {
    LD24XX_DPRINT("writeCalibrationToSensor(): cannot enter config mode" GG_FAIL);
    error(F("Cannot enter config mode"));
    return;
  }

  // 2) Thresholds für alle Gates setzen
  for (uint8_t gate = 0; gate < 9; gate++) {
    uint8_t mv = calibration.movingThresholds[gate];
    uint8_t st = calibration.stationaryThresholds[gate];
    LD24XX_DPRINT_ARG(GG_CC(10) "Gate %u: moving=%u stationary=%u", gate, mv, st);
    radar.setGateParameters(gate, mv, st);
  }

  // 3) Max Gates + No-One Window setzen
  LD24XX_DPRINT_ARG(GG_CC(10)"MaxMovingGate=%u", calibration.maxMovingGate);
  LD24XX_DPRINT_ARG(GG_CC(10)"MaxStationaryGate=%u", calibration.maxStationaryGate);
  LD24XX_DPRINT_ARG(GG_CC(10)"NoOneWindow=%u", calibration.noOneWindow);

  radar.setMaxGate(calibration.maxMovingGate, calibration.maxStationaryGate);
  radar.setNoOneWindow(calibration.noOneWindow);

  // 4) Auflösung setzen
  LD24XX_DPRINT_ARG(GG_CC(10)"Resolution=%u", calibration.resolution);
  radar.setResolution(calibration.resolution);

  // 5) Konfigurationsmodus verlassen
  radar.configMode(false);

  // 6) Optionaler Neustart (abhängig von Library)
  // radar.restart();   // Nur falls deine Library das verlangt

  LD24XX_DPRINT("writeCalibrationToSensor:" GG_OK);
}

/**
 * @brief Load calibration parameters from LD2410 sensor flash.
 *
 * Liest die persistent gespeicherten Kalibrierungswerte aus dem Sensor
 * und übernimmt sie in die lokale Struktur.
 */
void UsermodLD24xxGeoGab::loadCalibrationFromSensor()
{
  LD24XX_DPRINT("loadCalibrationFromSensor(): reading from sensor");

  // Alle Parameter vom Sensor anfordern
  radar.requestParameters();
  radar.requestResolution();

  // Moving thresholds
  const auto& mv = radar.getMovingThresholds();
  for (uint8_t gate = 0; gate < 9; gate++) {
    calibration.movingThresholds[gate] = mv.values[gate];
    LD24XX_DPRINT_ARG(GG_CC(10)"Gate %u movingThreshold = %u", gate, mv.values[gate]);
  }

  // Stationary thresholds
  const auto& st = radar.getStationaryThresholds();
  for (uint8_t gate = 0; gate < 9; gate++) {
    calibration.stationaryThresholds[gate] = st.values[gate];
    LD24XX_DPRINT_ARG(GG_CC(10)"Gate %u stationaryThreshold = %u", gate, st.values[gate]);
  }

  // Max gates
  calibration.maxMovingGate     = radar.getMaxMovingGate();
  calibration.maxStationaryGate = radar.getMaxStationaryGate();
  LD24XX_DPRINT_ARG(GG_CC(15)"maxMovingGate     = %u", calibration.maxMovingGate);
  LD24XX_DPRINT_ARG(GG_CC(15)"maxStationaryGate = %u", calibration.maxStationaryGate);

  // No-one window
  calibration.noOneWindow = radar.getNoOneWindow();
  LD24XX_DPRINT_ARG(GG_CC(15)"noOneWindow = %u", calibration.noOneWindow);

  // Resolution
  calibration.resolution = radar.getResolution();
  LD24XX_DPRINT_ARG(GG_CC(15)"resolution = %u", calibration.resolution);

  LD24XX_DPRINT("loadCalibrationFromSensor():" GG_OK);
}

/**
 * @brief Automatically detect the correct UART baudrate for the LD2410 sensor.
 *
 * Logic:
 *  1. If the current settings.baudrate already works with radar.begin(),
 *     no detection is needed and the current baudrate is returned.
 *
 *  2. Otherwise, iterate through a list of known-stable baudrates.
 *     For each baudrate:
 *       - Initialize Serial1 with that baudrate
 *       - Call radar.begin()
 *       - If radar.begin() succeeds, the baudrate is correct
 *
 *  3. If no baudrate works, return -1.
 *
 * This method is deterministic and relies solely on the LD2410 driver's
 * internal protocol validation. No custom packet parsing is required.
 *
 * @return int
 *   - Detected baudrate (9600–115200) on success
 *   - -1 if no working baudrate was found
 */
int UsermodLD24xxGeoGab::autoDetectBaudrate()
{
    // 1. First try the currently configured baudrate
    SENSORSERIAL.begin(settings.BaudRate, SERIAL_8N1, settings.rxpin, settings.txpin);
    delay(50);

    if (radar.begin()) {
        LD24XX_DPRINT_ARG("autoDetectBaudrate(): Baudrate: %d" GG_OK, settings.BaudRate);
        return settings.BaudRate;
    }

    // 2. Try known stable baudrates
    const int baudList[] = {9600, 19200, 38400, 57600, 115200, 256000};
    const int baudCount = sizeof(baudList) / sizeof(baudList[0]);

    for (int i = 0; i < baudCount; i++) {
        int baud = baudList[i];

        SENSORSERIAL.begin(baud, SERIAL_8N1, settings.rxpin, settings.txpin);
        delay(50);

        if (radar.begin()) {
            LD24XX_DPRINT_ARG("autoDetectBaudrate(): Auto-detected baudrate: %d" GG_OK, baud);
            return baud;
        }
    }

    LD24XX_DPRINT("autoDetectBaudrate(): Auto baudrate detection failed" GG_FAIL);
    return -1;
}


/******************************************************************************************************** */
/***************************************** Endpoints User Webpage *****************************************/
/******************************************************************************************************** */
/**
 * @brief Register LD2410 JSON API endpoints.
 *
 * Registriert:
 *  - GET  /json/ld2410  → Live-Daten
 *  - POST /json/ld2410  → Kommandos
 *  - Usermod Webpage: http://<wled-ip>/um/ld2410
 *
 * Wird typischerweise im setup() des Usermods aufgerufen.
 */
void UsermodLD24xxGeoGab::registerEndpoints()
{
    LD24XX_DPRINT("registerEndpoints(): Startet"); 
    // EIN EINZIGER GET-ENDPOINT für HTML + JSON
    server.on("/ld24xx", HTTP_GET,
        [this](AsyncWebServerRequest *request)
        {
            // JSON-Modus?
            if (request->hasParam("json")) {
                this->handleJsonGet(request);
                return;
            }

            // sonst: HTML-Webseite
            this->handleWebRequest(request);
        }
    );

    // POST-ENDPOINT bleibt wie er ist (JSON-API)
    server.on("/ld24xx/json", HTTP_POST,
        [this](AsyncWebServerRequest *request)
        {
            if (!request->hasParam("json", true)) {
                request->send(400, "application/json",
                              "{\"error\":\"missing json\"}");
                return;
            }

            String body = request->getParam("json", true)->value();

            DynamicJsonDocument doc(2048);
            DeserializationError err = deserializeJson(doc, body);
            if (err) {
                request->send(400, "application/json",
                              "{\"error\":\"invalid json\"}");
                return;
            }

            JsonVariant json = doc.as<JsonVariant>();
            this->handleJsonPost(request, json);
        }
    );
    LD24XX_DPRINT("registerEndpoints():" GG_OK); 
}


/**
 * @brief Memory-safe JSON GET handler for LD2410 data.
 * 
 * This version replaces AsyncJsonResponse with a direct DynamicJsonDocument 
 * to avoid heap allocation issues.
 * 
 * @param request Pointer to the AsyncWebServerRequest.
 */
void UsermodLD24xxGeoGab::handleJsonGet(AsyncWebServerRequest* request)
{
  LD24XX_DPRINT("handleJsonGet(): Startet"); 
  // --- MODE ERMITTELN ---
  String mode = "all";
  if (request->hasParam("json")) {
    String p = request->getParam("json")->value();
    if (p.length() > 0) mode = p;
  }

  // --- UNGÜLTIGE MODI ABFANGEN ---
  bool valid = mode == "all" || mode == "calibration" || mode == "values" || 
               mode == "pflags" || mode == "stati" || mode == "live";

  if (!valid) {
    request->send(400, "application/json", "{\"error\":\"invalid json mode\"}");
    LD24XX_DPRINT("handleJsonGet(): invalid json mode" GG_FAIL); 
    return;
  }

  // --- JSON RESPONSE ERZEUGEN ---
  // Wir nutzen direkt das Dokument, um Speicherprobleme zu umgehen
  DynamicJsonDocument doc(2048);
  JsonObject root = doc.to<JsonObject>();

  if (root.isNull()) {
    request->send(500, "application/json", "{\"error\":\"JSON buffer allocation failed\"}");
    LD24XX_DPRINT("handleJsonGet(): JSON buffer allocation failed" GG_FAIL); 
    return;
  }

  // --------------------------------------------------------------------------
  // SECTION: calibration
  // --------------------------------------------------------------------------
  if (mode == "calibration" || mode == "all") {
    LD24XX_DPRINT("handleJsonGet(): Calibration" GG_OK); 
    JsonObject cal = root.createNestedObject("calibration");
    JsonArray mv = cal.createNestedArray("movingThresholds");
    JsonArray st = cal.createNestedArray("stationaryThresholds");

    for (uint8_t i = 0; i < 9; i++) {
      mv.add(calibration.movingThresholds[i]);
      st.add(calibration.stationaryThresholds[i]);
    }
    cal["maxMovingGate"]     = calibration.maxMovingGate;
    cal["maxStationaryGate"] = calibration.maxStationaryGate;
    cal["noOneWindow"]       = calibration.noOneWindow;
    cal["resolution"]        = calibration.resolution;
  }

  // --------------------------------------------------------------------------
  // SECTION: values (live + static)
  // --------------------------------------------------------------------------
  if (mode == "values" || mode == "all") {
    LD24XX_DPRINT("handleJsonGet(): all " GG_OK); 
    JsonObject valuesObj = root.createNestedObject("values");
    JsonObject live = valuesObj.createNestedObject("live");
    live["presence"]           = values.presence;
    live["moving"]             = values.moving;
    live["stationary"]         = values.stationary;
    live["presentsDistance"]   = values.presentsDistance;
    live["movingDistance"]     = values.movingDistance;
    live["stationaryDistance"] = values.stationaryDistance;
    live["movingSignal"]       = values.movingSignal;
    live["staticSignal"]       = values.stationarySignale;

    JsonArray mvE = live.createNestedArray("movingEnergy");
    JsonArray stE = live.createNestedArray("staticEnergy");
    for (uint8_t i = 0; i < 9; i++) {
      mvE.add(values.movingEnergy[i]);
      stE.add(values.staticEnergy[i]);
    }

    JsonObject stat = valuesObj.createNestedObject("static");
    stat["firmwver"]     = values.firmware;
    stat["lastMessage"]  = values.lastMessage;
    stat["mac"]          = values.BTmac;
    stat["errorCounter"] = values.errorCounter;
  }

  // --------------------------------------------------------------------------
  // SECTION: pflags
  // --------------------------------------------------------------------------
  if (mode == "pflags" || mode == "all") {
    LD24XX_DPRINT("handleJsonGet(): pflags " GG_OK); 
    JsonObject pf = root.createNestedObject("pflags");
    pf["restartModule"]    = pflags.RestartModule;
    pf["factoryReset"]     = pflags.FactoryReset;
    pf["reInitialize"]     = pflags.ReInitialize;
    pf["autoThresholds"]   = pflags.AutoThresholds;
    pf["startCalibration"] = pflags.StartCalibration;
    pf["stopCalibration"]  = pflags.StopCalibration;
  }

  // --------------------------------------------------------------------------
  // SECTION: stati
  // --------------------------------------------------------------------------
  if (mode == "stati" || mode == "all") {
    LD24XX_DPRINT("handleJsonGet(): stati " GG_OK); 
    JsonObject st = root.createNestedObject("stati");
    st["initSuccessful"]      = sflags.InitSuccessful;
    st["mqttInitialized"]     = sflags.MqttInitialized;
    st["HADiscoverySent"]     = sflags.HADiscoverySent;
    st["calibrationMode"]     = sflags.CalibrationMode;
    st["engineeringMode"]     = sflags.EngineeringMode;
    st["autoThresholdsError"] = sflags.AutoThresholdsRun;
    st["error"]               = sflags.Error;
  }

  // --------------------------------------------------------------------------
  // SECTION: live (für schnelles Polling optimiert)
  // --------------------------------------------------------------------------
  if (mode == "live") { // "all" enthält live bereits via "values"
    LD24XX_DPRINT("handleJsonGet(): live " GG_OK); 
    JsonObject live = root.createNestedObject("live");
    live["presence"]           = values.presence;
    live["presentsDistance"]   = values.presentsDistance;
    live["movingSignal"]       = values.movingSignal;
    live["staticSignal"]       = values.stationarySignale;

    JsonArray mvE = live.createNestedArray("movingEnergy");
    JsonArray stE = live.createNestedArray("staticEnergy");
    for (uint8_t i = 0; i < 9; i++) {
      mvE.add(values.movingEnergy[i]);
      stE.add(values.staticEnergy[i]);
    }
  }

  // --- SEND RESPONSE ---
  String buffer;
  serializeJson(doc, buffer);
  request->send(200, "application/json", buffer);
  LD24XX_DPRINT("handleJsonGet(): done" GG_OK); 
}
 

/**
 * @brief Handle JSON POST requests for the LD2410 calibration interface.
 *
 * This handler supports only a minimal set of POST commands:
 *
 * @param request Pointer to the AsyncWebServerRequest.
 * @param json    Parsed JSON payload.
 */
void UsermodLD24xxGeoGab::handleJsonPost(AsyncWebServerRequest* request, JsonVariant json)
{
  LD24XX_DPRINT("handleJsonPost(): started"); 
  if (!json.is<JsonObject>()) {
    request->send(400, "application/json", "{\"error\":\"invalid json\"}");
    return;
  }

  JsonObject root = json.as<JsonObject>();
  const char* cmd = root["cmd"] | "";

  // --------------------------------------------------------------------------
  // Calibration mode control
  // --------------------------------------------------------------------------
  if (!strcmp(cmd, "start_calibration")) {
    LD24XX_DPRINT("handleJsonPost(): start_calibration" GG_OK); 
    pflags.StartCalibration = true;
  }
  else if (!strcmp(cmd, "stop_calibration")) {
    LD24XX_DPRINT("handleJsonPost(): stop_calibration" GG_OK); 
    pflags.StopCalibration = true;
  }
  else if (!strcmp(cmd, "auto_thresholds")) {
    LD24XX_DPRINT("handleJsonPost(): auto_thresholds" GG_OK); 
    pflags.AutoThresholds = true;
  }
  else if (!strcmp(cmd, "load_calibration")) {
    LD24XX_DPRINT("handleJsonPost(): load_calibration" GG_OK); 
    pflags.LoadCalibration = true;
  }

  // --------------------------------------------------------------------------
  // Apply full calibration set (flat JSON, matches calibration_t)
  // --------------------------------------------------------------------------
  else if (!strcmp(cmd, "apply_calibration")) {
    LD24XX_DPRINT("handleJsonPost(): apply_calibration" GG_OK); 

    // Moving thresholds
    if (root.containsKey("movingThresholds")) {
      JsonArray mv = root["movingThresholds"];
      if (mv.size() == 9) {
        for (int i = 0; i < 9; i++) {
          calibration.movingThresholds[i] = mv[i] | calibration.movingThresholds[i];
        }
      }
    }

    // Stationary thresholds
    if (root.containsKey("stationaryThresholds")) {
      JsonArray st = root["stationaryThresholds"];
      if (st.size() == 9) {
        for (int i = 0; i < 9; i++) {
          calibration.stationaryThresholds[i] = st[i] | calibration.stationaryThresholds[i];
        }
      }
    }

    // Scalar parameters
    calibration.maxMovingGate     = root["maxMovingGate"]     | calibration.maxMovingGate;
    calibration.maxStationaryGate = root["maxStationaryGate"] | calibration.maxStationaryGate;
    calibration.noOneWindow       = root["noOneWindow"]       | calibration.noOneWindow;
    calibration.resolution        = root["resolution"]        | calibration.resolution;

    // Request full write to sensor in main loop
    pflags.WriteCalibration = true;
  } 
  
  // --------------------------------------------------------------------------
  // Detect Baudrate
  // --------------------------------------------------------------------------
  else if (cmd == "detect_baudrate") {
    LD24XX_DPRINT("handleJsonPost(): detect_baudrate" GG_OK); 
    int detected = autoDetectBaudrate();

    if (detected > 0) {
        settings.BaudRate = detected;
        // TODO: WLED korrekt anweisen, die Settings zu speichern
        request->send(200, "application/json",
                      "{\"BaudRate\": " + String(detected) + "}");
    } else {
        request->send(500, "application/json", "{\"error\": \"no response\"}");
    }
    return;
  }

  // --------------------------------------------------------------------------
  // Unknown command
  // --------------------------------------------------------------------------
  else {
    request->send(400, "application/json", "{\"error\":\"unknown command\"}");
    LD24XX_DPRINT("handleJsonPost(): unknown command" GG_FAIL); 
    return;
  }

  // --------------------------------------------------------------------------
  // Default OK response
  // --------------------------------------------------------------------------
  request->send(200, "application/json", "{\"ok\":true}");
  LD24XX_DPRINT("handleJsonPost():" GG_OK); 
}


/**
 * @brief Handles web requests for the LD2410 UI with native WLED styling.
 * 
 * If the calibration file is missing, it serves a 404 page that 
 * links to the WLED internal stylesheet to maintain a consistent look.
 * 
 * @param request The pointer to the current AsyncWebServerRequest.
 */
void UsermodLD24xxGeoGab::handleWebRequest(AsyncWebServerRequest *request)
{
    LD24XX_DPRINT("handleWebRequest(): started"); 

    if (!request->url().equals(F("/ld24xx"))) return;

    const char* path = "/calibration.html";

    if (!LittleFS.exists(path)) {
      LD24XX_DPRINT("handleWebRequest(): File not found" GG_FAIL); 
      // Build an HTML response using WLED's own stylesheet
      String html = F("<!DOCTYPE html><html><head>");
      html += F("<meta name='viewport' content='width=device-width, initial-scale=1'>");
      html += F("<link rel='stylesheet' href='/style.css'>"); // Link to WLED system CSS
      html += F("<title>LD2410 Setup</title></head>");
      
      // 'back' provides the standard WLED dark-mode background
      html += F("<body class='back'><main class='container' style='text-align:center; padding-top:50px;'>");
      
      html += F("<h2>LD24xx Calibration</h2>");
      html += F("<p>The files for the calibration page are missing. They should be stored on the device's flash drive. You can find the files under “wled/usermods/LD21xx/device_filesystem/”. You can upload them via the WLED editor page.</p>");
      
      // 'btn' and 'btn-xs' are standard WLED button classes
      html += F("<a href='/edit' class='btn btn-xs'>Go to Editor</a>");
      html += F("<br><br>");
      html += F("<button onclick='history.back()' class='btn btn-xs'>Back to WLED</button>");
      
      html += F("</main></body></html>");

      request->send(200, "text/html", html);
        return;
    }

    request->send(LittleFS, path, "text/html");
    LD24XX_DPRINT("handleWebRequest():" GG_OK); 
}

static UsermodLD24xxGeoGab LD2010_full;
REGISTER_USERMOD(LD2010_full);
