#pragma once
/**
 * @file LD2420_config.h
 * @verbatim
  ____             ____       _     
 / ___| ___  ___  / ___| __ _| |__  
| |  _ / _ \/ _ \| |  _ / _` | '_ \ 
| |_| |  __/ (_) | |_| | (_| | |_) |
 \____|\___|\___/ \____|\__,_|_.__/ 
 * @endverbatim
 * @author Gabriel A. Sieben (GeoGab)
 * @brief Board and user configuration for the LD2420 WLED usermod.
 * @version 1.0.0
 * @date 2026-03
 *
 * @details
 * All defines can be overridden before including this file or via
 * PlatformIO build_flags in platformio.ini:
 * @code{.ini}
 * build_flags =
 *     -DLD2420_TXPIN=14
 *     -DLD2420_RXPIN=12
 *     -DLD2420_BAUDRATE=115200
 * @endcode
 */

// ─── ESP32-S3 Detection ───────────────────────────────────────────────────────

/**
 * @brief Compile-time flag: 1 if building for ESP32-S3, 0 otherwise.
 */
#if defined(ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_ESP32S3_DEV)
  #define IS_ESP32_S3 1
#else
  #define IS_ESP32_S3 0
#endif


// ─── General ─────────────────────────────────────────────────────────────────

/** @brief Enable or disable the usermod at compile time (default: enabled). */
#ifndef LD2420_ENABLED
  #define LD2420_ENABLED 1
#endif

/** @brief Loop execution interval in 0.1 s units (default: 1 → every 100 ms). */
#ifndef LD2420_INTERVAL
  #define LD2420_INTERVAL 1
#endif

/** @brief Publish Home Assistant MQTT Discovery messages (default: enabled). */
#ifndef LD2420_HA_DISCOVERY
  #define LD2420_HA_DISCOVERY 1
#endif

/** @brief MQTT discovery base topic. */
#ifndef HADISCOVERYBASE
  #define HADISCOVERYBASE "homeassistant/sensor/LD2420"
#endif

/** @brief Usermod display name (used in WLED info page). */
#define LD2420_NAME "LD2420"


// ─── UART / Baud Rate ─────────────────────────────────────────────────────────

/**
 * @brief UART baud rate for communication with the LD2420.
 * @details
 * - Firmware >= v1.5.3 → 115200 (default)
 * - Firmware  < v1.5.3 → 256000
 */
#ifndef LD2420_BAUDRATE
  #define LD2420_BAUDRATE 115200
#endif

/**
 * @brief ESP32 TX pin — connected to the LD2420 RX pin.
 * @details Default depends on the target chip variant.
 *          Override via build_flags = -DLD2420_TXPIN=xx
 */
#ifndef LD2420_TXPIN
  #if IS_ESP32_S3
    #define LD2420_TXPIN 40   ///< ESP32-S3 default TX
  #else
    #define LD2420_TXPIN 14   ///< ESP32 default TX
  #endif
#endif

/**
 * @brief ESP32 RX pin — connected to the LD2420 TX pin.
 * @details Default depends on the target chip variant.
 *          Override via build_flags = -DLD2420_RXPIN=xx
 */
#ifndef LD2420_RXPIN
  #if IS_ESP32_S3
    #define LD2420_RXPIN 42   ///< ESP32-S3 default RX
  #else
    #define LD2420_RXPIN 12   ///< ESP32 default RX
  #endif
#endif


// ─── Auto-Calibration Defaults ────────────────────────────────────────────────

/**
 * @brief Number of Energy frames to average during auto-calibration.
 * @details At ~10 Hz this equals ~10 seconds of measurement.
 */
#ifndef LD2420_CALIB_FRAMES
  #define LD2420_CALIB_FRAMES 100
#endif

/**
 * @brief Delay in milliseconds before auto-calibration starts collecting frames.
 * @details Gives the user time to leave the detection zone.
 */
#ifndef LD2420_CALIB_DELAY_MS
  #define LD2420_CALIB_DELAY_MS 5000
#endif


// ─── Debug Level ──────────────────────────────────────────────────────────────

/**
 * @brief Compile-time debug verbosity level.
 *
 * @details
 * | Value | Effect                                                      |
 * |-------|-------------------------------------------------------------|
 * | 0     | All logging compiled out — zero overhead (default)          |
 * | 1     | Info / warning / error messages via UM_LOGI / LOGW / LOGE   |
 * | 2     | Level 1 + hex dump of every TX and RX command frame         |
 *
 * Activated automatically when WLED global debug is enabled:
 * @code{.ini}
 * build_flags = -DWLED_DEBUG
 * @endcode
 *
 * Or set explicitly:
 * @code{.ini}
 * build_flags = -DGG_DEBUG=1
 * @endcode
 */

// Force debug on when WLED global debug is active
#ifdef WLED_DEBUG
  #ifndef GG_DEBUG
    #define GG_DEBUG 1
  #endif
#endif

#ifndef GG_DEBUG
  #define GG_DEBUG 0
#endif

/** @brief Serial port used for all usermod debug output (default: Serial). */
#ifndef GG_DEBUG_SERIAL
  #define GG_DEBUG_SERIAL Serial
#endif


// ─── Internal Logging Macros ─────────────────────────────────────────────────

/**
 * @defgroup LD2420_UM_Logging Usermod logging macros
 * @brief Printf-style logging helpers, active when GG_DEBUG >= 1.
 *
 * @details
 * Output format:
 * @code
 * [LD2420](  1234ms) setup() started
 * [LD2420](W)(  1235ms) UART settings changed — reinitialising sensor
 * [LD2420](E)(  1236ms) sensor not detected on UART
 * @endcode
 *
 * The ANSI colour defines (GG_CYAN, GG_RED, etc.) are provided by the
 * LD2420GeoGab library header, which is always included before this file.
 *
 * When GG_DEBUG == 0 all macros expand to empty do{}while(0) —
 * no code is generated and no strings are placed in flash.
 * @{
 */
#if GG_DEBUG >= 1

  /** @brief Info — normal lifecycle events (init, connect, mode changes). */
  #define UM_LOGI(fmt, ...) \
    GG_DEBUG_SERIAL.printf("[" GG_CYAN "LD2420" GG_RES \
      "](" GG_GREEN "%6lu" GG_RES "ms) " fmt "\n", millis(), ##__VA_ARGS__)

  /** @brief Warning — non-critical but unexpected states (already active, retry, skip). */
  #define UM_LOGW(fmt, ...) \
    GG_DEBUG_SERIAL.printf("[" GG_CYAN "LD2420" GG_RES \
      "](W)(" GG_YELLOW "%6lu" GG_RES "ms) " GG_YELLOW fmt GG_RES "\n", millis(), ##__VA_ARGS__)

  /** @brief Error — sensor failures, config errors, communication loss. */
  #define UM_LOGE(fmt, ...) \
    GG_DEBUG_SERIAL.printf("[" GG_CYAN "LD2420" GG_RES \
      "](E)(" GG_RED "%6lu" GG_RES "ms) " GG_RED fmt GG_RES "\n", millis(), ##__VA_ARGS__)

#else
  #define UM_LOGI(fmt, ...)  do {} while(0)
  #define UM_LOGW(fmt, ...)  do {} while(0)
  #define UM_LOGE(fmt, ...)  do {} while(0)
#endif
/** @} */
