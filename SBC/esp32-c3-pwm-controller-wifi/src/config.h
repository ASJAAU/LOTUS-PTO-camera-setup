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
  const int TIMEOUT_MS = 100000; // connection/reconnection timeout for clients
  
  // LED PWM setup
  constexpr uint32_t LED_PWM_FREQUENCY = 5000; // Hz
  constexpr uint8_t LED_PWM_RESOLUTION = 8; // bits (0-255) WE DO NOT SUPPORT ANY OTHER BIT RESOLUTIONS
  //Light0
  constexpr uint8_t LED0_PIN = 20; //(IO01) GPIO pin used for light0 PWM
  constexpr uint8_t LED0_CHANNEL = 1; // PWM Generator channel for light0
  //Light1
  constexpr uint8_t LED1_PIN = 21; //(IO04) GPIO pin used for light1 PWM
  constexpr uint8_t LED1_CHANNEL = 2; // PWM Generator channel for light1
  //Light2
  constexpr uint8_t LED2_PIN = 22; //(IO05) GPIO pin used for light2 PWM
  constexpr uint8_t LED2_CHANNEL = 3; // PWM Generator channel for light2

  // WIPER PWM setup
  constexpr uint32_t WIPER_PWM_FREQUENCY = 5000; // Hz
  constexpr uint8_t WIPER_PWM_RESOLUTION = 8; // bits (0-255) WE DO NOT SUPPORT ANY OTHER BIT RESOLUTIONS
  // WIPER0
  constexpr uint8_t WIPER0_PIN = 23; //(IO02) GPIO pin used for wiper PWM
  constexpr uint8_t WIPER0_CHANNEL= 4; //PWM Generator channel

  // Debugging
  static const int ONBOARD_LED_PIN = 8; //Standard led pin for esp32-c3-super-mini
  // currently the LED will be on when a client is connected and off when nothing is connected
}

#endif // CONFIG_H
