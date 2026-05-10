import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir

AUTO_LOAD = ["climate_ir"]

mhi_ns = cg.esphome_ns.namespace("mhi")
MhiClimate = mhi_ns.class_("MhiClimate", climate_ir.ClimateIR)
MhiProtocol = mhi_ns.enum("MhiProtocol")

CONF_PROTOCOL = "protocol"

PROTOCOLS = {
    "rla502a700k": MhiProtocol.MHI_PROTOCOL_RLA502A700K,
    "mitsubishi_heavy_152": MhiProtocol.MHI_PROTOCOL_RLA502A700K,
    "mhi_152": MhiProtocol.MHI_PROTOCOL_RLA502A700K,
    "mitsubishi_heavy_88": MhiProtocol.MHI_PROTOCOL_MITSUBISHI_HEAVY_88,
    "mhi_88": MhiProtocol.MHI_PROTOCOL_MITSUBISHI_HEAVY_88,
}

CONFIG_SCHEMA = climate_ir.climate_ir_with_receiver_schema(MhiClimate).extend(
    {
        cv.Optional(CONF_PROTOCOL, default="rla502a700k"): cv.enum(
            PROTOCOLS, lower=True, space="_"
        ),
    }
)


async def to_code(config):
    var = await climate_ir.new_climate_ir(config)
    cg.add(var.set_protocol(config[CONF_PROTOCOL]))
    cg.add_library("crankyoldgit/IRremoteESP8266", None)
