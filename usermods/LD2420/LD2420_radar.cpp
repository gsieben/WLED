/**
 * @file usermod_GeoGab.cpp
 * @author Gabriel A. Sieben (GeoGab)
 * @brief WLED Usermod implementing the Hi-Link LD2420 mmWave radar sensor
 * @version 1.0.0
 * @date 25 Feb 2026
 */
 /* 
  TODOs: 
    - Auto Threshholds auch über save config aktivieren. 
    - AUXILIARY CONTROL einbauen
    - Bluetooth einbauen (GUI: password setzen... ) und info screen
    - Nach serial speed suchen (gui)
    - evl. anderen Serialkanal verwenden (gui) wenn man das MyLD2420 radar(Serial1); flexiebel machen kann
    - Serielle geschwindigkeit ändern (gui) 
    - ins info   String getMACstr();
    - ins info  String getFirmware();
    - Gates auslesen
*/

#include "wled.h"
#include "LD2420_radar.h"
#include <ld2420.h>

class UmLD2420GeoGab: public Usermod {
	public:
    /* Public: WLED Functions */
    void setup();								                      // * Setup of the user module called by wled main
    void loop();								                      // * Loop of the user module called by wled main in loop

    /* Public: WLED Basic Functions */
    uint16_t getId();                                 // * Get unique usermod ID
    bool readFromConfig(JsonObject& root);            // * Read config from JSON
    void addToConfig(JsonObject& root);               // * Add config entries to WLED config
    void appendConfigData();                          // * Add config descriptions
    void addToJsonInfo(JsonObject& root);             // * Add info to WLED info page
    void addToJsonState(JsonObject& root);            // * Add State page
    void readFromJsonState(JsonObject& root);         // * React to the button click in the info screen.

    /* Public: WLED Special Functions*/
    void onWiFiConnect();   		                      // * Handel WIFI Connect things
    void onWiFiDisconnect();		                      // * dio 
    void onMqttConnect(bool sessionPresent) override; // * Erst beim Connect den MQTT Nutzen
    void onMqttDisconnect(int8_t reason);             // * Disconnect registirieren

    /* Public: WLED Webserver Functions */
    void handleWebRequest(AsyncWebServerRequest *request);    // Schickt die HTML Seite die auf das flash gespeichert ist
    void handleJsonGet(AsyncWebServerRequest *request);       // Schickt Daten zur Calibration GUI
    void handleJsonPost(AsyncWebServerRequest *request, JsonVariant json);  // Erhält Daten von der Calibraton GUI


  private:
  	/* Private: Functions */
    void FlagProcessor();                             // * Subroutinen die auf Bedarf über die pflags aufgerufen werden
    void initLD2420();                                // * Initialize sensor
    void checkLightControl();                         // * TODO: noch in write and read sensor config aufnehmen
    void readSensorData();                            // * Read sensor values
    void startEnhancedMode();                         // * Activates the calibration mode
    void stopEnhancedMode();                          // * De-activates the calibration mode
    void calculateAutoThresholds();                   // * Berechnet die Thresholds automaitsch
    void haDiscovery();                               // * HomeAssistant Discovery mqtt push
    void haPublishState();                            // * Publich the sendor data.
    void error(const char* msg);                      // * Error Handling ran strings
    void error(const __FlashStringHelper* msg);       // * Overload für flash strings
    void errorClear();                                // * Clear the error status
    int autoDetectBaudrate();                         // * Detects the Baudrate 
    String getMillisStamp();                          // * Timestamp for the log

    /* Private: Calibration Page Specials */
    void enterCalibrationMode();                      // * Enter the calibration mode
    void exitCalibrationMode();                       // * Exit he calibration mode
    void autoThresholds();                            // * Find auto thresholds
    void writeCalibrationToSensor();                  // * schreibt live auf den Sensor
    void loadCalibrationFromSensor();                 // * Load calibration values from sensor

    void registerEndpoints();                         // * Webserver: Endpukte registrieren

    /*** V A R I A B L E s  &  C O N S T A N T s ***/
    /* Private: Settings of Usermod BME68X wicht can be adapted in WLED usermods */

    struct settings_t {
      bool enabled = LD2420_ENABLED;                              // * Whether this usermod is enabled
		  uint8_t Interval = LD2420_INTERVAL; 	                      // * Interval of reading sensor data in seconds
      uint BaudRate = LD2420_BAUDRATE;                            // * Baud Rate of the Device 
      int8_t rxpin = LD2420_RXPIN;                                // * RX PIN -> should be connected to TX of LD2420 sensor
      int8_t txpin = LD2420_TXPIN;                                // * TX PIN > should be connected to RX of LD2420 sensor
      bool HADiscovery = LD2420_HA_DISCOVERY;                     // * Publish Home Assistant Discovery messages
      /* Fixed settings (not in the GUI Setting page)             */
      bool autoThresholdsTimeout = LD2420_AUTOTRESHOLDS_TIMEOUT;  // * Setting timeout for AutoTrasholds
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
      String sensorType = LD2420_NAME;    // TODO Sensor type LD2420, LD2420... (LD2450 not supported message)
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
      uint32_t LD2420_startMillis = 0;    // Start time.
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
LD2420 radar(SENSORSERIAL);

/**
 * @brief Called once by WLED during startup
 *
 * Initializes the LD2420 radar sensor if the usermod is enabled.
 * Any failure here prevents further processing in loop().
 */
void UmLD2420GeoGab::setup() {
  timer.LD2420_startMillis = millis();

  #ifdef LD2420_DEBUG
    LD2420_DBG.begin(115200);
    LD2420_DBG.println(LD2420_DNAME "Debug mode" GG_GREEN " active" GG_RES);
  #endif

  LD2420_DPRINT("setup() started");

  if (!settings.enabled) {
    LD2420_DPRINT("setup(): LD2420 usermod disabled by configuration.");
    error(F("LD2420 usermod disabled by configuration."));
    sflags.InitSuccessful = false;
    return;
  }

  initLD2420();           // Init Sensor
  registerEndpoints();    // usermod Webside Calibration Page Endpoints

  LD2420_DPRINT("setup():" GG_OK);
}

/**
 * @brief Main loop called repeatedly by WLED
 *
 */
void UmLD2420GeoGab::loop()
{
  // Abort early if usermod is inactive or WLED is updating LEDs
  if (!settings.enabled || strip.isUpdating() || !sflags.InitSuccessful) return;

  // Normal Mode → Intervallbasiert
  if (!sflags.CalibrationMode) {
    timer.actual = millis();
    if (timer.actual - timer.lastRun < settings.Interval * 100) return;   // 0.1s units
    timer.lastRun = timer.actual;
  }

  #ifdef LD2420_DEBUG
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

void UmLD2420GeoGab::FlagProcessor()
{
  // --- Restart module ------------------------------------------------------
  if (pflags.RestartModule) {
    pflags.RestartModule = false;
    LD2420_DPRINT("FlagProcessor(): Sensor reboot requested by user");
    radar.requestReboot();
  }

  // --- Factory reset -------------------------------------------------------
  if (pflags.FactoryReset) {
    pflags.FactoryReset = false;
    LD2420_DPRINT("FlagProcessor(): Sensor factory reset requested by user");
    radar.requestReset();
  }

  // --- Reinitialize sensor -------------------------------------------------
  if (pflags.ReInitialize) {
    pflags.ReInitialize = false;
    LD2420_DPRINT("FlagProcessor(): Reinitialisation of the sensor");
    initLD2420();
  }

  // --- Auto thresholds -----------------------------------------------------
  if (pflags.AutoThresholds) {
    // pflags.AutoThresholds = false;     // Darf hier nicht gesetzt werden!
    LD2420_DPRINT("FlagProcessor(): Auto thresholds requested");
    calculateAutoThresholds();
  }

  // --- Calibration Mode Start ---------------------------------------------
  if (pflags.StartCalibration) {
    pflags.StartCalibration = false;
    LD2420_DPRINT("FlagProcessor(): Entering calibration mode");
    enterCalibrationMode();
  }

  // --- Calibration Mode Stop ----------------------------------------------
  if (pflags.StopCalibration) {
    pflags.StopCalibration = false;
    LD2420_DPRINT("FlagProcessor(): Exiting calibration mode");
    exitCalibrationMode();
  }

  if (pflags.LoadCalibration) {
    pflags.LoadCalibration = false;
    LD2420_DPRINT("FlagProcessor(): Load Calibration Settings");
    loadCalibrationFromSensor();

  }
}

/******************************************************************************************************************************/
/****************************************** WLED relatet routines ****************************************/
/******************************************************************************************************************************/

/**
* @brief Called by WLED: Returns the unique usermod ID
*/
uint16_t UmLD2420GeoGab::getId() {
  LD2420_DPRINT("getId(): Was called.");
  return USERMOD_ID_LD2420;
}

/**
 * @brief Load LD2420 usermod configuration from JSON.
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
bool UmLD2420GeoGab::readFromConfig(JsonObject& root)
{
  LD2420_DPRINT("readFromConfig(): Reading LD2420 config");

  JsonObject top = root["LD2420"];
  if (top.isNull()) {
    LD2420_DPRINT("readFromConfig(): No LD2420 section found");
    return false;
  }

  // Backup old values to detect changes
  int8_t   oldRx   = settings.rxpin;
  int8_t   oldTx   = settings.txpin;
  uint32_t oldBaud = settings.BaudRate;

  // Load values with fallback defaults
  settings.enabled      = top["enabled"]      | LD2420_ENABLED;
  settings.rxpin        = top["rxpin"]        | LD2420_RXPIN;
  settings.txpin        = top["txpin"]        | LD2420_TXPIN;
  settings.BaudRate     = top["baudrate"]     | LD2420_BAUDRATE;
  settings.Interval     = top["interval"]     | LD2420_INTERVAL;
  settings.HADiscovery  = top["ha_discovery"] | LD2420_HA_DISCOVERY;

  // Detect UART changes (only when changed via Web-UI)
  if ((oldRx != settings.rxpin ||
       oldTx != settings.txpin ||
       oldBaud != settings.BaudRate))
  {
    LD2420_DPRINT("readFromConfig(): UART settings changed -> Reinitialize sensor");
    pflags.ReInitialize = true;
  }

  LD2420_DPRINT("readFromConfig():" GG_OK);
  return true;
}

/**
 * @brief Called by WLED: Add LD2420 usermod settings to cfg.json
 *
 * This function creates the configuration fields that appear
 * in the WLED Usermod settings page. Only persistent settings
 * belong here — NOT calibration data.
 */
void UmLD2420GeoGab::addToConfig(JsonObject& root)
{
  LD2420_DPRINT("addToConfig(): Adding LD2420 config entries");

  JsonObject top = root.createNestedObject("LD2420");

  top["enabled"]      = settings.enabled;
  top["rxpin"]        = settings.rxpin;
  top["txpin"]        = settings.txpin;
  top["baudrate"]     = settings.BaudRate;
  top["interval"]     = settings.Interval;
  top["ha_discovery"] = settings.HADiscovery;

  LD2420_DPRINT("addToConfig():" GG_OK);
}


/**
 * @brief Called by WLED: Add configuration descriptions and dropdowns
 *
 * This function provides human‑readable labels and descriptions
 * for the config fields created in addToConfig().
 */
void UmLD2420GeoGab::appendConfigData()
{
  LD2420_DPRINT("appendConfigData(): Appending LD2420 config UI");

  oappend(F("dd=addDropdown('LD2420','enabled');"));
  oappend(F("addOption(dd,'Disabled',0);"));
  oappend(F("addOption(dd,'Enabled',1);"));
  oappend(F("addInfo('LD2420:RX Pin',1,'ESP32 UART RX Pin (LD2420 TX)');"));
  oappend(F("addInfo('LD2420:TX Pin',1,'ESP32 UART TX Pin (LD2420 RX)');"));
  oappend(F("dd=addDropdown('LD2420','Baudrate');"));
  oappend(F("addOption(dd,'9600',9600);"));
  oappend(F("addOption(dd,'19200',19200);"));
  oappend(F("addOption(dd,'38400',38400);"));
  oappend(F("addOption(dd,'57600',57600);"));
  oappend(F("addOption(dd,'115200',115200);"));
  oappend(F("addOption(dd,'256000',256000);"));
  oappend(F("addInfo('LD2420:Baudrate',1,'Serial baudrate (default 115200)');"));
  oappend(F("addInfo('LD2420:interval',1,'Polling interval in 0.1 sec');"));
  oappend(F("dd=addDropdown('LD2420','ha_discovery');"));
  oappend(F("addOption(dd,'Disabled',0);"));
  oappend(F("addOption(dd,'Enabled',1);"));

  LD2420_DPRINT("appendConfigData():" GG_OK);
}

/**
 * @brief Called by WLED: Add usermod info to the WLED info page (/json/info)
 *
 */
void UmLD2420GeoGab::addToJsonInfo(JsonObject& root)
{
  LD2420_DPRINT("addToJsonInfo(): Adding to info page");
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
  calibRow.add("<a href='/LD2420' style='text-decoration:underline;'>Click Here</a>");

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
  LD2420_DPRINT("addToJsonInfo():" GG_OK);
}


/**
 * @brief Called by WLED: Add LD2420 sensor data to the JSON state (/json/state)
 *
 * Provides:
 *  - Presence / Moving / Stationary flags
 *  - Moving & Static distances
 *  - Usermod enable/disable toggle button for Info page
 *
 * Uses only values stored in the `values` struct (deterministic).
 */
void UmLD2420GeoGab::addToJsonState(JsonObject& root)
{
  JsonObject sensor = root.createNestedObject("LD2420");

  if (settings.enabled) {
    LD2420_DPRINT("addToJsonState(): Adding sensor date so /json/state");
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
    LD2420_DPRINT("addToJsonState(): LD2420 nicht aktive");
  }
  LD2420_DPRINT("addToJsonState(): done" GG_OK);
}

/**
 * @brief Reads custom state data from the JSON API.
 * 
 * This method is called whenever WLED receives a JSON state update.
 * It checks if the "LH24xx" object exists and updates the enabled status.
 * 
 * @param root The root JSON object containing the state update.
 */
void UmLD2420GeoGab::readFromJsonState(JsonObject& root) 
{
  LD2420_DPRINT("readFromJsonState(): Reading state");
  // The key must match the label used in addToJsonInfo exactly
  JsonObject usermod = root["lh24xx"]; 
  if (!usermod.isNull()) {
    if (usermod.containsKey("active")) {
      bool newState = usermod["active"];
      if (newState != settings.enabled) {
        settings.enabled = newState;      
        pflags.ReInitialize = newState;   // if active reinitialize
        LD2420_DPRINT_VAR("readFromJsonState(): Setting usermod active", newState ? GG_GREEN "on" GG_RES : GG_YELLOW "off" GG_RES);
      }
    }
  } 
  LD2420_DPRINT("readFromJsonState():" GG_OK);
}

/**
 * @brief Called by WLED when the device successfully connects to WiFi.
 *
 * This callback is triggered after DHCP has completed and a valid IP address
 * is assigned. At this point the network stack is fully operational and
 * network‑dependent features (MQTT, HTTP, HA Discovery, etc.) can be used.
 *
 * Typical use cases:
 * - Print debug information (IP, gateway, subnet)
 * - Trigger MQTT initialization
 * - Reset network‑related flags
 * - Start sensor communication that requires WiFi
 */
void UmLD2420GeoGab::onWiFiConnect() {
  IPAddress ip = Network.localIP();
  LD2420_DPRINT_ARG("onWiFiConnect() WiFi connected, IP: %s" GG_OK, ip.toString().c_str());
}

/**
 * @brief Called by WLED when the WiFi connection is lost.
 *
 * This callback is triggered whenever the station disconnects from the
 * access point. No valid IP is available at this point and all network
 * operations (MQTT, HTTP requests, HA Discovery) should be considered
 * unavailable until onWiFiConnect() fires again.
 *
 * Typical use cases:
 * - Print debug information
 * - Reset MQTT / HA flags
 * - Pause network‑dependent sensor operations
 */
void UmLD2420GeoGab::onWiFiDisconnect() {
  LD2420_DPRINT("onWiFiConnect() WiFi disconnected (station lost connection)" GG_FAIL);
  sflags.MqttInitialized = false;
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
void UmLD2420GeoGab::onMqttConnect(bool sessionPresent)
{
    LD2420_DPRINT("onMqttConnect(): MQTT Connected." GG_OK);
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
void UmLD2420GeoGab::onMqttDisconnect(int8_t reason)
{
    LD2420_DPRINT("onMqttDisconnect(): MQTT Disconnected." GG_FAIL);
    // MQTT is no longer ready
    sflags.MqttInitialized = false;

    // Optional: allow HA discovery to be sent again on reconnect
    // sflags.HADiscoverySent = false;

    LD2420_DPRINT_ARG("onMqttDisconnect(): Reason=%d", reason);
}

/******************************************************************************************************************************/
/********************************************** U S E R M O D  S U B  C O D E *************************************************/
/******************************************************************************************************************************/
/**
 * @brief Simple time index stamp for the debug log
 *
*/
String UmLD2420GeoGab::getMillisStamp() {
  uint32_t now = millis();
  // Sekunden mit 3 Nachkommastellen
  float sec = now / 1000.0f;

  char buf[16];
  snprintf(buf, sizeof(buf), "%8.3f", sec);  // z.B. "  12.347"
  return String(buf);
}


/**
 * @brief Initialize LD2420 / LD2420 radar sensor
 *
 * - Starts UART interface
 * - Initializes the MyLD2420 driver
 * - Reads firmware information
 * - Requests current sensor configuration
 *
 */
void UmLD2420GeoGab::initLD2420() {
  LD2420_DPRINT("initLD2420() Re/Initializing LD2420 radar");

  /* --- UART setup ---------------------------------------------------- */
  LD2420_DPRINT_ARG("initLD2420(): UART RX pin: %d -> TX Pin of the sensor", settings.rxpin);
  LD2420_DPRINT_ARG("initLD2420(): UART TX pin: %d -> RX Pin of the sensor", settings.txpin);

  SENSORSERIAL.begin(settings.BaudRate, SERIAL_8N1, settings.rxpin, settings.txpin);

  /* --- Sensor initialization ----------------------------------------- */
  if (!radar.begin()) {
    LD2420_DPRINT("initLD2420(): D2410 not detected on UART");
    error(F("Can't connect to the sensor"));
    sflags.InitSuccessful = false;
    

  } else {
    /* --- Firmware information ---------------------------------------- */
    values.firmware = radar.getFirmware();
    values.BTmac = radar.getMACstr();
    //values.hasBLE = radar._mac[0];

    LD2420_DPRINT("initLD2420(): Connected successfully to the sensor" GG_OK);
    LD2420_DPRINT_VAR("initLD2420(): Firmware version: ", values.firmware);

    loadCalibrationFromSensor();
    sflags.InitSuccessful = true;
    sflags.Error = false;
  }
  LD2420_DPRINT("initLD2420():" GG_OK);
}

/**
 * @brief Checks if there is an light control
 * 
 * TODO: Daten auslesen in read sensor
 * TODO: Eigentlich das in "write to sensor" und "read form sensor" aufnehmen
 */
void UmLD2420GeoGab::checkLightControl() {
  radar.configMode();

  if (!radar.requestAuxConfig()) {
    LD2420_DPRINT("readLightControlConfig(): AuxConfig request failed");
    radar.configMode(false);
    return;
  }

  // LightControl auswerten
  LightControl lc = radar.getLightControl();

  switch (lc) {
    case LightControl::NO_LIGHT_CONTROL:
      LD2420_DPRINT("LightControl: none");
      sflags.HasLightControl = false;
      break;

    case LightControl::LIGHT_BELOW_THRESHOLD:
      LD2420_DPRINT_ARG("LightControl: below threshold (%d)", radar.getLightThreshold());
      sflags.HasLightControl = true;
      break;

    case LightControl::LIGHT_ABOVE_THRESHOLD:
      LD2420_DPRINT_ARG("LightControl: above threshold (%d)", radar.getLightThreshold());
      sflags.HasLightControl = true;
      break;

    default:
      LD2420_DPRINT("LightControl: unknown");
      sflags.HasLightControl = false;
      break;
  }
  radar.configMode(false);
}


/**
 * @brief Read a complete LD2420 sensor frame and update all runtime values.
 *
 * This function processes both normal sensor data and enhanced-mode data.
 * It must be called frequently from loop() to keep the usermod state updated.
 *
 */
void UmLD2420GeoGab::readSensorData()
{
  LD2420_DPRINT("readSensorData(): Reading data");
  // 1) UART frame lesen
  MyLD2420::Response resp = radar.check();

  if (resp == MyLD2420::Response::FAIL) {
    LD2420_DPRINT("readSensorData(): Kein gültiges Frame" GG_FAIL);
    return;
  }

  if (resp == MyLD2420::Response::ACK) {
    LD2420_DPRINT("readSensorData(): ACK – Steuer-/Konfigurationsantwort" GG_FAIL);
    return;
  }

  // Ab hier: DATA-Frame
  LD2420_DPRINT("readSensorData(): DATA – Sensordaten aktualisiert" GG_OK);

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

    LD2420_DPRINT("readSensorData();: Enhanced mode: gate energies updated");
  }

  // Debug-Ausgabe (optional)
  LD2420_DPRINT("readSensorData(): Sensor data update:");
  LD2420_DPRINT_VAR(GG_CC(10) "Presence:" GG_CC(10),         values.presence);
  LD2420_DPRINT_VAR(GG_CC(10) "Moving:" GG_CC(10),            values.moving);
  LD2420_DPRINT_VAR(GG_CC(10) "Stationary: " GG_CC(10),       values.stationary);
  LD2420_DPRINT_VAR(GG_CC(10) "Moving dist:" GG_CC(10),       values.movingDistance);
  LD2420_DPRINT_VAR(GG_CC(10) "Static dist:" GG_CC(10),       values.stationaryDistance);
  LD2420_DPRINT_VAR(GG_CC(10) "Present dist:" GG_CC(10),      values.presentsDistance);
  LD2420_DPRINT_VAR(GG_CC(10) "Moving signal:" GG_CC(10),    values.movingSignal);
  LD2420_DPRINT_VAR(GG_CC(10) "Static signal:" GG_CC(10),    values.stationarySignale);
}



/**
 * @brief Publish Home Assistant MQTT Discovery configuration.
 *
 * Publishes one HA sensor per LD2420 value:
 */
void UmLD2420GeoGab::haDiscovery()
{
  if (!settings.HADiscovery || !sflags.MqttInitialized) {
    LD2420_DPRINT("haDiscovery(): HA Discovery not active");
    return;
  }
  
  LD2420_DPRINT("haDiscovery(): Sending discovery block");

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
    { "presence",        "LD2420 Presence",         "presence", nullptr, "mdi:motion-sensor" },
    { "moving",          "LD2420 Moving",           nullptr,    nullptr, "mdi:run" },
    { "stationary",      "LD2420 Stationary",       nullptr,    nullptr, "mdi:human-handsdown" },

    // --- Distanzen ----------------------------------------------------------
    { "movingDistance",  "LD2420 Moving Distance",  "distance", "cm",    "mdi:arrow-right-bold" },
    { "stationaryDistance",  "LD2420 Static Distance",  "distance", "cm",    "mdi:arrow-right" },
    { "presentsDistance","LD2420 Presence Distance","distance", "cm",    "mdi:map-marker-distance" },

    // --- Signalstärken ------------------------------------------------------
    { "movingSignal",    "LD2420 Moving Signal",    nullptr,    nullptr, "mdi:signal" },
    { "staticSignal",    "LD2420 Static Signal",    nullptr,    nullptr, "mdi:signal" },

    // --- Engineering Mode ---------------------------------------------------
    { "engMode",         "LD2420 Engineering Mode", nullptr,    nullptr, "mdi:cog" }
  };

  DynamicJsonDocument doc(2048);

  for (auto &s : sensors) {
    doc.clear();
    JsonObject p = doc.to<JsonObject>();

    p["name"]        = s.name;
    p["unique_id"]   = String("LD2420_") + s.key;
    p["state_topic"] = String("LD2420/") + s.key;

    if (s.device_class) p["device_class"] = s.device_class;
    if (s.unit)         p["unit_of_measurement"] = s.unit;
    if (s.icon)         p["icon"] = s.icon;

    JsonObject dev = p.createNestedObject("device");
    dev["identifiers"]  = "LD2420_sensor";
    dev["name"]         = "LD2420 Radar Sensor";
    dev["manufacturer"] = "HiLink";
    dev["model"]        = "LD2420";

    String payload;
    serializeJson(p, payload);

    String topic = String(base) + "/" + s.key + "/config";
    mqtt->publish(topic.c_str(), true, payload.c_str());   
  }
  LD2420_DPRINT("haDiscovery(): Published" GG_OK);
}


/**
 * @brief Publish current LD2420 sensor state values to MQTT.
 *
 * This function is intended to be called after each sensor update,
 * provided that Home Assistant discovery is enabled and MQTT has been
 * successfully initialized.
 */
void UmLD2420GeoGab::haPublishState()
{
  if (!settings.HADiscovery) return;
  if (!sflags.MqttInitialized) return;

  LD2420_DPRINT("haPublishState(): Publishing Sensor Data.");

  mqtt->publish("LD2420/presence",   true, values.presence   ? "1" : "0");
  mqtt->publish("LD2420/moving",     true, values.moving     ? "1" : "0");
  mqtt->publish("LD2420/stationary", true, values.stationary ? "1" : "0");

  String movingStr  = String(values.movingDistance);
  String staticStr  = String(values.stationaryDistance);

  mqtt->publish("LD2420/movingDistance", true, movingStr.c_str());
  mqtt->publish("LD2420/stationaryDistance", true, staticStr.c_str());

  mqtt->publish("LD2420/engMode", true, sflags.EngineeringMode ? "1" : "0");
}


/******************************************************************************************************************************/
/********************************************** Usermod Webpage relatet routines **********************************************/
/******************************************************************************************************************************/
/**
 * @brief Register an error condition with message (RAM string).
 */
void UmLD2420GeoGab::error(const char* msg)
{
    sflags.Error = true;
    values.errorCounter++;
    values.lastMessage = msg;
    LD2420_DPRINT_ARG("error(): %s (%d)" GG_FAIL, msg, values.errorCounter);
}

/**
 * @brief Register an error condition with message (Flash string).
 */
void UmLD2420GeoGab::error(const __FlashStringHelper* msg)
{
    char buffer[128];
    strncpy_P(buffer, (PGM_P)msg, sizeof(buffer));
    buffer[sizeof(buffer)-1] = '\0';
    error(buffer);  // forward to RAM version
}

/**
 * @brief Clear the error status
 */
void UmLD2420GeoGab::errorClear()
{
    sflags.Error = false;
    values.lastMessage = "";
    LD2420_DPRINT("errorClear(): Error status cleared" GG_OK);
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
void UmLD2420GeoGab::enterCalibrationMode()
{
  // Engineering Mode aktivieren
  LD2420_DPRINT("enterCalibrationMode(): Activating calibration mode");

  // Already active?
  if (sflags.EngineeringMode) {
    LD2420_DPRINT("enterCalibrationMode() Already active" GG_OK);
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
  LD2420_DPRINT("enterCalibrationMode(): Enhanced mode active" GG_OK);
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
void UmLD2420GeoGab::exitCalibrationMode()
{
  LD2420_DPRINT("exitCalibrationMode(): Deactivating enhanced mode");

  // Already inactive?
  if (!sflags.EngineeringMode) {
    LD2420_DPRINT("exitCalibrationMode() Already inactive" GG_OK);
  } else {
    // Try to disable enhanced mode on the sensor
    if (!radar.enhancedMode(false)) {
      LD2420_DPRINT("exitCalibrationMode(): Failed to deactivate enhanced mode" GG_FAIL) ;
      error("Failed to deactivate enhanced mode");
      return;
    }
  }
  
  strip.resume();        // WLED wieder aktivieren
  sflags.CalibrationMode = false;
  LD2420_DPRINT("exitCalibrationMode(): Enhanced mode disabled" GG_FAIL) ;
}

/**
 * @brief Execute the LD2420 automatic threshold detection routine.
 *
 * This function is called exclusively from the FlagProcessor when the
 * AutoThresholds flag is set. It performs the complete auto-threshold
 * workflow synchronously:
 *
 *  - Starts the auto-threshold routine on the LD2420 sensor
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
void UmLD2420GeoGab::calculateAutoThresholds()
{
    LD2420_DPRINT("calculateAutoThresholds(): starting auto-threshold routine");

    // --- Start the auto-threshold routine -----------------------------------
    if(!sflags.AutoThresholdsRun) {
      if (!radar.autoThresholds(settings.autoThresholdsTimeout)) {
          LD2420_DPRINT("calculateAutoThresholds(): rejected (Firmware < 2.44?)" GG_FAIL);
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
            LD2420_DPRINT("calculateAutoThresholds(): completed successfully" GG_OK);
            pflags.AutoThresholds = false;                // Stop the flag processor
            sflags.AutoThresholdsRun = true;              // AutoThresholds stoped
            pflags.LoadCalibration = true;                // Load the new calibration data
            errorClear();
            return;
        }
    }
}

/**
 * @brief Überträgt die komplette Kalibrierungsstruktur in den LD2420.
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
void UmLD2420GeoGab::writeCalibrationToSensor()
{
  LD2420_DPRINT("writeCalibrationToSensor(): begin");

  // 1) In den Konfigurationsmodus wechseln
  if (!radar.configMode(true)) {
    LD2420_DPRINT("writeCalibrationToSensor(): cannot enter config mode" GG_FAIL);
    error(F("Cannot enter config mode"));
    return;
  }

  // 2) Thresholds für alle Gates setzen
  for (uint8_t gate = 0; gate < 9; gate++) {
    uint8_t mv = calibration.movingThresholds[gate];
    uint8_t st = calibration.stationaryThresholds[gate];
    LD2420_DPRINT_ARG(GG_CC(10) "Gate %u: moving=%u stationary=%u", gate, mv, st);
    radar.setGateParameters(gate, mv, st);
  }

  // 3) Max Gates + No-One Window setzen
  LD2420_DPRINT_ARG(GG_CC(10)"MaxMovingGate=%u", calibration.maxMovingGate);
  LD2420_DPRINT_ARG(GG_CC(10)"MaxStationaryGate=%u", calibration.maxStationaryGate);
  LD2420_DPRINT_ARG(GG_CC(10)"NoOneWindow=%u", calibration.noOneWindow);

  radar.setMaxGate(calibration.maxMovingGate, calibration.maxStationaryGate);
  radar.setNoOneWindow(calibration.noOneWindow);

  // 4) Auflösung setzen
  LD2420_DPRINT_ARG(GG_CC(10)"Resolution=%u", calibration.resolution);
  radar.setResolution(calibration.resolution);

  // 5) Konfigurationsmodus verlassen
  radar.configMode(false);

  // 6) Optionaler Neustart (abhängig von Library)
  // radar.restart();   // Nur falls deine Library das verlangt

  LD2420_DPRINT("writeCalibrationToSensor:" GG_OK);
}

/**
 * @brief Load calibration parameters from LD2420 sensor flash.
 *
 * Liest die persistent gespeicherten Kalibrierungswerte aus dem Sensor
 * und übernimmt sie in die lokale Struktur.
 */
void UmLD2420GeoGab::loadCalibrationFromSensor()
{
  LD2420_DPRINT("loadCalibrationFromSensor(): reading from sensor");

  // Alle Parameter vom Sensor anfordern
  radar.requestParameters();
  radar.requestResolution();

  // Moving thresholds
  const auto& mv = radar.getMovingThresholds();
  for (uint8_t gate = 0; gate < 9; gate++) {
    calibration.movingThresholds[gate] = mv.values[gate];
    LD2420_DPRINT_ARG(GG_CC(10)"Gate %u movingThreshold = %u", gate, mv.values[gate]);
  }

  // Stationary thresholds
  const auto& st = radar.getStationaryThresholds();
  for (uint8_t gate = 0; gate < 9; gate++) {
    calibration.stationaryThresholds[gate] = st.values[gate];
    LD2420_DPRINT_ARG(GG_CC(10)"Gate %u stationaryThreshold = %u", gate, st.values[gate]);
  }

  // Max gates
  calibration.maxMovingGate     = radar.getMaxMovingGate();
  calibration.maxStationaryGate = radar.getMaxStationaryGate();
  LD2420_DPRINT_ARG(GG_CC(15)"maxMovingGate     = %u", calibration.maxMovingGate);
  LD2420_DPRINT_ARG(GG_CC(15)"maxStationaryGate = %u", calibration.maxStationaryGate);

  // No-one window
  calibration.noOneWindow = radar.getNoOneWindow();
  LD2420_DPRINT_ARG(GG_CC(15)"noOneWindow = %u", calibration.noOneWindow);

  // Resolution
  calibration.resolution = radar.getResolution();
  LD2420_DPRINT_ARG(GG_CC(15)"resolution = %u", calibration.resolution);

  LD2420_DPRINT("loadCalibrationFromSensor():" GG_OK);
}

/**
 * @brief Automatically detect the correct UART baudrate for the LD2420 sensor.
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
 * This method is deterministic and relies solely on the LD2420 driver's
 * internal protocol validation. No custom packet parsing is required.
 *
 * @return int
 *   - Detected baudrate (9600–115200) on success
 *   - -1 if no working baudrate was found
 */
int UmLD2420GeoGab::autoDetectBaudrate()
{
    // 1. First try the currently configured baudrate
    SENSORSERIAL.begin(settings.BaudRate, SERIAL_8N1, settings.rxpin, settings.txpin);
    delay(50);

    if (radar.begin()) {
        LD2420_DPRINT_ARG("autoDetectBaudrate(): Baudrate: %d" GG_OK, settings.BaudRate);
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
            LD2420_DPRINT_ARG("autoDetectBaudrate(): Auto-detected baudrate: %d" GG_OK, baud);
            return baud;
        }
    }

    LD2420_DPRINT("autoDetectBaudrate(): Auto baudrate detection failed" GG_FAIL);
    return -1;
}


/******************************************************************************************************** */
/***************************************** Endpoints User Webpage *****************************************/
/******************************************************************************************************** */
/**
 * @brief Register LD2420 JSON API endpoints.
 *
 * Registriert:
 *  - GET  /json/LD2420  → Live-Daten
 *  - POST /json/LD2420  → Kommandos
 *  - Usermod Webpage: http://<wled-ip>/um/LD2420
 *
 * Wird typischerweise im setup() des Usermods aufgerufen.
 */
void UmLD2420GeoGab::registerEndpoints()
{
    LD2420_DPRINT("registerEndpoints(): Startet"); 
    // EIN EINZIGER GET-ENDPOINT für HTML + JSON
    server.on("/LD2420", HTTP_GET,
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
    server.on("/LD2420/json", HTTP_POST,
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
    LD2420_DPRINT("registerEndpoints():" GG_OK); 
}


/**
 * @brief Memory-safe JSON GET handler for LD2420 data.
 * 
 * This version replaces AsyncJsonResponse with a direct DynamicJsonDocument 
 * to avoid heap allocation issues.
 * 
 * @param request Pointer to the AsyncWebServerRequest.
 */
void UmLD2420GeoGab::handleJsonGet(AsyncWebServerRequest* request)
{
  LD2420_DPRINT("handleJsonGet(): Startet"); 
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
    LD2420_DPRINT("handleJsonGet(): invalid json mode" GG_FAIL); 
    return;
  }

  // --- JSON RESPONSE ERZEUGEN ---
  // Wir nutzen direkt das Dokument, um Speicherprobleme zu umgehen
  DynamicJsonDocument doc(2048);
  JsonObject root = doc.to<JsonObject>();

  if (root.isNull()) {
    request->send(500, "application/json", "{\"error\":\"JSON buffer allocation failed\"}");
    LD2420_DPRINT("handleJsonGet(): JSON buffer allocation failed" GG_FAIL); 
    return;
  }

  // --------------------------------------------------------------------------
  // SECTION: calibration
  // --------------------------------------------------------------------------
  if (mode == "calibration" || mode == "all") {
    LD2420_DPRINT("handleJsonGet(): Calibration" GG_OK); 
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
    LD2420_DPRINT("handleJsonGet(): all " GG_OK); 
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
    LD2420_DPRINT("handleJsonGet(): pflags " GG_OK); 
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
    LD2420_DPRINT("handleJsonGet(): stati " GG_OK); 
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
    LD2420_DPRINT("handleJsonGet(): live " GG_OK); 
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
  LD2420_DPRINT("handleJsonGet(): done" GG_OK); 
}
 

/**
 * @brief Handle JSON POST requests for the LD2420 calibration interface.
 *
 * This handler supports only a minimal set of POST commands:
 *
 * @param request Pointer to the AsyncWebServerRequest.
 * @param json    Parsed JSON payload.
 */
void UmLD2420GeoGab::handleJsonPost(AsyncWebServerRequest* request, JsonVariant json)
{
  LD2420_DPRINT("handleJsonPost(): started"); 
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
    LD2420_DPRINT("handleJsonPost(): start_calibration" GG_OK); 
    pflags.StartCalibration = true;
  }
  else if (!strcmp(cmd, "stop_calibration")) {
    LD2420_DPRINT("handleJsonPost(): stop_calibration" GG_OK); 
    pflags.StopCalibration = true;
  }
  else if (!strcmp(cmd, "auto_thresholds")) {
    LD2420_DPRINT("handleJsonPost(): auto_thresholds" GG_OK); 
    pflags.AutoThresholds = true;
  }
  else if (!strcmp(cmd, "load_calibration")) {
    LD2420_DPRINT("handleJsonPost(): load_calibration" GG_OK); 
    pflags.LoadCalibration = true;
  }

  // --------------------------------------------------------------------------
  // Apply full calibration set (flat JSON, matches calibration_t)
  // --------------------------------------------------------------------------
  else if (!strcmp(cmd, "apply_calibration")) {
    LD2420_DPRINT("handleJsonPost(): apply_calibration" GG_OK); 

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
    LD2420_DPRINT("handleJsonPost(): detect_baudrate" GG_OK); 
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
    LD2420_DPRINT("handleJsonPost(): unknown command" GG_FAIL); 
    return;
  }

  // --------------------------------------------------------------------------
  // Default OK response
  // --------------------------------------------------------------------------
  request->send(200, "application/json", "{\"ok\":true}");
  LD2420_DPRINT("handleJsonPost():" GG_OK); 
}


/**
 * @brief Handles web requests for the LD2420 UI with native WLED styling.
 * 
 * If the calibration file is missing, it serves a 404 page that 
 * links to the WLED internal stylesheet to maintain a consistent look.
 * 
 * @param request The pointer to the current AsyncWebServerRequest.
 */
void UmLD2420GeoGab::handleWebRequest(AsyncWebServerRequest *request)
{
    LD2420_DPRINT("handleWebRequest(): started"); 

    if (!request->url().equals(F("/LD2420"))) return;

    const char* path = "/calibration.html";

    if (!LittleFS.exists(path)) {
      LD2420_DPRINT("handleWebRequest(): File not found" GG_FAIL); 
      // Build an HTML response using WLED's own stylesheet
      String html = F("<!DOCTYPE html><html><head>");
      html += F("<meta name='viewport' content='width=device-width, initial-scale=1'>");
      html += F("<link rel='stylesheet' href='/style.css'>"); // Link to WLED system CSS
      html += F("<title>LD2420 Setup</title></head>");
      
      // 'back' provides the standard WLED dark-mode background
      html += F("<body class='back'><main class='container' style='text-align:center; padding-top:50px;'>");
      
      html += F("<h2>LD2420 Calibration</h2>");
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
    LD2420_DPRINT("handleWebRequest():" GG_OK); 
}

static UmLD2420GeoGab LD2010_full;
REGISTER_USERMOD(LD2010_full);
