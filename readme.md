# Pi Panel — Run and Debug Cheat Sheet

The printable [Pi Panel Documentation Guide](documentation/pi-panel-guide.pdf) covers setup and operation. The separate [Software Design Description](documentation/pi-panel-sdd.pdf) records the software architecture and design baseline. Both PDFs are refreshed only when explicitly requested. Editable references are available in [`documentation/README.md`](documentation/README.md), with separate guides for the [LED](documentation/led.md) and [thermistor](documentation/thermistor.md).

Bill’s current setup: C++ / Qt Quick application on the Pi, edited and debugged from VS Code on Windows. The default debug configuration displays the application on the Pi desktop and does not require PuTTY or XLaunch. A separate forwarded-X11 configuration remains available when displaying the window on Windows is useful.

## Run on the Pi desktop

1. Log in to the graphical desktop on the Pi as `bill`.
2. Connect VS Code to the Pi and open `~/projects/pi-panel`.
3. Open **Run and Debug** and select **Debug pi_panel on Pi desktop**.
4. Press **F5**. The application opens directly on the Pi desktop.

The local profile uses the Pi desktop’s Xwayland display (`DISPLAY=:0`) and `/home/bill/.Xauthority`. It starts immediately without pausing at the program entry point.

## Display on Windows with X11 forwarding

1. Start XLaunch on Windows. Choose **Multiple windows → Display number 0 → Start no client**. Leave **Disable access control unchecked**.
2. Open the saved PuTTY session with X11 forwarding enabled.
3. Run `echo $DISPLAY` in PuTTY and update the `DISPLAY` value in **Debug pi_panel over forwarded X11** if it differs.
4. Select that debug configuration in VS Code and press **F5**.

Leave XLaunch and PuTTY running while using the forwarded configuration.

## Saved PuTTY settings

Configure these before connecting, then return to **Session** and save your session.

| Location / setting | Value |
|---|---|
| Session → Host Name | Your Pi’s usual SSH address |
| Login user | `bill` |
| Connection → SSH → X11 → Enable X11 forwarding | Checked |
| X display location | `localhost:0` |
| Remote X11 authentication protocol | `MIT-Magic-Cookie-1` |

Changes to forwarding require a new PuTTY connection. Password login works for this setup; an SSH key is optional.

## VS Code launch.json

The checked-in `.vscode/launch.json` contains two profiles:

| Profile | Display | Requirements |
|---|---|---|
| **Debug pi_panel on Pi desktop** | Pi monitor through Xwayland at `:0` | Pi graphical desktop logged in as `bill` |
| **Debug pi_panel over forwarded X11** | Windows through XLaunch | XLaunch running and a connected PuTTY session with X11 forwarding |

The local profile uses `/home/bill/.Xauthority` and starts immediately. The forwarded profile pauses at program entry and uses `DISPLAY=:10.0`; update that value if `echo $DISPLAY` in PuTTY reports a different display number.

Both profiles invoke **CMake: build pi_panel (Debug)** before launching.

## Build and run tests

Run in the **VS Code remote terminal**:

```bash
cd ~/projects/pi-panel
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Reconfigure after CMake changes; for ordinary source edits, the build command is enough. F5 also invokes your configured build task. The `ctest` command runs tests registered with CTest by your Google Test configuration.

## Run without the debugger

Stop any existing application instance first so two processes don’t compete for GPIO.

**From PuTTY** (its terminal already has the forwarded `DISPLAY`):

```bash
QT_QPA_PLATFORM=xcb QT_QUICK_BACKEND=software ~/projects/pi-panel/build/pi_panel
```

**From the VS Code remote terminal** (supply PuTTY’s current display explicitly):

```bash
cd ~/projects/pi-panel
DISPLAY=localhost:10.0 XDG_RUNTIME_DIR=/run/user/1000 QT_QPA_PLATFORM=xcb QT_QUICK_BACKEND=software ./build/pi_panel
```

The environment entries in `launch.json` apply to the debugged process; they do not set variables in VS Code’s terminal.

## Explorer HAT Pro analog input

The window reads Explorer HAT Pro **Analog 1** five times per second and displays the measured voltage. The HAT uses an ADS1015 ADC at I2C address `0x48`; Pimoroni maps the Analog 1 terminal to ADC channel 3.

The voltage is also converted to Fahrenheit using the thermistor beta equation. The defaults assume:

- 3.3 V divider supply
- 10 kΩ fixed resistor from 3.3 V to Analog 1
- 10 kΩ NTC thermistor from Analog 1 to ground
- 10 kΩ nominal resistance at 25°C
- 3950 K beta value

Match these values to the thermistor datasheet and measured supply for accurate readings. They can be overridden when launching:

```bash
PI_PANEL_DIVIDER_VOLTS=3.3 \
PI_PANEL_FIXED_RESISTOR_OHMS=10000 \
PI_PANEL_THERMISTOR_NOMINAL_OHMS=10000 \
PI_PANEL_THERMISTOR_NOMINAL_C=25 \
PI_PANEL_THERMISTOR_BETA=3950 \
./build/pi_panel
```

If the thermistor is connected to the supply and the fixed resistor is connected to ground, also set `PI_PANEL_THERMISTOR_TO_GROUND=0`.

The Raspberry Pi header I2C interface must be enabled. If `/dev/i2c-1` is absent on a new setup, enable the interface and reboot:

```bash
sudo raspi-config nonint do_i2c 0
sudo reboot
```

After reconnecting, verify that the bus and HAT are visible:

```bash
ls -l /dev/i2c-1
i2cdetect -y 1
```

The scan should show `48` for the ADC and normally `28` for the Explorer HAT touch controller. If the header bus has a different number, set `PI_PANEL_I2C_DEVICE` when launching the app, for example:

```bash
PI_PANEL_I2C_DEVICE=/dev/i2c-3 ./build/pi_panel
```

The application remains usable when the ADC cannot be read and displays the I2C error below the voltage area. The logged-in user must belong to the `i2c` group; user `bill` already does on this Pi.

## Stop the application

- **Close the QML window** for a normal exit. Your code exits the event loop and calls `ledController.turnOff()`.
- In a normal terminal run, **Ctrl+C** invokes your signal-handling shutdown path.
- During debugging, Ctrl+C may pause on `SIGINT`; continuing may be needed for the handler to run.
- **Stop Debugging** may terminate the process without cleanup. Don’t rely on it to turn the physical LED off.

## Troubleshooting

| Symptom | What to check |
|---|---|
| `could not connect to display` with no address | `DISPLAY` is probably empty. Check the environment inside the selected debug configuration, or supply it explicitly for a terminal run. |
| Cannot connect to `localhost:10.0` | Confirm XLaunch is running, PuTTY is still connected, and its current `echo $DISPLAY` matches your configuration. Use the same Pi user in PuTTY and VS Code. |
| PuTTY prints an empty `DISPLAY` | Enable X11 forwarding in PuTTY’s saved session and reconnect. Don’t invent a display value. |
| Runs in PuTTY but forwarded F5 fails | Confirm **Debug pi_panel over forwarded X11** is selected and its `DISPLAY` matches `echo $DISPLAY` in PuTTY. |
| Authorization error | First test from PuTTY. Capture the exact error; matching the display number alone doesn’t repair an X11 authentication problem. |
| Window appears in RealVNC instead of Windows | `DISPLAY=:0` targets the Pi’s desktop. Use PuTTY’s forwarded display for XLaunch. |
| Forwarded profile pauses before opening a window | It uses `stopAtEntry: true`; press F5 again to continue. |
| GPIO reports busy / already requested | Close the other application instance before starting another. |
| `libxcb-cursor0` dependency message | We used `sudo apt install libxcb-cursor0`. If the app works in PuTTY, check display settings first; that message can accompany a display connection failure. |

## RealVNC alternative

To display the app on the Pi desktop viewed through RealVNC, use `DISPLAY=:0` instead of the forwarded display. This route does not need PuTTY or XLaunch.

Known-working terminal command:

```bash
cd ~/projects/pi-panel
DISPLAY=:0 XDG_RUNTIME_DIR=/run/user/1000 QT_QPA_PLATFORM=xcb ./build/pi_panel
```

## Current application behavior

- The QML button toggles the real LED and updates the onscreen indicator through `LedViewModel`.
- A button on Explorer HAT Input 1 toggles the LED after a 30 ms debounce interval.
- Explorer HAT Pro Analog 1 is sampled every 200 ms and displayed in volts and degrees Fahrenheit.
- Available CPU, RP1 controller, NVMe, and other hwmon temperatures are sampled every two seconds and shown in a secondary panel.
- ADC connection errors are displayed in the window without stopping LED control.

This cheat sheet records the existing setup from our session. The deprecated Remote X11 extension is not part of it.
