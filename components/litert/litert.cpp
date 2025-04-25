#include "litert.h"
#include "esphome/core/log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

// Globals, used for compatibility with Arduino-style sketches.
namespace {
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;
int inference_count = 0;

constexpr int kTensorArenaSize = 2000;
uint8_t tensor_arena[kTensorArenaSize];
}  // namespace

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
