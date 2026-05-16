#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

#include "PanavoxAC.h"

namespace esphome {
namespace panavox {

// Thin Stream adapter so PanavoxAC (which takes a Stream&) can use
// ESPHome's UARTDevice read/write methods.
class ESPHomeUARTStream : public Stream {
public:
    explicit ESPHomeUARTStream(uart::UARTDevice* dev) : _dev(dev) {}

    int available() override { return _dev->available(); }

    int read() override {
        uint8_t b;
        return _dev->read_byte(&b) ? static_cast<int>(b) : -1;
    }

    // ESPHome UARTDevice does not support peek.
    int peek() override { return -1; }

    size_t write(uint8_t b) override {
        _dev->write_byte(b);
        return 1;
    }

    size_t write(const uint8_t* buf, size_t len) override {
        _dev->write_array(buf, len);
        return len;
    }

    // flush() is a no-op — ESPHome UART has no explicit flush API.
    void flush() override {}

private:
    uart::UARTDevice* _dev;
};

class PanavoxACComponent : public climate::Climate,
                           public Component,
                           public uart::UARTDevice {
public:
    // Called by ESPHome code-gen (climate.py to_code)
    void set_compressor_frequency_sensor(sensor::Sensor* s) { _compressor_freq_sensor = s; }
    void set_outdoor_temperature_sensor(sensor::Sensor* s)  { _outdoor_temp_sensor = s; }
    void set_current_humidity_sensor(sensor::Sensor* s)     { _humidity_sensor = s; }

    // Component lifecycle
    void setup() override;
    void loop() override;
    void dump_config() override;

    float get_setup_priority() const override {
        return setup_priority::DATA;
    }

    // climate::Climate interface
    climate::ClimateTraits traits() override;
    void control(const climate::ClimateCall& call) override;

private:
    // Stream adapter is initialised in the member initializer list.
    // _stream must be declared before _ac so it is constructed first.
    ESPHomeUARTStream _stream{this};
    PanavoxAC         _ac{_stream};

    // Optional diagnostic sensors (null if not configured in YAML)
    sensor::Sensor* _compressor_freq_sensor = nullptr;
    sensor::Sensor* _outdoor_temp_sensor    = nullptr;
    sensor::Sensor* _humidity_sensor        = nullptr;

    // Optimistic UI: publish_state() is suppressed for 15s after a control()
    // call so the user sees immediate feedback. After the window the AC state
    // takes over, reverting if the command was not accepted.
    uint32_t _optimistic_until = 0;

    // Callbacks from PanavoxAC
    void on_status(const DeviceStatus& s);
    void on_error(AcError err);

    // Map helpers
    static AcMode      to_ac_mode(climate::ClimateMode m);
    static FanSpeed    to_fan_speed(climate::ClimateFanMode fm);
    static SwingMode   to_swing_mode(climate::ClimateSwingMode sm);
    static Preset      to_preset(climate::ClimatePreset p);

    static climate::ClimateMode     from_ac_mode(AcMode m, bool power);
    static climate::ClimateFanMode  from_fan_speed(FanSpeed fs);
    static climate::ClimateSwingMode from_swing(bool v, bool h);
    static climate::ClimatePreset   from_preset(Preset p);

    static climate::ClimateAction   derive_action(climate::ClimateMode mode, uint8_t compressor_freq);
};

} // namespace panavox
} // namespace esphome
