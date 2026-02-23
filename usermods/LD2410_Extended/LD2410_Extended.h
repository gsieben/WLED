#pragma once
/* general settings */

//#define LD2410_DEBUG            // Special serial output of the LD2410 sensor

#ifdef WLED_DEBUG
    #define LD2410_DEBUG  
#endif

#ifndef ENABLED
    #define ENABLED 1             // Usermod enabled per default
#endif
#ifndef INTERVAL
    #define INTERVAL 1            // Executed every second
#endif
#ifndef RXPIN
    #define RXPIN 12              // Default UART RX pin
#endif
#ifndef TXPIN
    #define TXPIN 14              // Default UART TX pin
#endif
#ifndef HA_DISCOVERY
    #define HA_DISCOVERY 1        // Publish Home Assistant Discovery messages
#endif

#define BAUDRATE 115200           // Some Device are 115200 others are 256000
#define sensorSerial Serial1


// Do not change the following definitions
  #define UMOD_DEVICE "ESP32"                 			// NOTE - Set your hardware here
  #define HARDWARE_VERSION "1.0"              			// NOTE - Set your hardware version here
  #define UMOD_SW_VERSION "1.0.0"     			        // NOTE - Version of the User Mod
  #define UMOD_NAME "LD2410"                 			// NOTE - User module name
  #define UMOD_DEBUG_NAME "UM-LD2410: "      			// NOTE - Debug print module name addon
 
  #define ESC "\033"
  #define ESC_CSI ESC "["
  #define ESC_STYLE_RESET ESC_CSI "0m"
  #define ESC_CURSOR_COLUMN(n) ESC_CSI #n "G"
 
  #define ESC_FGCOLOR_BLACK ESC_CSI "30m"
  #define ESC_FGCOLOR_RED ESC_CSI "31m"
  #define ESC_FGCOLOR_GREEN ESC_CSI "32m"
  #define ESC_FGCOLOR_YELLOW ESC_CSI "33m"
  #define ESC_FGCOLOR_BLUE ESC_CSI "34m"
  #define ESC_FGCOLOR_MAGENTA ESC_CSI "35m"
  #define ESC_FGCOLOR_CYAN ESC_CSI "36m"
  #define ESC_FGCOLOR_WHITE ESC_CSI "37m"
  #define ESC_FGCOLOR_DEFAULT ESC_CSI "39m"
 
 /* Debug Print Special Text */
  #define INFO_COLUMN ESC_CURSOR_COLUMN(60)
  #define GOGAB_OK INFO_COLUMN "[" ESC_FGCOLOR_GREEN "OK" ESC_STYLE_RESET "]"
  #define GOGAB_FAIL INFO_COLUMN "[" ESC_FGCOLOR_RED "FAIL" ESC_STYLE_RESET "]"
  #define GOGAB_WARN INFO_COLUMN "[" ESC_FGCOLOR_YELLOW "WARN" ESC_STYLE_RESET "]"
  #define GOGAB_DONE INFO_COLUMN "[" ESC_FGCOLOR_CYAN "DONE" ESC_STYLE_RESET "]"
 
