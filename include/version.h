#pragma once

#ifndef FW_VERSION
#define FW_VERSION "1.12.1"
#endif

#if defined(BOARD_ESP32CAM)
  #define FW_TARGET "esp32cam"
#elif defined(BOARD_ESP32C3)
  #define FW_TARGET "esp32-c3"
#elif defined(BOARD_ESP32DEV)
  #define FW_TARGET "esp32dev"
#else
  #define FW_TARGET "unknown"
#endif
