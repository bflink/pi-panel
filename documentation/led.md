# LED Control

## Hardware mapping

The default LED is Raspberry Pi BCM GPIO 17. This corresponds to LED 2 on the Explorer HAT Pro. `LedController` configures BCM GPIO 23, Explorer HAT Input 1, as the button input.

`GpiodService` requests both lines from `/dev/gpiochip0`:

- The LED line is an output and starts inactive.
- The button line is an input with no Raspberry Pi bias configured.

The chip path and GPIO numbers are constructor parameters, so tests or future hardware configurations can supply different values.

## Software flow

When the user presses **Toggle LED** in the window:

1. `Main.qml` calls `ledViewModel.toggle()`.
2. `LedViewModel::toggle()` calls `LedController::toggle()`.
3. `LedController` writes `ACTIVE` or `INACTIVE` through `IGpioService`.
4. The controller updates its internal `m_ledOn` state.
5. The view model emits `ledOnChanged`.
6. QML updates the indicator color and the `LED ON` or `LED OFF` label.

The `IGpioService` interface separates LED behavior from Linux GPIO access. Unit tests inject a mock service and verify the requested pin values without requiring Raspberry Pi hardware.

## Shutdown behavior

`LedController::turnOff()` writes an inactive value and clears the stored state. It is called after the Qt event loop exits. The controller destructor also attempts to turn the LED off and suppresses exceptions because destructors must not allow hardware failures to escape.

Stopping the process forcibly can bypass normal cleanup, so the LED should not rely on application shutdown as a safety mechanism.

## Physical button

Explorer HAT digital inputs pass through an onboard buffer, so the Raspberry Pi's internal pull-up or pull-down cannot bias the external terminal. Input 1 must be held low externally while the button is released and driven high while it is pressed.

A typical connection is:

```text
3.3 V or 5 V ---- push button ---- Input 1
																	 |
																 10 kΩ
																	 |
																	GND
```

The Explorer HAT input is 5 V tolerant. Do not leave Input 1 floating, because an unconnected buffered input can produce unpredictable presses.

`LedController::isButtonPressed()` therefore treats an active/high GPIO value as pressed. `main.cpp` polls it every 5 ms and `ButtonDebouncer` requires a stable state for 30 ms. A stable transition to pressed calls `LedViewModel::toggle()`, which keeps the physical LED and QML indicator synchronized. Releasing the button does not toggle the LED again.

## Relevant files

- `Controllers/IGpioService.h`
- `Controllers/GpiodService.h` and `Controllers/GpiodService.cpp`
- `Controllers/LedController.h` and `Controllers/LedController.cpp`
- `ViewModels/LedViewModel.h` and `ViewModels/LedViewModel.cpp`
- `tests/LedControllerTests.cpp`
- `Main.qml`