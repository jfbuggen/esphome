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

 protected:

};

}  // namespace litert
}  // namespace esphome
