# Pi Panel Architecture

Two print-ready PDF snapshots are available:

- [Pi Panel Documentation Guide](pi-panel-guide.pdf) is the user and hardware setup manual.
- [Pi Panel Software Design Description](pi-panel-sdd.pdf) is the formal software architecture and design baseline.

The editable print sources are `pi-panel-guide.html` and `pi-panel-sdd.html`.

> **PDF refresh policy:** These PDFs are controlled snapshots. Do not regenerate them as part of ordinary code or Markdown changes. Regenerate a PDF only when the project owner explicitly requests a refresh.

Manual generation commands:

```bash
bash documentation/generate-pdf.sh
bash documentation/generate-sdd-pdf.sh
```

Pi Panel is a Qt Quick application that controls Raspberry Pi GPIO and reads an analog thermistor through a Pimoroni Explorer HAT Pro. The executable is built with CMake and uses C++ for hardware access and application state, with QML for the window.

## Component overview

| Component | Responsibility |
|---|---|
| `main.cpp` | Creates the application, hardware services, view model, and polling timers |
| `Controllers/GpiodService` | Reads and writes Raspberry Pi GPIO lines through `libgpiod` |
| `Controllers/LedController` | Owns LED state and translates LED/button operations into GPIO calls |
| `Controllers/Ads1015` | Reads Explorer HAT Pro analog channels over Linux I2C |
| `Controllers/Thermistor` | Converts divider voltage into degrees Fahrenheit |
| `ViewModels/LedViewModel` | Exposes LED, voltage, temperature, and error state to QML |
| `Main.qml` | Displays the LED control, measured voltage, temperature, and errors |

## Runtime data flow

```mermaid
flowchart LR
    QML[Main.qml] -->|toggle| VM[LedViewModel]
    VM --> LED[LedController]
    LED --> GPIO[GpiodService]
    GPIO --> PINS[Raspberry Pi GPIO]

    TIMER[200 ms QTimer] --> VM
    VM --> ADC[Ads1015]
    ADC -->|I2C| HAT[Explorer HAT Pro]
    ADC -->|voltage| VM
    VM --> TH[Thermistor]
    TH -->|degrees F| VM
    VM -->|Qt properties and signals| QML
```

The analog timer calls `LedViewModel::sampleAnalogInput()` every 200 ms. The view model first reads ADC channel 3, which is Explorer HAT Pro Analog 1, and then passes that voltage to the thermistor conversion. Qt property notification signals cause QML labels to refresh.

The 5 ms GPIO timer handles shutdown signals and polls Explorer HAT Input 1. A stable physical-button press and the QML button both call `LedViewModel::toggle()`, so both controls update the real LED and the onscreen state.

## Error handling

Hardware exceptions are caught before they escape into Qt event dispatch:

- LED errors emit `operationFailed`.
- ADC errors set `analogAvailable` to false and expose `analogError`.
- Conversion errors set `temperatureAvailable` to false and expose `temperatureError`.
- GPIO polling errors are caught in the 5 ms timer callback and exit the application with an error.

An ADC or thermistor error is shown in the window and does not stop LED control.

## Build and test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build --output-on-failure
```

## Feature documentation

- [LED control](led.md)
- [Thermistor and temperature](thermistor.md)
- [Windows gRPC telemetry](telemetry.md)

General setup, I2C configuration, and display-launch instructions remain in the project root `readme.md`.
