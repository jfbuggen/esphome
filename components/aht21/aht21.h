#pragma once

#include <utility>

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace aht21 {

enum class AHT21state {
    idle,
    start,
    init,
    reset1,
    reset2,
    reset3,
    ready,
    start_measure,
    wait_measure,
    check_measure,
    read_measure,
    finish
};

class AHT21Component : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;

  void set_temperature_sensor(sensor::Sensor *temperature_sensor) { this->temperature_sensor_ = temperature_sensor; }
  void set_humidity_sensor(sensor::Sensor *humidity_sensor) { this->humidity_sensor_ = humidity_sensor; }
  void set_power_pin(GPIOPin *power_pin) { this->power_pin_ = power_pin; }
  
 protected:
  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *humidity_sensor_{nullptr};
  GPIOPin *power_pin_{nullptr};
  
  // State machine variables
  uint32_t lastread_{0};
  uint32_t lasttime_{0};
  uint8_t count_{0};
  AHT21state state_{ AHT21state::idle };
  
  bool check_status_(uint8_t status);
  bool init_();
  bool should_reset_();
  bool reset1_();
  bool reset2_();
  bool reset3_();
  bool reset_reg_(uint8_t reg);
  bool request_();
  bool busy_();
  bool retrieve_();
  uint8_t crc8_(uint8_t *data, uint8_t len);

};

}  // namespace aht21
}  // namespace esphome
