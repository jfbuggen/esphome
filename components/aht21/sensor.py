import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import i2c, sensor
from esphome.const import (
    CONF_GPIO,
    CONF_HUMIDITY,
    CONF_ID,
    CONF_TEMPERATURE,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
    CONF_VARIANT,
)

DEPENDENCIES = ["i2c"]

aht21_ns = cg.esphome_ns.namespace("aht21")
AHT21Component = aht21_ns.class_("AHT21Component", cg.Component, i2c.I2CDevice)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(AHT21Component),
            cv.Required(CONF_GPIO): pins.gpio_output_pin_schema,
            cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_HUMIDITY): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_HUMIDITY,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
        }
    )
    .extend(i2c.i2c_device_schema(0x38))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    pwr_pin = await cg.gpio_pin_expression(config[CONF_GPIO])
    cg.add(var.set_power_pin(pwr_pin))
    
    if temperature := config.get(CONF_TEMPERATURE):
        sens = await sensor.new_sensor(temperature)
        cg.add(var.set_temperature_sensor(sens))

    if humidity := config.get(CONF_HUMIDITY):
        sens = await sensor.new_sensor(humidity)
        cg.add(var.set_humidity_sensor(sens))
