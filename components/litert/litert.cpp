#include "litert.h"
#include "esphome/core/log.h"

namespace esphome {
namespace litert {

static const char *const TAG = "litert";

void LiteRTComponent::setup() {

  ESP_LOGV(TAG, "Setup()");  
}

void LiteRTComponent::loop() {

}

float LiteRTComponent::get_setup_priority() const { return setup_priority::DATA; }

void LiteRTComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "LiteRT:");
  if (this->is_failed()) {
    ESP_LOGE(TAG, "LiteRT setup failed!");
  }
}

}  // namespace litert
}  // namespace esphome
