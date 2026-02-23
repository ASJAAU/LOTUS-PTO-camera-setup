#include "pwm_controller.h"
#include "config.h"
#include <Arduino.h>
#include <ArduinoJson.h>

// Map channels 1..3 to ledc channels 0..2
static const uint8_t LEDC_CHANNEL_MAP[4] = {0, 0, 1, 2 };


// ##################################################################################
// ##                           CONTEROLLER DEFINITION                             ##
// ##################################################################################
void PwmController::begin() {
  // Configure ledc timers and attach pins
  for (uint8_t ch = 1; ch <= 3; ++ch) {
    uint8_t ledcChannel = LEDC_CHANNEL_MAP[ch];
    // ledcSetup(channel, freq, resolution)
    ledcSetup(ledcChannel, Config::PWM_FREQUENCY, Config::PWM_RESOLUTION);
  }

  // Attach pins to channels
  ledcAttachPin(Config::PWM_PIN_CH1, LEDC_CHANNEL_MAP[1]);
  ledcAttachPin(Config::PWM_PIN_CH2, LEDC_CHANNEL_MAP[2]);
  ledcAttachPin(Config::PWM_PIN_CH3, LEDC_CHANNEL_MAP[3]);

  // Initialize outputs to 0
  setAll(0);
}

void PwmController::setChannel(uint8_t ch, uint8_t value) {
  if (ch < 1 || ch > 3) return;
  channels[ch] = value;
  uint8_t ledcChannel = LEDC_CHANNEL_MAP[ch];
  ledcWrite(ledcChannel, value);
  Serial.printf("PwmController: set channel %u = %u\n", ch, value);
}

uint8_t PwmController::getChannel(uint8_t ch) const {
  if (ch < 1 || ch > 3) return 0;
  return channels[ch];
}

void PwmController::setAll(uint8_t value) {
  for (uint8_t i = 1; i <= 3; ++i) {
    setChannel(i, value);
  }
}

void PwmController::pwmTest() {
  uint8_t max_val = Config::PWM_RESOLUTION^2 - 1;
  for (int v = 0; v <= max_val; ++v) {
    setAll(v);
    delay(10);
  }
}

void PwmController::pwmOff() {
  setAll(0);
}

void PwmController::pwmOn() {
  uint8_t max_val = Config::PWM_RESOLUTION^2 - 1;
  setAll(uint8_t(max_val/2));
}

// ##################################################################################
// ##                               PROVIDER INTERFACE                             ##
// ##################################################################################
/// The provider class links the controller with a standardized message handling interface.
PwmProvider::PwmProvider(PwmController& pwm)
  : _pwm(pwm) {}

bool PwmProvider::matchesKey(const char* key) const {
  return (strncmp(key, "pwm", 3) == 0);
}

bool PwmProvider::handleSet(const char* key, const JsonVariant& value, JsonDocument& reply) {
  // Support both "pwm" and "pwm.N"
  if (strncmp(key, "pwm.", 4) == 0) {
    int ch = atoi(key + 4);
    if (ch >= 1 && ch <= 3 && value.is<int>()) {
      int v = value.as<int>();
      if (v < 0) v = 0;
      if (v > 255) v = 255;
      _pwm.setChannel((uint8_t)ch, (uint8_t)v);
      reply["success"] = true;
      reply["result"] = String("pwm.") + String(ch) + " set";
      return true;
    }
    reply["success"] = false;
    reply["error"] = "invalid pwm channel or value";
    reply["value"] = key;
    return true; // handled (but invalid)
  }

  if (strcmp(key, "pwm") == 0 && value.is<int>()) {
    int v = value.as<int>();
    if (v < 0) v = 0;
    if (v > 255) v = 255;
    _pwm.setAll((uint8_t)v);
    reply["success"] = true;
    reply["result"] = "all pwm channels set";
    return true;
  }

  return false;
}

bool PwmProvider::handleGet(const char* key, JsonDocument& reply) {
  if (strncmp(key, "pwm.", 4) == 0) {
    int ch = atoi(key + 4);
    if (ch >= 1 && ch <= 3) {
      reply["success"] = true;
      reply["value"] = _pwm.getChannel((uint8_t)ch);
      return true;
    }
    reply["success"] = false;
    reply["error"] = "unknown get key";
    reply["value"] = key;
    return true;
  }

  if (strcmp(key, "pwm") == 0) {
    reply["success"] = true;
    reply["value"] = _pwm.getChannel(1);
    return true;
  }

  return false;
}

bool PwmProvider::handleCmd(const char* cmd, const JsonVariant& params, JsonDocument& reply) {
  if (strcmp(cmd, "pwmTest") == 0) {
    _pwm.pwmTest();
    reply["success"] = true;
    reply["result"] = "pwmTest started";
    return true;
  }

  if (strcmp(cmd, "pwmOff") == 0) {
    _pwm.pwmOff();
    reply["success"] = true;
    reply["result"] = "all pwm off";
    return true;
  }

  if (strcmp(cmd, "pwmOn") == 0) {
    _pwm.pwmOn();
    reply["success"] = true;
    reply["result"] = "all pwm on (half brightness)";
    return true;
  }

  return false;
}