# Windows gRPC telemetry client

The **Windows telemetry** tab subscribes to the C++ simulator in
[bflink/GrpcServerApp](https://github.com/bflink/GrpcServerApp). The Explorer HAT
tab retains the real LED, analog voltage and thermistor controls.

## Pi setup (Debian 13 / arm64)

```bash
sudo apt update
sudo apt install build-essential pkg-config libgrpc++-dev libprotobuf-dev \
  protobuf-compiler protobuf-compiler-grpc
```

Bill's Pi reports gRPC **1.51.1**, protobuf/protoc **3.21.12**, and CMake
**3.31.6**. The build generates C++ bindings using the Pi's own tools. It can
find gRPC through either its CMake package or its pkg-config metadata.
The existing Qt, GPIO and I2C dependencies are still needed for the GUI.

Stop the running Pi application before rebuilding. From its checkout:

```bash
git status --short
git fetch origin
git switch --track origin/feature/grpc-telemetry-client
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j 2
ctest --test-dir build --output-on-failure
```

If the local branch already exists, use `git switch feature/grpc-telemetry-client`.
If Git reports local changes that would be overwritten, keep them and resolve
that situation before switching; do not reset the checkout. `-j 2` limits build
memory demand on the Pi.

## Start the Windows server

In the Windows server checkout, after building as described in its README:

```powershell
.\build\vs2026\Release\pi-data-server.exe --listen 0.0.0.0:50051
```

Use the build folder you actually built (`vs2022`, `vs2026`, or `verify`).
The initial validated executable on Bill's PC is in `build/verify/Release`.
Find the Windows PC's active LAN IPv4 address with `ipconfig`. On the Pi use
that address and port, for example `192.168.1.50:50051`. The example IP must be
replaced. `localhost` on the Pi refers to the Pi itself.

Allow the server through Windows Firewall on the trusted private network if
needed, limiting access to the Pi. This development connection is unencrypted
and unauthenticated; keep it on a trusted LAN. No firewall settings are changed
by the client or its tests.

## Check the connection before opening Qt

```bash
./build/telemetry_probe 192.168.1.50:50051
```

Replace the example IP. The probe needs no GPIO, I2C, display server or running
Qt app. It waits up to 10 seconds and exits successfully after receiving all
three streams, printing temperature (C), pressure (kPa), and waveform (V).
An error returns exit code 1; incorrect command syntax returns 2.

For a minimal networking-only build:

```bash
cmake -S . -B build/network -DPI_PANEL_BUILD_GUI=OFF
cmake --build build/network -j 2
./build/network/telemetry_probe 192.168.1.50:50051
```

## Open the application

Use the existing **Debug pi_panel on Pi desktop** VS Code profile, then select
**Windows telemetry**, enter the PC address and port, and click **Connect**.
Click **Disconnect** to stop all subscriptions or change the address.

To connect automatically when starting from the Pi desktop terminal:

```bash
PI_PANEL_SERVER=192.168.1.50:50051 ./build/pi_panel
```

For F5 auto-connect, add this entry to that profile's `environment` array in
`.vscode/launch.json`, using the actual PC address:

```json
{ "name": "PI_PANEL_SERVER", "value": "192.168.1.50:50051" }
```

The address is optional: without it the hardware tab operates normally and
networking starts only when you click Connect. Display/X11 configuration is
unchanged; see the root readme for the existing Pi desktop and forwarded-X11
launch commands.

## Behavior and class structure

| Component | Responsibility |
| --- | --- |
| `proto/telemetry.proto` | Exact shared server contract, package `pi.telemetry.v1` |
| `Networking/TelemetryClient` | Three independent blocking-read workers, retry, watchdog, bounded buffer |
| `ViewModels/TelemetryViewModel` | GUI-thread properties and 50 ms refresh timer |
| `TelemetryPanel.qml` | Endpoint controls, per-stream status/values, waveform Canvas |
| `tools/TelemetryProbe.cpp` | Hardware-free connection diagnostic |

The defaults are **temperature 1 Hz, pressure 10 Hz, waveform 100 Hz**. Network
reads never run on the Qt thread. The GUI copies a synchronized snapshot at
most 20 times per second and emits Qt property notifications from the GUI
thread. It does not post a Qt event for each waveform sample.

The client retains at most **500 waveform samples** (about five seconds at the
default rate). When the GUI is busy, the oldest samples are discarded. The
chart uses the server's monotonic elapsed seconds and a 0-3.3 V display range.
The initial history fills gradually. Each reconnect clears waveform history
and accepts a fresh sequence starting at zero, including server restarts.

Streams reconnect independently with a 0.5-second initial retry delay, doubling
to a maximum of 8 seconds. If a stream produces no sample for **5 seconds**, a
watchdog cancels it and starts recovery. Thus a silent network loss does not
leave readings marked connected indefinitely. Values disappear when their
stream disconnects; a temperature failure does not stop the waveform.
These timings assume the server's documented default rates.

Disconnect and normal application shutdown cancel every live gRPC context,
wake retry waits, and join all workers before destroying the view model.
The existing hardware cleanup remains in place.

## Shared contract and tests

The contract is copied from server commit
[`25acd8c4db9ef359d74888b2bdefb4407e01a01a`](https://github.com/bflink/GrpcServerApp/blob/25acd8c4db9ef359d74888b2bdefb4407e01a01a/proto/telemetry.proto).
Update both repositories together if the contract changes; generate bindings
locally rather than copying Windows-generated binaries or headers.

CTest includes the original hardware-logic tests plus real loopback tests for
all streams, bounded buffering, independent reconnection, stalled reads and
shutdown. An offscreen QML test loads the real two-tab window with a GPIO
stand-in, receives live fixture telemetry, and checks disconnect cleanup.
It requires the usual Qt Quick/Controls QML runtime modules. Tests do not
operate physical GPIO or I2C.

Validation before publishing: the full application compiled on Linux x86-64
using GCC 11, Qt 6, gRPC 1.30.2, protobuf 3.12.4 and an isolated libgpiod 2.2.2.
All 22 CTest tests passed. The final telemetry layout also passed an offscreen
test and a virtual-display screenshot check. Building on Debian 13/arm64 and
testing the actual Pi-to-Windows LAN connection remain the next on-device steps.

## Troubleshooting

- **All streams disconnected:** check the PC address, server process, LAN bind
  option, network reachability and Windows Firewall; try `telemetry_probe`.
- **Status includes `(14)`:** gRPC UNAVAILABLE, commonly an unreachable or
  restarting server. Retry is automatic.
- **No data / `(1)`:** the watchdog or user cancellation ended a stalled RPC.
- **GPIO busy:** stop the old Pi app; the probe itself never claims GPIO.
- **QML module missing in offscreen test:** install the corresponding Debian
  `qml6-module-*` package named in the error (Qt Quick, Controls, Window,
  Templates or QtQml WorkerScript).
- **Need to return to the earlier app:** stop it, switch back to your previous
  branch, reconfigure CMake, and rebuild. This feature is isolated on a branch.
