# Thermistor Temperature Measurement

## Hardware arrangement

The current defaults describe this voltage divider:

```text
3.3 V
  |
  +-- 10 kΩ fixed resistor
  |
  +-- Explorer HAT Pro Analog 1
  |
  +-- 10 kΩ NTC thermistor
  |
 GND
```

The Explorer HAT Pro uses an ADS1015 ADC at I2C address `0x48`. Pimoroni maps the terminal labeled Analog 1 to ADS1015 channel 3. `Ads1015::readVoltage(3)` therefore reads this divider.

At the thermistor's nominal temperature, both divider resistances are approximately 10 kΩ. The measured voltage should consequently be close to half of 3.3 V, or 1.65 V. With the current beta model, a measured 1.70 V converts to approximately 74.6°F.

## Voltage acquisition

`Ads1015` opens `/dev/i2c-1` by default and selects address `0x48`. Each sample:

1. Configures a single-ended, single-shot conversion.
2. Selects the 6.144 V full-scale range.
3. Waits up to 100 ms for conversion completion.
4. Decodes either ADS1015 12-bit or ADS1115 16-bit data.
5. Returns a non-negative voltage.

The application samples every 200 ms. Set `PI_PANEL_I2C_DEVICE` if the header I2C adapter has a different device path.

## Resistance calculation

For the default arrangement, with the fixed resistor connected to the supply and the thermistor connected to ground, thermistor resistance is:

$$
R_T = R_F \frac{V_O}{V_S - V_O}
$$

where:

- $R_T$ is thermistor resistance.
- $R_F$ is fixed resistance.
- $V_O$ is the measured divider voltage.
- $V_S$ is the divider supply voltage.

If the thermistor and fixed resistor are swapped, the implementation uses:

$$
R_T = R_F \frac{V_S - V_O}{V_O}
$$

## Temperature calculation

The project uses the thermistor beta equation:

$$
\frac{1}{T} = \frac{1}{T_0} + \frac{1}{\beta}\ln\left(\frac{R_T}{R_0}\right)
$$

$T$ and $T_0$ are in kelvin. The result is converted first to Celsius and then to Fahrenheit:

$$
T_F = (T_K - 273.15)\frac{9}{5} + 32
$$

The default model is a 10 kΩ NTC thermistor with a nominal temperature of 25°C and a beta value of 3950 K. These are common values for SunFounder kit thermistors, but the thermistor datasheet should take precedence when available.

## Calibration settings

The defaults can be changed through environment variables:

| Variable | Default | Meaning |
|---|---:|---|
| `PI_PANEL_DIVIDER_VOLTS` | `3.3` | Measured divider supply voltage |
| `PI_PANEL_FIXED_RESISTOR_OHMS` | `10000` | Fixed resistor value |
| `PI_PANEL_THERMISTOR_NOMINAL_OHMS` | `10000` | Thermistor resistance at its nominal temperature |
| `PI_PANEL_THERMISTOR_NOMINAL_C` | `25` | Nominal temperature in Celsius |
| `PI_PANEL_THERMISTOR_BETA` | `3950` | Thermistor beta value in kelvin |
| `PI_PANEL_THERMISTOR_TO_GROUND` | `1` | Use `0` when the thermistor is connected to the supply instead |

Example:

```bash
PI_PANEL_DIVIDER_VOLTS=3.28 \
PI_PANEL_FIXED_RESISTOR_OHMS=9980 \
PI_PANEL_THERMISTOR_BETA=3950 \
./build/pi_panel
```

Measuring the actual supply and fixed resistor with a multimeter improves accuracy. For higher accuracy across a wide temperature range, replace the beta model with thermistor-specific Steinhart-Hart coefficients.

## Error handling and tests

Voltages at or outside the range from 0 V to the configured supply are rejected because they would produce an invalid or infinite resistance. The view model keeps displaying the raw ADC voltage when available and reports a separate temperature conversion error.

`tests/ThermistorTests.cpp` verifies:

- 1.65 V maps to 77°F with the defaults.
- 1.70 V maps to approximately 74.6°F.
- Temperature changes in the correct direction for both divider orientations.
- Divider-limit voltages are rejected.

## Relevant files

- `Controllers/Ads1015.h` and `Controllers/Ads1015.cpp`
- `Controllers/Thermistor.h` and `Controllers/Thermistor.cpp`
- `ViewModels/LedViewModel.h` and `ViewModels/LedViewModel.cpp`
- `tests/ThermistorTests.cpp`
- `Main.qml`