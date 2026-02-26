#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Basic configuration values for the project. Replace these with your
// network credentials and any platform-specific constants.
namespace Config {
  // Network configuration
  static const char* WIFI_SSID = "************";
  static const char* WIFI_PASS = "************";
  constexpr uint16_t SERVER_PORT = 5000;
  const uint8_t TIMEOUT_MS = 100000; // connection/reconnection timeout for clients
  
  // LED PWM setup
  constexpr uint32_t LED_PWM_FREQUENCY = 5000; // Hz
  constexpr uint8_t LED_PWM_RESOLUTION = 8; // bits (0-255) WE DO NOT SUPPORT ANY OTHER BIT RESOLUTIONS
  constexpr uint8_t LED_PIN_0 = 0; // GPIO pin used for light0 PWM
  constexpr uint8_t LED_PIN_1 = 1; // GPIO pin used for light1 PWM
  constexpr uint8_t LED_PIN_2 = 3; // GPIO pin used for light2 PWM

  // WIPER PWM setup
  constexpr uint32_t WIPER_PWM_FREQUENCY = 5000; // Hz
  constexpr uint8_t WIPER_PWM_RESOLUTION = 8; // bits (0-255) WE DO NOT SUPPORT ANY OTHER BIT RESOLUTIONS
  constexpr uint8_t WIPER_PIN = 8; // GPIO pin used for wiper PWM

  // Debugging
  static const int ONBOARD_LED_PIN = 8; //Standard led pin for esp32-c3-super-mini
  // currently the LED will be on when a client is connected and off when nothing is connected
}

#endif // CONFIG_H
