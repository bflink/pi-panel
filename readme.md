# Pi Panel — Run and Debug Cheat Sheet

Bill’s current setup: C++ / Qt Quick application on the Pi, edited and debugged from VS Code on Windows. The application runs on the Pi and controls its GPIO; XLaunch displays its window on Windows, with PuTTY supplying the X11 forwarding connection.

## Everyday startup

1. **Start XLaunch on Windows.** Choose **Multiple windows → Display number 0 → Start no client**. Leave **Disable access control unchecked**.
2. **Open your saved PuTTY session** to the Pi as `bill`, with X11 forwarding enabled (settings below).
3. In **PuTTY**, check the forwarded display:

   ```bash
   echo $DISPLAY
   ```

   Expect a value such as `localhost:10.0`. Use the actual value—it can change after reconnecting.

4. **Connect VS Code to the Pi** and open `~/projects/pi-panel`.
5. Make sure the `DISPLAY` entry in `.vscode/launch.json` matches PuTTY’s value.
6. Press **F5 in VS Code**. With `stopAtEntry: true`, press **F5 again** after the initial pause.

**Leave XLaunch and PuTTY running. You can minimize PuTTY; you do not need to launch the application there.** F5 launches and debugs the application.

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

Use this in the Pi project’s `.vscode/launch.json`. Replace `localhost:10.0` with the value reported by your current PuTTY session.

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug pi_panel",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/pi_panel",
            "args": [],
            "stopAtEntry": true,
            "cwd": "${workspaceFolder}",
            "environment": [
                { "name": "DISPLAY", "value": "localhost:10.0" },
                { "name": "XDG_RUNTIME_DIR", "value": "/run/user/1000" },
                { "name": "QT_QPA_PLATFORM", "value": "xcb" },
                { "name": "QT_QUICK_BACKEND", "value": "software" }
            ],
            "externalConsole": false,
            "MIMode": "gdb",
            "miDebuggerPath": "/usr/bin/gdb",
            "preLaunchTask": "CMake: build pi_panel (Debug)"
        }
    ]
}
```

- **`environment` belongs inside the configuration**, replacing its empty array—not at the file’s top level.
- **Ctrl+Shift+D** opens Run and Debug. Select **Debug pi_panel** beside the green play button.
- Set `stopAtEntry` to `false` if you don’t want the initial pause.
- `preLaunchTask` refers to your existing build task; keep its name matched to that task.
- `QT_QUICK_BACKEND=software` avoids depending on forwarded OpenGL for this simple QML window.

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
| Runs in PuTTY but fails with F5 | Check the selected configuration and the placement/value of its `environment` array. |
| Authorization error | First test from PuTTY. Capture the exact error; matching the display number alone doesn’t repair an X11 authentication problem. |
| Window appears in RealVNC instead of Windows | `DISPLAY=:0` targets the Pi’s desktop. Use PuTTY’s forwarded display for XLaunch. |
| Program pauses before opening a window | With `stopAtEntry: true`, press F5 again. |
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

- The physical button passes through `ButtonDebouncer` and toggles the real LED.
- A Qt timer polls the hardware every 5 ms; the debouncer uses elapsed time.
- The QML button currently toggles only its onscreen indicator.
- The next coding step is a `LedViewModel` exposing the real LED state to QML, so the physical and onscreen controls share that state.

This cheat sheet records the existing setup from our session. The deprecated Remote X11 extension is not part of it.
