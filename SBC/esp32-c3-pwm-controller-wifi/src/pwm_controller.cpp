#include "pwm_controller.h"
#include "config.h"
#include <Arduino.h>
#include <ArduinoJson.h>

// Map logical channels 0..2 to ledc channels 0..2 (0-based indexing)
static const uint8_t LED_NUM_CHANNELS = 3;
static const uint8_t LED_CHANNEL_PINS[LED_NUM_CHANNELS] = {
  Config::LED_PIN_0, 
  Config::LED_PIN_1, 
  Config::LED_PIN_2};
static const uint8_t WIPER_CHANNEL = LED_NUM_CHANNELS; // Make sure it is allways the next channel after LED channels

// ##################################################################################
// ##                           CONTEROLLER DEFINITION                             ##
// ##################################################################################
void PwmController::begin() {
  // Lights Attach pins to channels (i.e. Logic channels to pins)
  for (uint8_t ch = 0; ch < LED_NUM_CHANNELS; ++ch) {
    ledcSetup(ch, Config::LED_PWM_FREQUENCY, Config::LED_PWM_RESOLUTION);
    ledcAttachPin(LED_CHANNEL_PINS[ch], ch); // Assign the channel to pin in accordance with the LED_CHANNEL_PINS sequence
  }
  // Attach wiper pins to channels
  ledcSetup(WIPER_CHANNEL, Config::WIPER_PWM_FREQUENCY, Config::WIPER_PWM_RESOLUTION);

  // Initialize outputs to 0
  setAll(0);
}

void PwmController::setChannel(uint8_t ch, uint8_t value) {
  if (ch > LED_NUM_CHANNELS) {return;}// We allow ch to be == LED_NUM_CHANNELS as it links to WIPER_CHANNEL
  ledcWrite(channels[ch], value);
  Serial.printf("PwmController: set channel %u = %u\n", ch, value);
}

uint8_t PwmController::getChannel(uint8_t ch) const {
  if (ch > LED_NUM_CHANNELS) {return 0;} // We allow ch to be == LED_NUM_CHANNELS as it links to WIPER_CHANNEL
  return ledcRead(channels[ch]);
}

void PwmController::setAll(uint8_t value) {
  for (uint8_t i = 0; i < LED_NUM_CHANNELS; ++i) {setChannel(i, value);}
  setChannel(WIPER_CHANNEL, value);
}

void PwmController::lightTest() {
  int max_val = (1 << Config::LED_PWM_RESOLUTION) - 1;
  for (int v = 0; v <= max_val; ++v) {
    setAll((uint8_t)v);
    delay(10);
  }
}

void PwmController::lightOff() {
  for (uint8_t ch = 0; ch < LED_NUM_CHANNELS; ++ch) {setChannel(ch, 0);}
}

void PwmController::lightOn() {
  for (uint8_t ch = 0; ch < LED_NUM_CHANNELS; ++ch) {setChannel(ch, 128);}
}

void PwmController::wipe(){
  //Implement wiping feature
  setChannel(WIPER_CHANNEL, 128); // Wipe to left
  delay(1000); // Wait wait
  setChannel(WIPER_CHANNEL, 128); // Wipe to the right
  delay(1000); // Wait wait
  setChannel(WIPER_CHANNEL, 0); // Stop wiping real smooth
}

// ##################################################################################
// ##                               PROVIDER INTERFACE                             ##
// ##################################################################################
/// The provider class links the controller with a standardized message handling interface.
PwmProvider::PwmProvider(PwmController& pwm)
  : _pwm(pwm) {}

bool PwmProvider::matchesKey(const char* key) const {
  bool is_light = (strncmp(key, "light", 5) == 0);
  bool is_wiper = (strcmp(key, "wiper") == 0);
  return is_light || is_wiper;
}

bool PwmProvider::handleSet(const char* key, const JsonVariant& value, JsonDocument& reply) {
  // Support both "light" and "light.N"
  if (strncmp(key, "light.", 6) == 0) {
    int ch = atoi(key + 6);
    if (ch < LED_NUM_CHANNELS && value.is<int>()) {
      int v = value.as<int>();
      if (v < 0) v = 0; //Floor
      if (v > 255) v = 255; //Ceil
      _pwm.setChannel((uint8_t)ch, (uint8_t)v);
      reply["success"] = true;
      reply["result"] = String("light.") + String(ch) + " set";
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
    for (uint8_t ch = 0; ch < LED_NUM_CHANNELS; ++ch) {_pwm.setChannel(ch, v);}
    reply["success"] = true;
    reply["result"] = "all light channels set";
    return true;
  }

  if (strcmp(key, "wiper") == 0 && value.is<int>()) {
    int v = value.as<int>();
    if (v < 0) v = 0;
    if (v > 255) v = 255;
    _pwm.setChannel(WIPER_CHANNEL, v);
    reply["success"] = true;
    reply["result"] = "all light channels set";
    return true;
  }
  return false;
}

bool PwmProvider::handleGet(const char* key, JsonDocument& reply) {
  // Retrieve specific LED value
  if (strncmp(key, "light.", 6) == 0) {
    int ch = atoi(key + 6);
    if (ch >= 1 && ch < (int)LED_NUM_CHANNELS) {
      // convert 1-based key to 0-based index
      reply["success"] = true;
      reply["value"] = _pwm.getChannel((uint8_t)ch);
      return true;
    }
  }
  
  // Retrieve all led values
  if (strcmp(key, "light") == 0) {
    reply["success"] = true;
    reply["key"] = key;
    uint8_t values[LED_NUM_CHANNELS];
    for (uint8_t i = 0; i < LED_NUM_CHANNELS; ++i) {values[i] = _pwm.getChannel(i);}
    reply["value"] = values;
    return true;
  }
  
  // Retrieve wiper stepper value
  if (strcmp(key, "wiper") == 0) {
      reply["success"] = true;
      reply["key"] = key;
      reply["value"] = _pwm.getChannel(WIPER_CHANNEL);
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