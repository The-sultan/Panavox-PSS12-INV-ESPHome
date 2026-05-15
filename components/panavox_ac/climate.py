import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, sensor, uart
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)
from . import panavox_ns

CODEOWNERS = ["@farid-elias"]
DEPENDENCIES = ["uart", "climate"]
AUTO_LOAD = ["sensor"]

PanavoxACComponent = panavox_ns.class_(
    "PanavoxACComponent",
    climate.Climate,
    cg.Component,
    uart.UARTDevice,
)

CONF_COMPRESSOR_FREQUENCY = "compressor_frequency"
CONF_OUTDOOR_TEMPERATURE  = "outdoor_temperature"
CONF_CURRENT_HUMIDITY     = "current_humidity"

CONFIG_SCHEMA = climate.climate_schema(PanavoxACComponent).extend(
    {
        cv.Optional(CONF_COMPRESSOR_FREQUENCY): sensor.sensor_schema(
            unit_of_measurement="Hz",
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:sine-wave",
        ),
        cv.Optional(CONF_OUTDOOR_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CURRENT_HUMIDITY): sensor.sensor_schema(
            unit_of_measurement="%",
            accuracy_decimals=0,
            device_class="humidity",
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
).extend(uart.UART_DEVICE_SCHEMA).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    cg.add_library(
        "panavox-pss12-inv-core=https://github.com/The-sultan/Panavox-PSS12-INV-Core.git",
        "0.2.3",
    )

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
    await uart.register_uart_device(var, config)

    if CONF_COMPRESSOR_FREQUENCY in config:
        sens = await sensor.new_sensor(config[CONF_COMPRESSOR_FREQUENCY])
        cg.add(var.set_compressor_frequency_sensor(sens))

    if CONF_OUTDOOR_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_OUTDOOR_TEMPERATURE])
        cg.add(var.set_outdoor_temperature_sensor(sens))

    if CONF_CURRENT_HUMIDITY in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT_HUMIDITY])
        cg.add(var.set_current_humidity_sensor(sens))
