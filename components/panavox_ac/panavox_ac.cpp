#include "panavox_ac.h"
#include "esphome/core/log.h"

namespace esphome {
namespace panavox {

static const char* const TAG = "panavox_ac";

// ---- Component lifecycle ----

void PanavoxACComponent::setup() {
    _ac.onStatusUpdate([this](const DeviceStatus& s) { on_status(s); });
    _ac.onError([this](AcError err) { on_error(err); });
    _ac.begin();
}

void PanavoxACComponent::loop() {
    _ac.loop();
}

void PanavoxACComponent::dump_config() {
    LOG_CLIMATE("", "Panavox AC", this);
    ESP_LOGCONFIG(TAG, "  Protocol: W950 UART 9600 8N1");
}

// ---- climate::Climate interface ----

climate::ClimateTraits PanavoxACComponent::traits() {
    auto traits = climate::ClimateTraits();

    traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
    traits.set_visual_min_temperature(16.0f);
    traits.set_visual_max_temperature(32.0f);
    traits.set_visual_temperature_step(1.0f);

    traits.set_supported_modes({
        climate::CLIMATE_MODE_OFF,
        climate::CLIMATE_MODE_COOL,
        climate::CLIMATE_MODE_HEAT,
        climate::CLIMATE_MODE_FAN_ONLY,
        climate::CLIMATE_MODE_DRY,
    });

    traits.set_supported_fan_modes({
        climate::CLIMATE_FAN_AUTO,
        climate::CLIMATE_FAN_QUIET,
        climate::CLIMATE_FAN_LOW,
        climate::CLIMATE_FAN_MEDIUM,
        climate::CLIMATE_FAN_HIGH,
    });

    traits.set_supported_swing_modes({
        climate::CLIMATE_SWING_OFF,
        climate::CLIMATE_SWING_VERTICAL,
        climate::CLIMATE_SWING_HORIZONTAL,
        climate::CLIMATE_SWING_BOTH,
    });

    traits.set_supported_presets({
        climate::CLIMATE_PRESET_NONE,
        climate::CLIMATE_PRESET_BOOST,
        climate::CLIMATE_PRESET_ECO,
    });

    return traits;
}

void PanavoxACComponent::control(const climate::ClimateCall& call) {
    // Determine if we need to change power state
    bool needs_power_on  = false;
    bool needs_power_off = false;

    if (call.get_mode().has_value()) {
        auto m = *call.get_mode();
        if (m == climate::CLIMATE_MODE_OFF) {
            needs_power_off = true;
        } else {
            // Update desired mode first (before setPower so enqueueFullStateOnPowerOn picks it up)
            _ac.setMode(to_ac_mode(m));
            if (!_ac.getDesiredState().power) {
                needs_power_on = true;
            }
        }
    }

    // Apply all other desired-state changes before triggering power-on restore
    if (call.get_target_temperature().has_value()) {
        _ac.setTargetTemp(static_cast<float>(*call.get_target_temperature()));
    }

    if (call.get_fan_mode().has_value()) {
        _ac.setFanSpeed(to_fan_speed(*call.get_fan_mode()));
    }

    if (call.get_swing_mode().has_value()) {
        _ac.setSwing(to_swing_mode(*call.get_swing_mode()));
    }

    if (call.get_preset().has_value()) {
        _ac.setPreset(to_preset(*call.get_preset()));
    }

    // Now apply power change — desired_state is fully updated at this point
    if (needs_power_off) {
        _ac.setPower(false);
    } else if (needs_power_on) {
        _ac.setPower(true);  // enqueueFullStateOnPowerOn() uses current desired_state
    }
}

// ---- Status callback ----

void PanavoxACComponent::on_status(const DeviceStatus& s) {
    this->mode               = from_ac_mode(s.mode, s.power);
    this->target_temperature = s.target_temp_c;
    this->current_temperature = s.current_temp_c;
    this->fan_mode           = from_fan_speed(s.fan_speed);
    this->swing_mode         = from_swing(s.swing_vertical, s.swing_horizontal);
    this->preset             = from_preset(s.preset);

    this->publish_state();

    if (_compressor_freq_sensor != nullptr) {
        _compressor_freq_sensor->publish_state(static_cast<float>(s.compressor_freq));
    }
    if (_outdoor_temp_sensor != nullptr) {
        _outdoor_temp_sensor->publish_state(s.outdoor_temp_c);
    }
    if (_humidity_sensor != nullptr && s.humidity_status != -128) {
        _humidity_sensor->publish_state(static_cast<float>(s.humidity_status));
    }
}

void PanavoxACComponent::on_error(AcError err) {
    switch (err) {
    case AcError::TIMEOUT:
        ESP_LOGW(TAG, "ACK timeout — no response from AC");
        break;
    case AcError::CHECKSUM_MISMATCH:
        ESP_LOGW(TAG, "Checksum mismatch in received frame");
        break;
    case AcError::INVALID_FRAME:
        ESP_LOGW(TAG, "Invalid frame received (bad size or structure)");
        break;
    }
}

// ---- Map helpers: ESPHome → Core ----

AcMode PanavoxACComponent::to_ac_mode(climate::ClimateMode m) {
    switch (m) {
    case climate::CLIMATE_MODE_HEAT:     return AcMode::HEAT;
    case climate::CLIMATE_MODE_FAN_ONLY: return AcMode::FAN_ONLY;
    case climate::CLIMATE_MODE_DRY:      return AcMode::DRY;
    default:                             return AcMode::COOL;
    }
}

FanSpeed PanavoxACComponent::to_fan_speed(climate::ClimateFanMode fm) {
    switch (fm) {
    case climate::CLIMATE_FAN_QUIET:  return FanSpeed::FAN_QUIET;
    case climate::CLIMATE_FAN_LOW:    return FanSpeed::FAN_LOW;
    case climate::CLIMATE_FAN_MEDIUM: return FanSpeed::FAN_MEDIUM;
    case climate::CLIMATE_FAN_HIGH:   return FanSpeed::FAN_HIGH;
    default:                          return FanSpeed::FAN_AUTO;
    }
}

SwingMode PanavoxACComponent::to_swing_mode(climate::ClimateSwingMode sm) {
    switch (sm) {
    case climate::CLIMATE_SWING_VERTICAL:   return SwingMode::VERTICAL;
    case climate::CLIMATE_SWING_HORIZONTAL: return SwingMode::HORIZONTAL;
    case climate::CLIMATE_SWING_BOTH:       return SwingMode::BOTH;
    default:                                return SwingMode::OFF;
    }
}

Preset PanavoxACComponent::to_preset(climate::ClimatePreset p) {
    switch (p) {
    case climate::CLIMATE_PRESET_BOOST: return Preset::TURBO;
    case climate::CLIMATE_PRESET_ECO:   return Preset::ECO;
    default:                            return Preset::NONE;
    }
}

// ---- Map helpers: Core → ESPHome ----

climate::ClimateMode PanavoxACComponent::from_ac_mode(AcMode m, bool power) {
    if (!power) return climate::CLIMATE_MODE_OFF;
    switch (m) {
    case AcMode::HEAT:     return climate::CLIMATE_MODE_HEAT;
    case AcMode::FAN_ONLY: return climate::CLIMATE_MODE_FAN_ONLY;
    case AcMode::DRY:      return climate::CLIMATE_MODE_DRY;
    default:               return climate::CLIMATE_MODE_COOL;
    }
}

climate::ClimateFanMode PanavoxACComponent::from_fan_speed(FanSpeed fs) {
    switch (fs) {
    case FanSpeed::FAN_QUIET:  return climate::CLIMATE_FAN_QUIET;
    case FanSpeed::FAN_LOW:    return climate::CLIMATE_FAN_LOW;
    case FanSpeed::FAN_MEDIUM: return climate::CLIMATE_FAN_MEDIUM;
    case FanSpeed::FAN_HIGH:   return climate::CLIMATE_FAN_HIGH;
    default:                   return climate::CLIMATE_FAN_AUTO;
    }
}

climate::ClimateSwingMode PanavoxACComponent::from_swing(bool v, bool h) {
    if (v && h) return climate::CLIMATE_SWING_BOTH;
    if (v)      return climate::CLIMATE_SWING_VERTICAL;
    if (h)      return climate::CLIMATE_SWING_HORIZONTAL;
    return climate::CLIMATE_SWING_OFF;
}

climate::ClimatePreset PanavoxACComponent::from_preset(Preset p) {
    switch (p) {
    case Preset::TURBO: return climate::CLIMATE_PRESET_BOOST;
    case Preset::ECO:   return climate::CLIMATE_PRESET_ECO;
    default:            return climate::CLIMATE_PRESET_NONE;
    }
}

} // namespace panavox
} // namespace esphome
