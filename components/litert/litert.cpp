#include "litert.h"
#include "esphome/core/log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

// Globals, used for compatibility with Arduino-style sketches.
namespace {
constexpr int kTensorArenaSize = 2000;
uint8_t tensor_arena[kTensorArenaSize];
}  // namespace

namespace esphome {
namespace litert {

static const char *const TAG = "litert";

void LiteRTComponent::setup() {

  ESP_LOGV(TAG, "Setup()");
  
  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  if (this->model_data_ == nullptr) {
    ESP_LOGE(TAG, "No model provided!");
    this->mark_failed();
    return;
  }
  
  this->model_ = tflite::GetModel(this->model_data_);
  if (this->model_->version() != TFLITE_SCHEMA_VERSION) {
    ESP_LOGE(TAG, "Model provided is schema version %d not equal to supported "
                "version %d.", this->model_->version(), TFLITE_SCHEMA_VERSION);
    this->mark_failed();
    return;
  }

  // Pull in only the operation implementations we need.
  static tflite::MicroMutableOpResolver<1> resolver;
  if (resolver.AddFullyConnected() != kTfLiteOk) {
    ESP_LOGE(TAG, "Could not set-up resolver");
    this->mark_failed();
    return;
  }

  // Build an interpreter to run the model with.
  static tflite::MicroInterpreter static_interpreter(
      this->model_, resolver, tensor_arena, kTensorArenaSize);
  this->interpreter_ = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = this->interpreter_->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    ESP_LOGE(TAG, "AllocateTensors() failed");
    this->mark_failed();
    return;
  }

  // Obtain pointers to the model's input and output tensors.
  this->input_ = interpreter->input(0);
  this->output_ = interpreter->output(0);

  // Keep track of how many inferences we have performed.
  this->inference_count_ = 0;
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
