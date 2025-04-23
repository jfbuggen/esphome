import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor

from esphome.const import (
    CONF_ID,
    CONF_PATH,
)

litert_ns = cg.esphome_ns.namespace("litert")
LiteRTComponent = litert_ns.class_("LiteRTComponent", cg.Component)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LiteRTComponent),
        }
    )
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
