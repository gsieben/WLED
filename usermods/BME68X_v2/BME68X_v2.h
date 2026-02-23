#pragma once
// Options for the BME68X_v2 usermod for WLED

 /* general settings */
  #define GG_ENABLED 1							//Usermod enabled per default
  #define GG_I2CADRESS 0X77					//Defalut IC2 adress set to 0x77 (some modules are set to 0x76)
  #define GG_INTERVAL 1							//Executed every second
  #define GG_MAXAGE 0								//Force publication after max age in seconds (0 = disabled)
  #define GG_PUBLISCHCHANGE 0   		//Publish changed values only
  #define GG_TEMPSCALE 0						//Temp sale set to Celsius (1=Fahrenheit)
  #define GG_TEMPOFFSET 0						//Temp offset is set to 0 (Celsius)
  #define GG_PUBLISHSENSORSTATE 1		//Publish the sensor states
  #define GG_PUBACC 1							  //Publish accuracy values 
  #define GG_PUBLISHAFTERCALIB 0		//Publish only after sensor calibration
  #define GG_HOMEASSISTANTDISCOVERY 1   //Activate HomeAssistant Discovery (this Module will be shown as MQTT device in HA)
  #define GG_PAUSEONACTIVEWLED 0    //Pause on active WLED not activated per default
 
 /* Decimal places */         			        /* no of digs / -1 means deactivated */
  #define GG_DECIMALS_TEMPERATURE 1				//One decimal places
  #define GG_DECIMALS_HUMIDITY 1
  #define GG_DECIMALS_PRESSURE 0				  //Zero decimal places
  #define GG_DECIMALS_GASRESISTANCE -1		//deavtivated
  #define GG_DECIMALS_DREWPOINT 1
  #define GG_DECIMALS_ABSHUMIDITY 1
  #define GG_DECIMALS_IAQ 0						//Index for Air Quality Number is active
  #define GG_PUBLISHIAQVERBAL -1				//deactivated - Index for Air Quality (IAQ) verbal classification
  #define GG_DECIMALS_STATICIAQ 0				//activated - Static IAQ is better than IAQ for devices that are not moved
  #define GG_PUBLISHSTATICIAQVERBAL 0			//activated
  #define GG_DECIMALS_CO2 0
  #define GG_DECIMALS_VOC 0
  #define GG_DECIMALS_GASPERC 0


// Do not change the following definitions
  #define GG_UMOD_DEVICE "ESP32"                 			 // NOTE - Set your hardware here
  #define GG_HARDWARE_VERSION "1.0"              			 // NOTE - Set your hardware version here
  #define GG_UMOD_BME680X_SW_VERSION "1.0.3"     			 // NOTE - Version of the User Mod
  #define GG_CALIB_FILE_NAME "/BME680X-Calib.hex"			 // NOTE - Calibration file name
  #define GG_UMOD_NAME "BME680X"                 			 // NOTE - User module name
  #define GG_UMOD_DEBUG_NAME "UM-BME680X: "      			 // NOTE - Debug print module name addon
 
  #define GG_ESC "\033"
  #define GG_ESC_CSI ESC "["
  #define GG_ESC_STYLE_RESET ESC_CSI "0m"
  #define GG_ESC_CURSOR_COLUMN(n) ESC_CSI #n "G"
 
  #define GG_ESC_FGCOLOR_BLACK ESC_CSI "30m"
  #define GG_ESC_FGCOLOR_RED ESC_CSI "31m"
  #define GG_ESC_FGCOLOR_GREEN ESC_CSI "32m"
  #define GG_ESC_FGCOLOR_YELLOW ESC_CSI "33m"
  #define GG_ESC_FGCOLOR_BLUE ESC_CSI "34m"
  #define GG_ESC_FGCOLOR_MAGENTA ESC_CSI "35m"
  #define GG_ESC_FGCOLOR_CYAN ESC_CSI "36m"
  #define GG_ESC_FGCOLOR_WHITE ESC_CSI "37m"
  #define GG_ESC_FGCOLOR_DEFAULT ESC_CSI "39m"
 
 /* Debug Print Special Text */
  #define GG_INFO_COLUMN ESC_CURSOR_COLUMN(60)
  #define GG_GOGAB_OK INFO_COLUMN "[" ESC_FGCOLOR_GREEN "OK" ESC_STYLE_RESET "]"
  #define GG_GOGAB_FAIL INFO_COLUMN "[" ESC_FGCOLOR_RED "FAIL" ESC_STYLE_RESET "]"
  #define GG_GOGAB_WARN INFO_COLUMN "[" ESC_FGCOLOR_YELLOW "WARN" ESC_STYLE_RESET "]"
  #define GG_GOGAB_DONE INFO_COLUMN "[" ESC_FGCOLOR_CYAN "DONE" ESC_STYLE_RESET "]"
 