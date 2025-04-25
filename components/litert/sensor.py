import esphome.codegen as cg
import esphome.config_validation as cv
import hashlib
import io
import logging

from esphome import core, external_files
from esphome.components import sensor
from pathlib import Path

from esphome.const import (
    CONF_ID,
    CONF_FILE,
    CONF_PATH,
    CONF_RAW_DATA_ID,
    CONF_SOURCE,
    CONF_TYPE,
    CONF_URL,
)
from esphome.core import CORE, HexInt

litert_ns = cg.esphome_ns.namespace("litert")
LiteRTComponent = litert_ns.class_("LiteRTComponent", cg.Component)

_LOGGER = logging.getLogger(__name__)
DOMAIN = "litert"

SOURCE_LOCAL = "local"
SOURCE_WEB = "web"
FILE_DOWNLOAD_TIMEOUT = 30 # seconds

# For online resources, compute a local path where it will be stored
def compute_local_path(value) -> Path:
    url = value[CONF_URL] if isinstance(value, dict) else value
    h = hashlib.new("sha256")
    h.update(url.encode())
    key = h.hexdigest()[:8]
    base_dir = external_files.compute_local_file_dir(DOMAIN)
    return base_dir / key

# Download the online resource to the local path
def download_file(url, path):
    external_files.download_content(url, path, FILE_DOWNLOAD_TIMEOUT)
    return str(path)
    
# Download the online resource
def download_model(value):
    value = value[CONF_URL] if isinstance(value, dict) else value
    return download_file(value, compute_local_path(value))

# For local resources, validate the full path
def local_path(value):
    value = value[CONF_PATH] if isinstance(value, dict) else value
    return str(CORE.relative_config_path(value))

# Accept only files with .tflite extension
def validate_model_file(value):
    if not value.lower().endswith(".tflite"):
        raise cv.Invalid(f"Only .tflite files are supported.")
    return CORE.relative_config_path(cv.file_(value))

# When file type is not specified, detect it
def validate_file_shorthand(value):
    value = cv.string_strict(value)
    if value.startswith("http://") or value.startswith("https://"):
        return download_model(value)

    value = cv.file_(value)
    return local_path(value)

LOCAL_SCHEMA = cv.All(
    {
        cv.Required(CONF_PATH): validate_model_file,
    },
    local_path,
)

WEB_SCHEMA = cv.All(
    {
        cv.Required(CONF_URL): cv.string,
    },
    download_model,
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
            cv.Required(CONF_FILE): cv.Any(validate_file_shorthand, TYPED_FILE_SCHEMA),
            cv.GenerateID(CONF_RAW_DATA_ID): cv.declare_id(cg.uint8),
        }
    )
)

async def register_model(config):
    path = Path(config[CONF_FILE])
    if not path.is_file():
        raise core.EsphomeError(f"Could not load model file {path}")

    with open(file=path, mode="rb") as md_file:
        content = md_file.read()
        content_size = len(content)
        bytes_as_int = ", ".join(hex(x) for x in content)
        uint8_t = f"const uint8_t LITERT_MODEL[{content_size}] PROGMEM = {{{bytes_as_int}}}"
        size_t = (
            f"const size_t LITERT_MODEL_SIZE = {content_size}"
        )
        cg.add_global(cg.RawExpression(uint8_t))
        cg.add_global(cg.RawExpression(size_t))      
        
#    data = path.read_bytes()
#    rhs = [HexInt(x) for x in data]
#    prog_arr = cg.progmem_array(config[CONF_RAW_DATA_ID], rhs)
#    cg.add_global(var.add_model(prog_arr, len(rhs)))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    
    # Retrieve model file
    path = Path(config[CONF_FILE])
    if not path.is_file():
        raise core.EsphomeError(f"Could not load model file {path}")
    try:
        with open(path, mode="rb") as md_file:
            content = md_file.read()
            content_size = len(content)
    except Exception as e:
        raise core.EsphomeError(f"Could not open binary model file {path}: {e}")
    # Convert model file to an array of ints
    rhs = [int(x) for x in content]
    # Create an array which will reside in program memory and configure the sensor instance to use it
    model_arr = cg.progmem_array(config[CONF_RAW_DATA_ID], rhs)
    cg.add(var.set_model(model_arr, len(rhs)))

    await cg.register_component(var, config)

    # Add Expressif's Tensorflow Lite for ESP32 library
    cg.add_library(
        name="TensorFlow",
        repository="https://github.com/espressif/esp-tflite-micro.git",
        version=None,
    )
    # Need to point explicitly to the flatbuffer header files
    cg.add_build_flag("-v")
    cg.add_build_flag('-I"${platformio.workspace_dir}/${env}/flatbuffers/include"')
    cg.add_build_flag('-I"${platformio.libdeps_dir}/third_party/flatbuffers/include"')
    cg.add_build_flag("-I.piolibdeps/test-litert/TensorFlow/third_party/gemmlowp")
    # Set deeper ldf mode to ensure the compiler finds the right header files accross component code and library code
    cg.add_platformio_option("lib_ldf_mode", "chain+")
