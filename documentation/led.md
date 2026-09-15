# LED Control

## Hardware mapping

The default LED is Raspberry Pi BCM GPIO 17. This corresponds to LED 2 on the Explorer HAT Pro. `LedController` also configures BCM GPIO 23 as a button input by default.

`GpiodService` requests both lines from `/dev/gpiochip0`:

- The LED line is an output and starts inactive.
- The button line is an input configured with a pull-up bias.

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

## Physical button status

`LedController::isButtonPressed()` treats an inactive GPIO value as pressed because the input uses pull-up logic. `ButtonDebouncer` and a 5 ms polling timer are created in `main.cpp`, but the block that reads the button and toggles the LED is currently commented out. As a result, only the QML button controls the LED in the current build.

## Relevant files

- `Controllers/IGpioService.h`
- `Controllers/GpiodService.h` and `Controllers/GpiodService.cpp`
- `Controllers/LedController.h` and `Controllers/LedController.cpp`
- `ViewModels/LedViewModel.h` and `ViewModels/LedViewModel.cpp`
- `tests/LedControllerTests.cpp`
- `Main.qml`