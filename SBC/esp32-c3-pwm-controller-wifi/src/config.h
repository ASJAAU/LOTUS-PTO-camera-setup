#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Basic configuration values for the project. Replace these with your
// network credentials and any platform-specific constants.
namespace Config {
  static const char* WIFI_SSID = "************";
  static const char* WIFI_PASS = "************";
  constexpr uint16_t SERVER_PORT = 5000;
  constexpr uint8_t PWM_PIN_CH1 = 0; // GPIO pin for PWM channel 1
  constexpr uint8_t PWM_PIN_CH2 = 1; // GPIO pin for PWM channel 2
  constexpr uint8_t PWM_PIN_CH3 = 3; // GPIO pin for PWM channel 3
  constexpr uint32_t PWM_FREQUENCY = 5000; // Hz
  constexpr uint8_t PWM_RESOLUTION = 8; // bits (0-255) WE DO NOT SUPPORT ANY OTHER BIT RESOLUTIONS

  // Debugging
  static const int LED_PIN = 8; //Standard led pin for esp32-c3-super-mini
}

#endif // CONFIG_H
