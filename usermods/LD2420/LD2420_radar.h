#pragma once
/**
 * @file usermod_GeoGab.cpp
 * @author Gabriel A. Sieben (GeoGab)
 * @brief Config file: Default Values die in der my_config.h oder in den buildflags geändert werden können. 
 *  e.g.  -D LD2420_ENABLED=false
 * @version 1.0.0
 * @date 03 Mar 2026
 */

/* ESP32-S3 Special Settings (do not change)*/
#if defined(ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_ESP32S3_DEV)
  #define IS_ESP32_S3 1
#else
  #define IS_ESP32_S3 0
#endif

/****************************************************************/
/**************** G E N E R A L  S E T T I N G S ****************/
/****************************************************************/
#ifndef LD2420_ENABLED
    #define LD2420_ENABLED 1             // Usermod enabled per default
#endif
#ifndef LD2420_INTERVAL
    #define LD2420_INTERVAL 1            // Executed every second/10
#endif
#if IS_ESP32_S3 
      // S3 detected
      #ifndef LD2420_TXPIN
        #define LD2420_TXPIN 40              // Default UART TX pin for the esp32-s3
      #endif
      #ifndef LD2420_RXPIN
        #define LD2420_RXPIN 42              // Default UART TX pin for the esp32-s3
      #endif
#else 
      #ifndef LD2420_TXPIN
         #define LD2420_TXPIN 14              // Default UART TX pin for the esp32
      #endif
      #ifndef LD2420_RXPIN
        #define LD2420_RXPIN 12              // Default UART TX pin for the esp32
      #endif
#endif
#ifndef LD2420_HA_DISCOVERY
    #define LD2420_HA_DISCOVERY 1        // Publish Home Assistant Discovery messages
#endif
#ifndef LD2420_BAUDRATE
    #define LD2420_BAUDRATE 115200       // Some Device are 115200 others are 256000
#endif
#ifndef LD2420_AUTOTRESHOLDS_TIMEOUT
    #define LD2420_AUTOTRESHOLDS_TIMEOUT 10  // Timeout of Auto Tresholds
#endif 
#ifndef SENSORSERIAL
  #define SENSORSERIAL Serial1            // Der verwendete Serial Kanal
#endif 
#ifndef HADISCOVERYBASE
    #define HADISCOVERYBASE "homeassistant/sensor/LD2420"
#endif

// Debug
#define LD2420_DEBUG_SERIAL         // Ativate Print to "Serial" 
//#define LD2420_DEBUG_USB          // OR Print to "Native USB" BUT NOT BOTH

/*************************************************************** */
/************ Do not change the following definitions ************/
/*************************************************************** */
#define LD2420_NAME "LD2420"
#define LD2420_DNAME GG_CYAN GG_BOLD LD2420_NAME ": " GG_RES

/* DEBUGGING */
// Force serial debug in cases of global wled debug
#ifdef WLED_DEBUG
    #ifndef LD2420_DEBUG_SERIAL
        #define LD2420_DEBUG_SERIAL 
    #endif
    #ifndef LD2420_DEBUG_USB
        #undef  LD2420_DEBUG_USB
    #endif
#endif

#if defined(LD2420_DEBUG_SERIAL) && defined(LD2420_DEBUG_USB)
  #error "Only one debug mode allowed: SERIAL OR USB"
#endif

#ifdef LD2420_DEBUG_USB
    #define LD2420_DBG USBSerial
    #define LD2420_DEBUG
#elif defined(LD2420_DEBUG_SERIAL)
    #define LD2420_DBG Serial
    #define LD2420_DEBUG
#endif

/* Debug Prints */
#ifdef LD2420_DEBUG
  #define LD2420_DPRINT(msg) \
    do { \
      LD2420_DBG.print("["); \
      LD2420_DBG.print(getMillisStamp()); \
      LD2420_DBG.print("] "); \
      LD2420_DBG.print(LD2420_DNAME); \
      LD2420_DBG.println(msg); \
    } while(0)

  #define LD2420_DPRINT_VAR(msg, var) \
    do { \
      LD2420_DBG.print("["); \
      LD2420_DBG.print(getMillisStamp()); \
      LD2420_DBG.print("] "); \
      LD2420_DBG.print(LD2420_DNAME); \
      LD2420_DBG.print(msg); \
      LD2420_DBG.println(var); \
    } while(0)

  #define LD2420_DPRINT_ARG(fmt, ...) \
    do { \
      char _buf[128]; \
      snprintf(_buf, sizeof(_buf), fmt, __VA_ARGS__); \
      LD2420_DBG.print("["); \
      LD2420_DBG.print(getMillisStamp()); \
      LD2420_DBG.print("] "); \
      LD2420_DBG.print(LD2420_DNAME); \
      LD2420_DBG.println(_buf); \
    } while(0)
#else
  #define LD2420_DPRINT(msg)
  #define LD2420_DPRINT_VAR(msg, var)
  #define LD2420_DPRINT_ARG(fmt, ...)

#endif

/* Stamps */
#define GG_OK GG_CC(80) "[" GG_GREEN "OK" GG_RES "]"
#define GG_FAIL GG_CC(80) "[" GG_RED "FAIL" GG_RES "]"

/* Color Settings */
#define GG "\033"
#define GG_CSI GG "["
#define GG_RES GG_CSI "0m"
#define GG_CC(n) GG_CSI #n "G"

#define GG_BLACK GG_CSI "30m"
#define GG_RED GG_CSI "31m"
#define GG_GREEN GG_CSI "32m"
#define GG_YELLOW GG_CSI "33m"
#define GG_BLUE GG_CSI "34m"
#define GG_MAGENTA GG_CSI "35m"
#define GG_CYAN GG_CSI "36m"
#define GG_WHITE GG_CSI "37m"
#define GG_DEFAULT GG_CSI "39m"

#define GG_BOLD    GG_CSI "1m"
#define GG_ITALIC  GG_CSI "3m"

// Macro Helper
// #define STR_HELPER(x) #x
// #define STR(x) STR_HELPER(x)

// #pragma message ">>> LD2420_RXPIN = " STR(LD2420_RXPIN)
// #pragma message ">>> LD2420_TXPIN = " STR(LD2420_TXPIN)