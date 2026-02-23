/**
 * @file usermod_GeoGab.cpp
 * @author Gabriel A. Sieben (GeoGab)
 * @brief WLED Usermod implementing the Hi-Link LD2410 mmWave radar sensor
 * @version 1.0.0
 * @date 15 Jan 2026
 */

/* 
TODOs: 
/*
  - usermod webseite für die einstellung des moduls
      - Daten werden über eigene Json channels übertragen
            - Config
            - Engeneering live daten 
      - enable engineering
      - disable engineering
      - restart
      - factory reset
      - read config
      - wirte config
      - engineering erhöht die loopfrequenz und schaltet wled aus

  - wenn sich die baudrate geändert hat -> neu initialisieren

  - evl. Suchen ob es eine automaitsch routine gibt, wenn das wled eingeschaltet wird. dies verwenden um den engineering modus auszuschalten und das auf der usermod config seite darzustellen
  - evl. suchen nach einer option, wo man lichter mit der bewegung beeinflussen kann oder eine eigene beispielsroutine schreiben die die bewegung per leds darstellt. 
      - Tiefeninformation
      - Geschwindigkeit der Bewegung

  - Wled an wenn Bewegung erkannt auf jeden Fall rein
  - Homeassistant Discovery nutzen.
  - Am Schluss sinnvolle Konstante für Speichersparen anlegen. 

  CHECKS:
  - Checken ob die reinitialisierung nach dem Baudwechsel geht. 
  */

#include "wled.h"
#include "LD2410_Extended.h"
#include <MyLD2410.h>

class UsermodLD2410GeoGab: public Usermod {
	public:
    /* Public: Functions */
    void setup();								                      // Setup of the user module called by wled main
    void loop();								                      // Loop of the user module called by wled main in loop

    void addToConfig(JsonObject& root);               // TODO: Add config entries to WLED config
    void appendConfigData();                          // TODO: AAdd config descriptions
    void readFromConfig(JsonObject& root);            // TODO: ARead config from JSON
    void addToJsonInfo(JsonObject& root);             // TODO: AAdd info to WLED info page
    uint16_t getId();                                 // Get unique usermod ID

  private:
  	/* Private: Functions */
    void initLD2410();                                // Initialize sensor
    void readSensorData();
    void autoThresholds();
    void startEnhancedMode();
    void stopEnhancedMode();
    void readEngineeringValues();

void readConfiguration();					        // TODO: ARead current configuration from sensor
void writeConfiguration();					      // TODO: AWrite configuration to sensor
void readEngineeringValues();             // TODO: ARead engineering mode values

// TODO: Webserver den eine userseite anbietet


    /*** V A R I A B L E s  &  C O N S T A N T s ***/
    /* Private: Settings of Usermod BME68X wicht can be adapted in WLED usermods */
    struct settings_t {
      bool enabled = ENABLED;               // Whether this usermod is enabled
		  uint8_t Interval = INTERVAL; 	        // Interval of reading sensor data in seconds
      uint BaudRate = BAUDRATE;             // Baud Rate of the Device 
      int8_t rxpin = RXPIN;                 // RX PIN > TX of LD2410
      int8_t txpin = TXPIN;                 // TX PIN > RX of LD2410
      bool HADiscovery = HA_DISCOVERY;      // Publish Home Assistant Discovery messages
    } settings;

	  /* Private: Status Flags */
	  struct sflags_t {
		  bool InitSuccessful = false;  				// Initialation was un-/successful
      bool Error = false;                   // Error flag
      bool EngineeringMode = false;         // Engineering mode active   
      bool MqttInitialized = false; 				// MQTT Initialation done flag (first MQTT Connect)
      bool ReadConfigValid = false;         // Config successfully read
      bool AutoThresholdsSuc = false;       // Auto Thresholds was sucessful
    } sflags;

    /* Private: Processing flags that perform tasks in the loop */
    struct pflags_t {
      bool FactoryReset = false;            // Factory reset the device
      bool RestartModule = false;           // Restart the device
      bool AutoThresholds = false;          // Set auto thresholds
      bool ReInitialize = false;            // Re initialize the device on e.g baud rate change
    } pflags;

    /* Private: Internal Values and values of the Sensor */
    struct values_t {
      String firmwver   = "";               // Firmware version string
      String error_info  = "No Error";      // Error information string
    } values;

    /* Private: LD2410 configuration mirror */
    struct config_t {
      uint8_t max_gate = 0;                     // Highest configured gate index
      uint8_t max_moving_gate = 0;              // Max gate for moving targets
      uint8_t max_stationary_gate = 0;          // Max gate for stationary targets
      uint16_t sensor_idle_time = 0;             // Idle time in seconds

      uint8_t motion_sensitivity[9] = {0};      // Per-gate moving sensitivity
      uint8_t stationary_sensitivity[9] = {0};  // Per-gate stationary sensitivity

     } config;

    /* Private: Values des Engineering Modus -> werden über JSON /json/ldh2410-eng-values.json bereitgestellt */
    struct engineeringvalues_t {
      // TODO
    } evalues;

 	 /* Private: Measurement timers */
	 struct timer_t {
		 unsigned long actual;  									// Actual time stamp
		 unsigned long lastRun; 									// Last measurement time stamp
	 } timer;
   
    /* Private: Various variables */



	 /*** V A R I A B L E s  &  C O N S T A N T s ***/
   /* Public: Settings Strings*/
//TODO: am Ende der Version 1.0    static const char _name[];

}; /* class UsermodLD2410GeoGab END*/

/*** Setting C O N S T A N T S ***/
 /* Private: Settings Strings*/
// TODO: am Ende der Version 1.0     const char UsermodLD2410GeoGab::_name[] PROGMEM = "LD2410";

/************************************************************************************************************/
/********************************************* M A I N  C O D E *********************************************/
/************************************************************************************************************/

MyLD2410 radar(Serial1);             // Constructor

/**
 * @brief Called once by WLED during startup
 *
 * Initializes the LD2410 radar sensor if the usermod is enabled.
 * Any failure here prevents further processing in loop().
 */
void UsermodLD2410GeoGab::setup() {
  if (!settings.enabled) {
    DEBUG_PRINTLN(F(UMOD_DEBUG_NAME "LD2410 usermod disabled by configuration."));
    sflags.InitSuccessful = false;
  } else {
    DEBUG_PRINTLN(F(UMOD_DEBUG_NAME "Setup started"));
    initLD2410();
    if (!sflags.InitSuccessful) {
      DEBUG_PRINTLN(F(UMOD_DEBUG_NAME "Setup failed – sensor not operational"));
    } else {
      DEBUG_PRINTLN(F(UMOD_DEBUG_NAME "Setup was sucessful"GOGAB_OK));
    }
  }
}

/**
 * @brief Initialize LD2410 / LD2420 radar sensor
 *
 * - Starts UART interface
 * - Initializes the MyLD2410 driver
 * - Reads firmware information
 * - Requests current sensor configuration
 *
 * @return true if initialization was successful
 */
void UsermodLD2410GeoGab::initLD2410() {
  DEBUG_PRINTLN(F(UMOD_DEBUG_NAME "Initializing LD2410 radar"));

  /* --- UART setup ---------------------------------------------------- */
  DEBUG_PRINT(F(UMOD_DEBUG_NAME "UART RX pin: "));
  DEBUG_PRINTLN(settings.rxpin);
  DEBUG_PRINT(F(UMOD_DEBUG_NAME "UART TX pin: "));
  DEBUG_PRINTLN(settings.txpin);

  sensorSerial.begin(settings.BaudRate, SERIAL_8N1, settings.rxpin, settings.txpin);

  #if LD2410_DEBUG
    // Enable verbose protocol-level debug output from the library
    radar.debug(Serial);
  #endif
    
  /* --- Sensor initialization ----------------------------------------- */
  if (!radar.begin()) {
    DEBUG_PRINTLN(F(UMOD_DEBUG_NAME "LD2410 not detected on UART" GOGAB_FAIL));
    sflags.Error = true;
    sflags.InitSuccessful = false;
    values.error_info = F("LD2410 sensor not connected");
  } else {
    /* --- Firmware information ------------------------------------------ */
    values.firmwver = radar.getFirmware();
    DEBUG_PRINTLN(F(UMOD_DEBUG_NAME "LD2410 connected successfully" GOGAB_OK));
    DEBUG_PRINT(F(UMOD_DEBUG_NAME "Firmware version: "));
    DEBUG_PRINTLN(values.firmwver);
    sflags.InitSuccessful = true;
    sflags.Error = false;

    /* --- Read current sensor configuration ----------------------------- */
    readConfiguration();
  }
}


/**
 * @brief Main loop called repeatedly by WLED
 *
 * Handles:
 *  - Periodic sensor polling
 *  - Processing of incoming LD2410 frames
 *  - Execution of deferred actions (reset / reboot)
 */
void UsermodLD2410GeoGab::loop() {
  // Abort early if usermod is inactive or WLED is updating LEDs
  if (!settings.enabled || strip.isUpdating() || !sflags.InitSuccessful) return;

  timer.actual = millis();

  // Respect configured polling interval
  if (timer.actual - timer.lastRun < settings.Interval * 1000) return;
  timer.lastRun = timer.actual;

  /* ----------------------- Process incoming sensor data ----------------------- */
  radar.check();


  /* ------------------------------ pFlag Handling ------------------------------ */
  if (pflags.RestartModule) {
    pflags.RestartModule = false;
    DEBUG_PRINTLN(F(UMOD_DEBUG_NAME "Sensor reboot requested by user"));
    radar.requestReboot();
  }
  if (pflags.FactoryReset) {
    pflags.FactoryReset = false;
    DEBUG_PRINTLN(F(UMOD_DEBUG_NAME "Sensor factory reset requested by user"));
    radar.requestReset();
  }
  if (pflags.ReInitialize) {
    pflags.ReInitialize = false;
    DEBUG_PRINTLN(F(UMOD_DEBUG_NAME "Reinitialisation of the sensor"));
    initLD2410();
  }
}

/**
 * @brief Enable enhanced (engineering) mode
 *
 * Increases data output from the sensor but does not affect detection logic.
 */
void UsermodLD2410GeoGab::startEnhancedMode() {
  if (sflags.EngineeringMode) return;

  DEBUG_PRINT(F(UMOD_DEBUG_NAME "Starting engineering mode: "));

  if (radar.enhancedMode(true)) {
    sflags.EngineeringMode = true;
    DEBUG_PRINTLN(F("OK" GOGAB_OK));
  } else {
    DEBUG_PRINTLN(F("Failed" GOGAB_FAIL));
  }
}


/**
 * @brief Disable enhanced (engineering) mode
 *
 */
void UsermodLD2410GeoGab::stopEnhancedMode() {
  if (!sflags.InitSuccessful || !sflags.EngineeringMode) return;

  DEBUG_PRINT(F(UMOD_DEBUG_NAME "Disabling engineering mode: "));

  if (radar.enhancedMode(true)) {
    sflags.EngineeringMode = true;
    DEBUG_PRINTLN(F("OK" GOGAB_OK));
  } else {
    DEBUG_PRINTLN(F("Failed" GOGAB_FAIL));
  }
}

/**
 * @brief Read standard sensor data (non-enhanced mode)
 *
 * Uses the processed detection results provided by the sensor firmware:
 *  - Presence detection
 *  - Moving target detection
 *  - Stationary target detection
 *  - Estimated distances
 */
void UsermodLD2410GeoGab::readSensorData() {

  bool presence   = radar.presenceDetected();
  bool moving     = radar.movingTargetDetected();
  bool stationary = radar.stationaryTargetDetected();

  uint16_t movDist = radar.movingTargetDistance();
  uint16_t staDist = radar.stationaryTargetDistance();

  DEBUG_PRINTLN(F(UMOD_DEBUG_NAME "Sensor data update"));
  DEBUG_PRINT(F(" Presence: "));   DEBUG_PRINTLN(presence);
  DEBUG_PRINT(F(" Moving: "));     DEBUG_PRINTLN(moving);
  DEBUG_PRINT(F(" Stationary: ")); DEBUG_PRINTLN(stationary);
  DEBUG_PRINT(F(" Moving dist: "));DEBUG_PRINTLN(movDist);
  DEBUG_PRINT(F(" Static dist: "));DEBUG_PRINTLN(staDist);

  // TODO: store values in values-struct / publish to MQTT / HA
}

/******************************************************************************************************** */
/************************************ Usermod Webpage relatet routines ************************************/
/******************************************************************************************************** */
/**
 * @brief Read current configuration from LD2410 and store it in config struct
 *
 * This function requests the configuration from the sensor and copies
 * all relevant values into the local config mirror.
 */
void UsermodLD2410GeoGab::readConfiguration() {
  DEBUG_PRINT(F(UMOD_DEBUG_NAME "Requesting sensor configuration: "));

  sflags.ReadConfigValid  = false;

  // Request configuration from sensor
  if (!radar.requestCurrentConfiguration()) {
    DEBUG_PRINTLN(F("FAILED" GOGAB_FAIL));
    return;
  }

  DEBUG_PRINTLN(F("OK"));

  // Wait for configuration frame
  unsigned long start = millis();
  while (millis() - start < 500) {
    if (radar.check() == MyLD2410::CONFIG) {
      break;
    }
  }

  /* --- Copy configuration values ----------------------------------- */
  config.max_gate = radar.getMaxGate();
  config.max_moving_gate = radar.getMaxMovingGate();
  config.max_stationary_gate = radar.getMaxStationaryGate();
  config.sensor_idle_time = radar.getIdleTime();

  for (uint8_t gate = 0; gate <= config.max_gate && gate < 9; gate++) {
    config.motion_sensitivity[gate]     = radar.getMotionSensitivity(gate);
    config.stationary_sensitivity[gate] = radar.getStationarySensitivity(gate);
  }

  sflags.ReadConfigValid = true;

  DEBUG_PRINTLN(F(UMOD_DEBUG_NAME "LD2410 configuration loaded"));
  DEBUG_PRINT(F(" Max gate: ")); DEBUG_PRINTLN(config.max_gate);
  DEBUG_PRINT(F(" Max moving gate: ")); DEBUG_PRINTLN(config.max_moving_gate);
  DEBUG_PRINT(F(" Max stationary gate: ")); DEBUG_PRINTLN(config.max_stationary_gate);
  DEBUG_PRINT(F(" Idle time: ")); DEBUG_PRINTLN(config.sensor_idle_time);

  DEBUG_PRINTLN(F(" Gate sensitivities:"));
  for (uint8_t gate = 0; gate <= config.max_gate; gate++) {
    DEBUG_PRINT(F("  Gate "));
    DEBUG_PRINT(gate);
    DEBUG_PRINT(F(" -> moving="));
    DEBUG_PRINT(config.motion_sensitivity[gate]);
    DEBUG_PRINT(F(" stationary="));
    DEBUG_PRINTLN(config.stationary_sensitivity[gate]);
  }

}

/**
 * @brief Write configuration from config struct back to the LD2410
 *
 * All values are written to the sensor RAM.
 * A reboot is required to activate them.
 */
void UsermodLD2410GeoGab::writeConfiguration() {
  if (!config.valid) {
    DEBUG_PRINTLN(F(UMOD_DEBUG_NAME "No valid configuration to write" GOGAB_FAIL));
    return;
  }

  DEBUG_PRINTLN(F(UMOD_DEBUG_NAME "Writing configuration to LD2410"));

  /* --- Gate limits -------------------------------------------------- */
  radar.setMaxMovingGate(config.max_moving_gate);
  radar.setMaxStationaryGate(config.max_stationary_gate);
  radar.setIdleTime(config.sensor_idle_time);

  /* --- Per-gate sensitivities -------------------------------------- */
  for (uint8_t gate = 0; gate <= config.max_gate && gate < 9; gate++) {
    radar.setMotionSensitivity(gate, config.motion_sensitivity[gate]);
    radar.setStationarySensitivity(gate, config.stationary_sensitivity[gate]);
  }

  /* --- Apply configuration ----------------------------------------- */
  DEBUG_PRINT(F(UMOD_DEBUG_NAME "Applying configuration: "));
  if (radar.requestApplyConfiguration()) {
    DEBUG_PRINTLN(F("OK" GOGAB_OK));
    DEBUG_PRINTLN(F(UMOD_DEBUG_NAME "Sensor reboot required to activate settings"));
  } else {
    DEBUG_PRINTLN(F("FAILED" GOGAB_FAIL));
  }
}


/**
* @brief (Usermod Config Webpage): Start engineering mode. This subroutine is used by teh user web page
*/
void UsermodLD2410GeoGab::startEnhancedMode() {
  DEBUG_PRINT(F(UMOD_DEBUG_NAME "Starting engineering mode: "));
  if(radar.enhancedMode(true)) {
    sflags.EngineeringMode = true;
    DEBUG_PRINTLN(F("OK" GOGAB_OK));
  } else {
    DEBUG_PRINTLN(F("Failed" GOGAB_FAIL));
  }
}

/**
* @brief (Usermod Config Webpage) Stop engineering mode
*/
void UsermodLD2410GeoGab::stopEnhancedMode() {
  DEBUG_PRINT(F(UMOD_DEBUG_NAME "Stopping engineering mode: "));
  if(radar.enhancedMode(false)) {
    sflags.EngineeringMode = false;
    DEBUG_PRINTLN(F("OK" GOGAB_OK));
  } else {
    DEBUG_PRINTLN(F("Failed" GOGAB_FAIL));
  }
}

/**
 * @brief Read raw gate-level signal data (enhanced mode)
 *
 * Enhanced mode provides per-gate signal strengths for:
 *  - Moving targets
 *  - Stationary targets
 *
 * This data is intended for tuning, diagnostics and visualization.
 */
void UsermodLD2410GeoGab::readEngineeringValues() {
  if (!sflags.EngineeringMode) {
#if WLED_DEBUG
    DEBUG_PRINTLN(F(UMOD_DEBUG_NAME "Engineering data requested but mode is inactive"));
#endif
    return;
  }

  const uint8_t* mov = radar.getMovingSignals();
  const uint8_t* sta = radar.getStationarySignals();

  DEBUG_PRINTLN(F(UMOD_DEBUG_NAME "Engineering gate signals"));
  for (uint8_t gate = 0; gate < 9; gate++) {
    DEBUG_PRINT(F(" Gate "));
    DEBUG_PRINT(gate);
    DEBUG_PRINT(F(": moving="));
    DEBUG_PRINT(mov[gate]);
    DEBUG_PRINT(F(" stationary="));
    DEBUG_PRINTLN(sta[gate]);
  }

  // TODO: copy values into evalues struct for JSON endpoint
}


void autoThreshholds() {
  Serial.println("Initial sensor parameters\n-------------------------");
  if (radar.autoThresholds()) {
    Serial.println("\n************\nYOU HAVE 10 SECONDS TO LEAVE THE ROOM!!!\n************");
    delay(10000);
    Serial.print("In progress ");
    while (true) {
      switch (sensor.getAutoStatus()) {
        case AutoStatus::IN_PROGRESS:
          Serial.print('.');
          delay(2000);
          break;
        case AutoStatus::COMPLETED:
          Serial.println("\nSUCCESS!!!");
          Serial.println("Final sensor parameters\n-----------------------");
          printParameters();
          Serial.println("Done!");
          return;
        case AutoStatus::NOT_IN_PROGRESS:
          Serial.println("\nStopped. Motion detected?");
          printParameters();
          Serial.print("Performing factory reset... ");
          delay(2000);
          if (sensor.requestReset()) Serial.println("Done!");
          else Serial.println("Fail...");
          printParameters();
          Serial.println("Bye");
          return;
        default:
          break;
      }
    }
  } else {
    Serial.println("Automatic thresholds configuration failed...");
    Serial.println("Is the firmware < 2.44?");
    sensor.requestReboot();
    printParameters();
    Serial.println("Bye");
  }
}



/*********************************************************************************************************/
/****************************************** WLED relatet routines ****************************************/
/*********************************************************************************************************/

/**
* @brief Called by WLED: Returns the unique usermod ID
*/
uint16_t UsermodLD2410GeoGab::getId() {
  return USERMOD_ID_LD2410;
}

/**
* @brief Called by WLED: Add usermod configuration entries to config page
*/
void UsermodLD2410GeoGab::addToConfig(JsonObject& root) {
  DEBUG_PRINT(F(UMOD_DEBUG_NAME "Creating configuration page content."));
  //TODO: Add checkbox for LD2410 reset flag
  
//TODO
  
  DEBUG_PRINTLN(F(GOGAB_OK));
}

/**
* @brief Called by WLED: Add configuration descriptions and dropdowns
*/
void UsermodLD2410GeoGab::appendConfigData() {
  DEBUG_PRINT(F(UMOD_DEBUG_NAME "Appending ifnfomations to the configuartion page."));
  // TODO: Zusatzinfos bereitstellen


}

/**
* @brief Called by WLED: Read configuration from JSON
*/
bool UsermodLD2410GeoGab::readFromConfig(JsonObject& root) {
  DEBUG_PRINT(F(UMOD_DEBUG_NAME "Reading usermod configuration."));
  //TODO: special handling for LD2410 reset flag checkbox

}

/**
* @brief Called by WLED: Add usermod info to info page
*/
void UsermodLD2410GeoGab::addToJsonInfo(JsonObject& root) {
  DEBUG_PRINT(F(UMOD_DEBUG_NAME "Providing info screen content."));
    
  
}


static UsermodLD2410GeoGab LD2010_Extended;
REGISTER_USERMOD(LD2010_Extended);
