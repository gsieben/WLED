#pragma once
/**
 * @file usermod_GeoGab.cpp
 * @author Gabriel A. Sieben (GeoGab)
 * @brief Config file: Default Values die in der my_config.h oder in den buildflags geändert werden können. 
 *  e.g.  -D LD2410_ENABLED=false
 * @version 1.0.0
 * @date 25 Feb 2026
 */

/****************************************************************/
/**************** G E N E R A L  S E T T I N G S ****************/
/****************************************************************/
#ifndef LD2410_ENABLED
    #define LD2410_ENABLED 1             // Usermod enabled per default
#endif
#ifndef LD2410_INTERVAL
    #define LD2410_INTERVAL 1            // Executed every second/10
#endif
#ifndef LD2410_RXPIN
    #define LD2410_RXPIN 12              // Default UART RX pin
#endif
#ifndef LD2410_TXPIN
    #define LD2410_TXPIN 14              // Default UART TX pin
#endif
#ifndef LD2410_HA_DISCOVERY
    #define LD2410_HA_DISCOVERY 1        // Publish Home Assistant Discovery messages
#endif
#ifndef LD2410_BAUDRATE
    #define LD2410_BAUDRATE 115200           // Some Device are 115200 others are 256000
#endif
#ifndef LD2410_AUTOTRESHOLDS_TIMEOUT
    #define LD2410_AUTOTRESHOLDS_TIMEOUT 10  // Timeout of Auto Tresholds
#endif 
#ifndef SENSORSERIAL
  #define SENSORSERIAL Serial1              // Der verwendete Serial Kanal
#endif 
#ifndef HADISCOVERYBASE
    #define HADISCOVERYBASE "homeassistant/sensor/ld2410"
#endif

// Debug
#define LD24XX_DEBUG_SERIAL         // Ativate Print to "Serial" 
//#define LD24XX_DEBUG_USB          // OR Print to "USB" BUT NOT BOTH

/*************************************************************** */
/************ Do not change the following definitions ************/
/*************************************************************** */
#define LD24XX_NAME "LD24xx"
#define LD24XX_DNAME GG_CYAN GG_BOLD LD24XX_NAME ": " GG_RES

/* DEBUGGING */
// Force serial debug in cases of global wled debug
#ifdef WLED_DEBUG
    #ifndef LD24XX_DEBUG_SERIAL
        #define LD24XX_DEBUG_SERIAL 
    #endif
    #ifndef LD24XX_DEBUG_USB
        #undef  LD24XX_DEBUG_USB
    #endif
#endif

#if defined(LD24XX_DEBUG_SERIAL) && defined(LD24XX_DEBUG_USB)
  #error "Only one debug mode allowed: SERIAL OR USB"
#endif

#ifdef LD24XX_DEBUG_USB
    #define LD24XX_DBG USBSerial
    #define LD24XX_DEBUG
#elif defined(LD24XX_DEBUG_SERIAL)
    #define LD24XX_DBG Serial
    #define LD24XX_DEBUG
#endif

/* Debug Prints */
#ifdef LD24XX_DEBUG
  #define LD24XX_DPRINT(msg) \
    do { \
      LD24XX_DBG.print("["); \
      LD24XX_DBG.print(getMillisStamp()); \
      LD24XX_DBG.print("] "); \
      LD24XX_DBG.print(LD24XX_DNAME); \
      LD24XX_DBG.println(msg); \
    } while(0)

  #define LD24XX_DPRINT_VAR(msg, var) \
    do { \
      LD24XX_DBG.print("["); \
      LD24XX_DBG.print(getMillisStamp()); \
      LD24XX_DBG.print("] "); \
      LD24XX_DBG.print(LD24XX_DNAME); \
      LD24XX_DBG.print(msg); \
      LD24XX_DBG.print(": "); \
      LD24XX_DBG.println(var); \
    } while(0)

  #define LD24XX_DPRINT_ARG(fmt, ...) \
    do { \
      char _buf[128]; \
      snprintf(_buf, sizeof(_buf), fmt, __VA_ARGS__); \
      LD24XX_DBG.print("["); \
      LD24XX_DBG.print(getMillisStamp()); \
      LD24XX_DBG.print("] "); \
      LD24XX_DBG.print(LD24XX_DNAME); \
      LD24XX_DBG.println(_buf); \
    } while(0)
#else
  #define LD24XX_DPRINT(msg)
  #define LD24XX_DPRINT_VAR(msg, var)
  #define LD24XX_DPRINT_ARG(fmt, ...)

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