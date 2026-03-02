#include "pwm_controller.h"
#include "config.h"
#include <Arduino.h>
#include <ArduinoJson.h>

// Map logical pwm channels to specific GPIO pins

//LED pins and channels
static const uint8_t NUM_LEDS = 3;
static const uint8_t LED_PINS[] = {
  Config::LED0_PIN, 
  Config::LED1_PIN, 
  Config::LED2_PIN,
};
static const uint8_t LED_CHANNELS[] = {
  Config::LED0_CHANNEL, 
  Config::LED1_CHANNEL, 
  Config::LED2_CHANNEL,
};
// Wiper pins and channels
static const uint8_t NUM_WIPERS = 1; 
static const uint8_t WIPER_PINS[] = {
  Config::WIPER0_PIN, 
};
static const uint8_t WIPER_CHANNELS[] = {
  Config::WIPER0_CHANNEL, 
};
// ##################################################################################
// ##                           CONTEROLLER DEFINITION                             ##
// ##################################################################################
void PwmController::begin() {
  // Lights Attach pins to channels (i.e. Logic channels to pins)
  for (uint8_t i=0; i<NUM_LEDS; i++) {
    ledcSetup(LED_CHANNELS[i], Config::LED_PWM_FREQUENCY, Config::LED_PWM_RESOLUTION);
    ledcAttachPin(LED_PINS[i], LED_CHANNELS[i]); // Assign the channel to pin in accordance with the LED_CHANNEL_PINS sequence
  }

  // Attach wiper pins to channels
  for (uint8_t i=0; i<NUM_WIPERS; i++) {
    ledcSetup(WIPER_CHANNELS[i], Config::WIPER_PWM_FREQUENCY, Config::WIPER_PWM_RESOLUTION);
    ledcAttachPin(WIPER_PINS[i], WIPER_CHANNELS[i]); // Assign the channel to pin in accordance with the LED_CHANNEL_PINS sequence
  }
  // Initialize outputs to 0
  setAll(0);
}

void PwmController::setChannel(uint8_t ch, uint8_t value) {
  ledcWrite(ch, value);
  Serial.printf("PwmController: set channel %u = %u\n", ch, value);
}

uint8_t PwmController::getChannel(uint8_t ch) const {
  return ledcRead(ch);
}

void PwmController::setAll(uint8_t value) {
  for (uint8_t i = 0; i < NUM_LEDS; ++i) {
    setChannel(LED_CHANNELS[i], value);
  }
    for (uint8_t i = 0; i < NUM_WIPERS; ++i) {
    setChannel(WIPER_CHANNELS[i], value);
  }
}

void PwmController::lightTest() {
  int max_val = (1 << Config::LED_PWM_RESOLUTION) - 1;
  for (int v = 0; v <= max_val; ++v) {
    setAll((uint8_t)v);
    delay(10);
  }
}

void PwmController::lightOff() {
  for (uint8_t i=0; i<NUM_LEDS; i++) {setChannel(LED_CHANNELS[i], 0);}
}

void PwmController::lightOn() {
  for (uint8_t i=0; i<NUM_LEDS; i++) {setChannel(LED_CHANNELS[i], 128);} // ONly half light
}

void PwmController::wipe(){
  //Implement wiping feature
  for (uint8_t i = 0; i < NUM_WIPERS; ++i) {
  setChannel(WIPER_CHANNELS[i], 128); // Wipe to left
  delay(1000); // Wait wait
  setChannel(WIPER_CHANNELS[i], 128); // Wipe to the right
  delay(1000); // Wait wait
  setChannel(WIPER_CHANNELS[i], 0); // Stop wiping real smooth
  }
}

// ##################################################################################
// ##                               PROVIDER INTERFACE                             ##
// ##################################################################################
/// The provider class links the controller with a standardized message handling interface.
PwmProvider::PwmProvider(PwmController& pwm)
  : _pwm(pwm) {}

bool PwmProvider::matchesKey(const char* key) const {
  bool is_light = (strncmp(key, "light", 5) == 0);
  bool is_wiper = (strncmp(key, "wiper", 5) == 0);
  return is_light || is_wiper;
}

bool PwmProvider::handleSet(const char* key, const JsonVariant& value, JsonDocument& reply) {
  // Support both "light" and "light.N"
  if (strncmp(key, "light.", 6) == 0) {
    int i = atoi(key + 6);
    if (i < NUM_LEDS && value.is<int>()) {
      int v = value.as<int>();
      if (v < 0) v = 0; //Floor
      if (v > 255) v = 255; //Ceil
      _pwm.setChannel(LED_CHANNELS[i], (uint8_t)v);
      reply["success"] = true;
      reply["result"] = String("light.") + String(i) + " set";
      return true;
    }
    reply["success"] = false;
    reply["error"] = "invalid light channel or value";
    reply["value"] = key;
    return true; // handled (but invalid)
  }
  
  if (strcmp(key, "light") == 0 && value.is<int>()) {
    int v = value.as<int>();
    if (v < 0) v = 0; //Floor
    if (v > 255) v = 255; //Ceil
    for (uint8_t i = 0; i < NUM_LEDS; ++i) {_pwm.setChannel(LED_CHANNELS[i], v);}
    reply["success"] = true;
    reply["result"] = "all light channels set";
    return true;
  }

  if (strncmp(key, "wiper.", 6) == 0) {
    int i = atoi(key + 6);
    if (i < NUM_WIPERS && value.is<int>()) {
      int v = value.as<int>();
      if (v < 0) v = 0; //Floor
      if (v > 255) v = 255; //Ceil
      _pwm.setChannel(WIPER_CHANNELS[i], (uint8_t)v);
      reply["success"] = true;
      reply["result"] = String("wiper.") + String(i) + " set";
      return true;
    }
    reply["success"] = false;
    reply["error"] = "invalid light channel or value";
    reply["value"] = key;
    return true; // handled (but invalid)
  }
  
  if (strcmp(key, "wiper") == 0 && value.is<int>()) {
    int v = value.as<int>();
    if (v < 0) v = 0; //Floor
    if (v > 255) v = 255; //Ceil
    for (uint8_t i = 0; i < NUM_WIPERS; ++i) {_pwm.setChannel(WIPER_CHANNELS[i], v);}
    reply["success"] = true;
    reply["result"] = "all wiper channels set";
    return true;
  }
}

bool PwmProvider::handleGet(const char* key, JsonDocument& reply) {
  if (strncmp(key, "light.", 6) == 0) {
    int i = atoi(key + 6);
    if (i >= 1 && i < NUM_LEDS) {
      // convert 1-based key to 0-based index
      reply["success"] = true;
      reply["value"] = _pwm.getChannel(LED_CHANNELS[i]);
      return true;
    }
  }
  
  if (strcmp(key, "light") == 0) {
    reply["success"] = true;
    reply["key"] = key;
    uint8_t values[NUM_LEDS];
    for (uint8_t i = 0; i < NUM_LEDS; ++i) {values[i] = _pwm.getChannel(LED_CHANNELS[i]);}
    reply["value"] = values;
    return true;
  }
  
  if (strcmp(key, "wiper") == 0) {
      reply["success"] = true;
      reply["key"] = key;
      uint8_t values[NUM_WIPERS];
      for (uint8_t i = 0; i < NUM_WIPERS; ++i) {values[i] = _pwm.getChannel(WIPER_CHANNELS[i]);}
      return true;
    }

    reply["success"] = false;
    reply["error"] = "unknown get key";
    reply["value"] = key;
    return false;
}

bool PwmProvider::handleCmd(const char* cmd, const JsonVariant& params, JsonDocument& reply) {
  if (strcmp(cmd, "lightTest") == 0) {
    _pwm.lightTest();
    reply["success"] = true;
    reply["result"] = "light Test started";
    return true;
  }

  if (strcmp(cmd, "lightOff") == 0) {
    _pwm.lightOff();
    reply["success"] = true;
    reply["result"] = "all lights off";
    return true;
  }

  if (strcmp(cmd, "lightOn") == 0) {
    _pwm.lightOn();
    reply["success"] = true;
    reply["result"] = "all lights on (half brightness)";
    return true;
  }

  if (strcmp(cmd, "wipe") == 0) {
    _pwm.wipe();
    reply["success"] = true;
    reply["result"] = "Performing 1x wipe";
    return true;
  }

  return false;
}