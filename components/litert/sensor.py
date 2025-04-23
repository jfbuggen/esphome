import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor

from esphome.const import (
    CONF_ID,
    CONF_FILE,
    CONF_PATH,
    CONF_SOURCE,
    CONF_URL,
)

litert_ns = cg.esphome_ns.namespace("litert")
LiteRTComponent = litert_ns.class_("LiteRTComponent", cg.Component)

FILE_DOWNLOAD_TIMEOUT = 30  # seconds
SOURCE_LOCAL = "local"
SOURCE_WEB = "web"

def local_path(value):
    value = value[CONF_PATH] if isinstance(value, dict) else value
    return str(CORE.relative_config_path(value))


def download_file(url, path):
    external_files.download_content(url, path, FILE_DOWNLOAD_TIMEOUT)
    return str(path)

LOCAL_SCHEMA = cv.All(
    {
        cv.Required(CONF_PATH): cv.file_,
    },
    local_path,
)

WEB_SCHEMA = cv.All(
    {
        cv.Required(CONF_URL): cv.string,
    },
    download_image,
)

TYPED_FILE_SCHEMA = cv.typed_schema(
    {
        SOURCE_LOCAL: LOCAL_SCHEMA,
        SOURCE_WEB: WEB_SCHEMA,
    },
    key=CONF_SOURCE,
)


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
