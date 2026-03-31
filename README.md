# Panavox PSS-12 INV ESPHome Component

An ESPHome external component for controlling the **Panavox PSS-12 INV** split inverter air conditioner, exposing a full `climate` entity in Home Assistant.

## What it does

- Power on/off
- Modes: Cool, Heat, Dry, Fan Only
- Target temperature (16–32°C)
- Fan speeds: Auto, Quiet, Low, Medium, High
- Swing: Off, Vertical, Horizontal, Both
- Presets: Boost (Turbo), Eco
- Optional diagnostic sensors: compressor frequency, outdoor temperature, indoor humidity

## Usage

Add to your ESPHome YAML:

```yaml
external_components:
  - source: github://The-sultan/Panavox-PSS12-INV-ESPHome
    refresh: 0s
    components: [ panavox_ac ]

uart:
  id: ac_uart
  baud_rate: 9600
  parity: NONE
  tx_pin:
    number: 16       # adjust to your wiring
    inverted: true
  rx_pin:
    number: 17       # adjust to your wiring

climate:
  - platform: panavox_ac
    name: "Panavox AC"
    uart_id: ac_uart
    compressor_frequency:
      name: "AC Compressor Frequency"
    outdoor_temperature:
      name: "AC Outdoor Temperature"
    current_humidity:
      name: "AC Indoor Humidity"
```

The `compressor_frequency`, `outdoor_temperature`, and `current_humidity` sensors are optional.

## Protocol

This component is backed by the [Panavox PSS-12 INV Core](https://github.com/The-sultan/Panavox-PSS12-INV-Core) driver library, which is fetched automatically from GitHub at compile time. Protocol details and documentation are maintained there.

## License

MIT
