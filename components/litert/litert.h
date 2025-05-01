#pragma once

#include "esphome/core/component.h"
#include "tensorflow/lite/micro/micro_interpreter.h"

namespace esphome {
namespace litert {

class LiteRTComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void set_model_data (const uint8_t* model_data, const uint32_t len) {
    this->model_data_ = model_data;
    this->model_data_length_ = len;
  }
  void set_op_resolver (tflite::MicroOpResolver& op_resolver) { this->op_resolver_ = op_resolver; }

 protected:
  uint8_t const *model_data_{nullptr};
  uint32_t model_data_length_{0}; 
  const tflite::Model* model_{nullptr};
  tflite::MicroInterpreter* interpreter_{nullptr};
  TfLiteTensor* input_{nullptr};
  TfLiteTensor* output_{nullptr};
  int inference_count_{0};
  tflite::MicroOpResolver& op_resolver_; // op resolver is instanciated elsewhere
};

}  // namespace litert
}  // namespace esphome
