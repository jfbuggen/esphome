#pragma once

#include "esphome/core/component.h"

namespace esphome {
namespace litert {

class LiteRTComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void set_model (const uint8_t* model, const uint32_t len) {
    this->model_ = model;
    this->model_length_ = len;
  }

 protected:
  uint8_t const *model_{nullptr};
  uint32_t model_length_{0}; 

};

}  // namespace litert
}  // namespace esphome
