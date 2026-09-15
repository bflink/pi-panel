# System Temperatures

Pi Panel displays available Raspberry Pi component temperatures in a secondary **System temperatures** panel. These readings describe internal hardware, not room temperature, so they use smaller type and less visual emphasis than the external thermistor reading.

## Available readings

`SystemTemperatureReader` scans Linux hwmon devices under `/sys/class/hwmon`. On the current Pi, it discovers:

| Display name | hwmon device | Sensor label |
|---|---|---|
| CPU | `cpu_thermal` | Unlabelled temperature input |
| RP1 controller | `rp1_adc` | Unlabelled temperature input |
| NVMe | `nvme` | `Composite` |
| NVMe Sensor 2 | `nvme` | `Sensor 2` |

The reader matches device names and sensor labels rather than paths such as `hwmon0`, because Linux can assign different hwmon numbers after rebooting or changing hardware. Devices without temperature inputs are ignored. Malformed or unreadable readings are skipped without stopping the application.

## Data flow

1. A Qt timer calls `LedViewModel::sampleSystemTemperatures()` every two seconds.
2. `SystemTemperatureReader` scans each hwmon device and reads its `temp*_input` files.
3. Linux values in thousandths of a degree Celsius are converted to Fahrenheit.
4. The view model exposes a list of names and Fahrenheit values through `systemTemperatures`.
5. A QML `Repeater` renders every available reading in the secondary panel.

The CPU hwmon input represents the same sensor as `/sys/class/thermal/thermal_zone0`, so the thermal-zone reading is not added separately. This avoids displaying the CPU temperature twice.

## Availability

The list is discovered dynamically. An NVMe entry disappears automatically on a Pi without an NVMe drive, while any additional hwmon temperature inputs are displayed using their kernel device name and label. If no readable sensors exist, the panel shows **No system sensors available**.

## Testing

`tests/SystemTemperatureReaderTests.cpp` creates a temporary fake hwmon tree and verifies:

- Discovery does not depend on `hwmon` numbering.
- CPU, RP1, and NVMe sensors receive readable names.
- Millidegree Celsius values convert correctly to Fahrenheit.
- Non-temperature and malformed files are ignored.

## Relevant files

- `Controllers/SystemTemperatureReader.h`
- `Controllers/SystemTemperatureReader.cpp`
- `ViewModels/LedViewModel.h`
- `ViewModels/LedViewModel.cpp`
- `tests/SystemTemperatureReaderTests.cpp`
- `Main.qml`