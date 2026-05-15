# Panavox PSS-12 INV — ESPHome Component for Home Assistant

> **Compatibility:** The Panavox PSS-12 INV uses a serial protocol compatible with the Hisense/Aircon International (F4F5) family of inverter air conditioners. This component may also work with other brands sharing the same W950 WiFi module UART interface.

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

### Pinning to a stable version

The `refresh: 0s` setting above caches whatever commit was fetched on first compile. To pin to a specific release instead, replace it with a `ref:` pointing to a version tag:

```yaml
external_components:
  - source: github://The-sultan/Panavox-PSS12-INV-ESPHome
    ref: v1.0.0
    components: [ panavox_ac ]
```

See the [Releases](https://github.com/The-sultan/Panavox-PSS12-INV-ESPHome/releases) page for available versions.

## UART Configuration

The W950 connector on the AC indoor unit operates at 5V logic. A level shifter is required
to interface it with the 3.3V GPIO of an ESP32. The correct `inverted` flags in the UART YAML
depend on your level shifter topology — there is no single universal combination.

**Reference design: 2N2222 NPN inverting level shifter**

This is one known-working setup. The 2N2222 common-emitter circuit inverts the signal on both
TX and RX paths, so the inversions must be compensated in software:

```yaml
uart:
  id: ac_uart
  baud_rate: 9600
  parity: NONE
  tx_pin:
    number: GPIO16      # adjust to your wiring
    inverted: true      # cancels the hardware inversion on TX
  rx_pin:
    number: GPIO17      # adjust to your wiring
                        # no inverted: true here — hardware already corrects RX
```

A schematic for this reference design is available in the
[Core library repository](https://github.com/The-sultan/Panavox-PSS12-INV-Core/blob/main/docs/level_shifter_2n2222_panavox.svg).

Other level shifter topologies (e.g. non-inverting buffers like the CD4050) require different
flag combinations. Consult the [protocol specification](https://github.com/The-sultan/Panavox-PSS12-INV-Core/blob/main/docs/Panavox_PSS-12_INV_Protocol_Specification.md#uart-polarity-flexibility-empirical-observation)
for a detailed explanation of the polarity flexibility of the AC's UART interface.

**Auto-diagnostic**

If the UART flags are wrong, the component will detect the problem automatically and log a
warning every 30 seconds telling you exactly what to change:

```
[W][panavox_ac]: UART polarity mismatch detected. The AC is responding but its
[W][panavox_ac]: response cannot be decoded with the current UART configuration.
[W][panavox_ac]: Fix: toggle the 'inverted' flag on your rx_pin in the UART YAML.
[W][panavox_ac]:   - If 'inverted: true' is set, remove it or set it to false.
[W][panavox_ac]:   - If 'inverted' is absent or false, add 'inverted: true'.
...
```

If bytes are received but the pattern is not recognized (wrong baud rate, bad wiring, etc.),
a different warning is shown pointing to the physical connection.

## Protocol

This component is backed by the [Panavox PSS-12 INV Core](https://github.com/The-sultan/Panavox-PSS12-INV-Core) driver library, which is fetched automatically from GitHub at compile time. Protocol details and documentation are maintained there.

## License

MIT
