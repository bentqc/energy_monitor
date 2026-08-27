import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, UNIT_AMPERE

ads1115_rms_ns = cg.esphome_ns.namespace("ads1115_rms")
ADS1115RMS = ads1115_rms_ns.class_("ADS1115RMS", sensor.Sensor, cg.Component)

CONFIG_SCHEMA = sensor.sensor_schema(UNIT_AMPERE).extend({
    cv.GenerateID(): cv.declare_id(ADS1115RMS),
})

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield sensor.register_sensor(var)
    yield cg.register_component(var)
