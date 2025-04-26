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

// Constants for example
const float kXrange = 2.f * 3.14159265359f;
const int kInferencesPerCycle = 20;

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
  this->input_ = this->interpreter_->input(0);
  this->output_ = this->interpreter_->output(0);

  // Keep track of how many inferences we have performed.
  this->inference_count_ = 0;
}

void LiteRTComponent::loop() {
  // Calculate an x value to feed into the model. We compare the current
  // inference_count to the number of inferences per cycle to determine
  // our position within the range of possible x values the model was
  // trained on, and use this to calculate a value.
  float position = static_cast<float>(this->inference_count_) /
                   static_cast<float>(kInferencesPerCycle);
  float x = position * kXrange;

  // Quantize the input from floating-point to integer
  int8_t x_quantized = x / this->input_->params.scale + this->input_->params.zero_point;
  // Place the quantized input in the model's input tensor
  this->input_->data.int8[0] = x_quantized;

  // Run inference, and report any error
  TfLiteStatus invoke_status = this->interpreter_->Invoke();
  if (invoke_status != kTfLiteOk) {
    ESP_LOGE(TAG, "Invoke failed on x: %f\n",
                         static_cast<double>(x));
    return;
  }

  // Obtain the quantized output from model's output tensor
  int8_t y_quantized = this->output_->data.int8[0];
  // Dequantize the output from integer to floating-point
  float y = (y_quantized - this->output_->params.zero_point) * this->output_->params.scale;

  // Output the results. A custom HandleOutput function can be implemented
  // for each supported hardware target.
  //HandleOutput(x, y);
  ESP_LOGD(TAG, "Inference result: input %.6f, Output %.6f, x, y);
  
  // Increment the inference_counter, and reset it if we have reached
  // the total number per cycle
  this->inference_count_ += 1;
  if (this->inference_count_ >= kInferencesPerCycle) this->inference_count_ = 0;
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
