# Artillery Boom

C++ firmware for a Raspberry Pi Pico 2 W scanning turret, with stepper motors,
Modulino sensors, and a buzzer.

## Build with Bazel

The supported build host is **Linux x86_64**, matching CI and the downloaded ARM
toolchain. Install Bazelisk (or Bazel 8.1.0) plus a host C/C++ compiler, Git,
Python 3, and xz. On Ubuntu:

```sh
sudo apt-get install build-essential git python3 curl xz-utils
bazelisk build //...
```

Bazelisk reads `.bazelversion`. Bazel downloads the Pico SDK **2.3.1**, Arm GNU
Toolchain **15.2.Rel1** (`x86_64-arm-none-eabi`), and its other dependencies.
The SDK and compiler archives are checksum-pinned, and `MODULE.bazel.lock` records
module resolution. No CMake installation or `PICO_SDK_PATH` is needed.

Outputs:

- `bazel-bin/light_turret.elf` — linked firmware and binary metadata
- `bazel-bin/light_turret.bin` — raw flash image
- `bazel-bin/light_turret.uf2` — copy to the Pico's BOOTSEL volume to flash

`bazelisk build //:light_turret` builds just the firmware and all three formats.
`bazelisk build //...` also compiles the currently unused light sensor driver.
USB serial is enabled and UART serial is disabled. The default is a Release build
with GNU C11 / GNU C++23. For debugging:

```sh
bazelisk build //:light_turret -c dbg \
  --@pico-sdk//bazel/config:PICO_BAZEL_BUILD_TYPE=Debug
```

If the compiler archive is already downloaded into `.tools/`, Bazel can reuse it:

```sh
bazelisk build //... --distdir=.tools
```

The local extracted compiler, when present, is at
`.tools/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi/bin/`.
Bazel uses its own checksum-verified extraction; adding the local compiler to
`PATH` is unnecessary.

## Container build

```sh
docker build -t artillery-boom .
docker create --name artillery-firmware artillery-boom
docker cp artillery-firmware:/firmware ./firmware
docker rm artillery-firmware
```

The image builds with Bazel and contains ELF, BIN, and UF2 outputs in `/firmware`.
The Docker build also requires a Linux x86_64 host (or suitable emulation).

## Build integration

Firmware sources and headers live in `src/`. `BUILD.bazel` defines the firmware,
optional light driver, binary metadata, and image conversion. `.bazelrc` selects the Pico 2 W / RP2350 and serial settings.
GitHub Actions builds all targets with the committed lockfile and uploads the
three firmware images.

Three small patches adapt the SDK's native Bazel build:

- `bazel/arm_toolchain.patch` upgrades its Linux x86_64 compiler from 13.2.Rel1
  to the same 15.2.Rel1 archive used by the previous GitHub workflow.
- `bazel/tinyusb_device_only.patch` excludes TinyUSB's example board setup from
  the device library. Pico USB stdio initializes the device stack itself; the
  example setup unnecessarily calls UART stdio even when UART is disabled.
- `bazel/usb_reset_alwayslink.patch` retains the USB reset interface driver and
  BOS descriptor callbacks, which TinyUSB references weakly. Without this,
  Linux reports `can't set config #1, error -32` and creates no serial device.

When changing SDK/compiler versions, review these patches, update archive
checksums, run `bazelisk build //...`, and commit the updated module lockfile.

Two additional dependency patches keep the host build warning-free:

- `bazel/picotool_warnings.patch` fixes initialization order, integer comparisons,
  unused helpers, and an uninitialized flag in picotool 2.3.0.
- `bazel/rules_cc_deprecations.patch` removes deprecation markers from the six
  legacy feature placeholders that rules_cc 0.1.1 still requires internally to
  define their replacements. It does not change toolchain behavior.

Review these patches when upgrading picotool or rules_cc. Compiler warning
flags remain enabled.

## Distance sensor

The Modulino Distance driver initializes the VL53L4CD and reads distances in
millimeters using C++23 typed registers and `std::expected` error handling.
The firmware reports measurements over USB serial. See
[the distance driver guide](docs/modulino-distance.md) for wiring, API examples,
initialization details, and hardware validation.

Run the simulated-sensor tests on the host:

```sh
bazelisk test //tests:modulino_distance_test --platforms=@platforms//host --test_output=errors
```

Firmware distributions also include `THIRD_PARTY_NOTICES.txt` for ST's sensor
configuration and algorithms.

## Sensor reference

The light sensor driver follows the
[LTR-381RGB-01 datasheet](https://optoelectronics.liteon.com/upload/download/DS86-2018-0007/LTR-381RGB-01_Final_DS_V1.8.PDF).

## Distance following

The sensor must turn with the motor on GPIO **21, 20, 19, 18**. At power-on,
physically center the mechanism: the firmware assumes position 5672 within the
existing 0–11343 half-step travel. There is no homing switch or absolute position
feedback. The other motor is unused.

The motor sweeps until it sees a valid object within **1000 mm**, then repeatedly
scans a local ±256 half-step window and turns toward the average position of the
closest returns (40 mm tolerance). This lets it follow objects moving across the
sensor's view. It resumes the broad sweep when the local scan loses the object.
It follows nearby surfaces, including stationary ones; a single range sensor
cannot identify motion independently or distinguish people from other objects.
Fast objects may leave the local window and need to be reacquired.

Measurements are taken while stationary, and readings that overlap movement are
discarded. Invalid samples do not count as targets. Sensor errors/timeouts pause
movement and restart the sensor, preserving the motor's estimated position.
Tune `maxDistanceMm`, `radius`, `stride`, and `distanceToleranceMm` in
`src/distance_tracker.h` for your scene and mechanism. Smaller strides improve
angular sampling but slow the scan. The motor retains its existing 2 ms half-step
interval. The piezo on GPIO **0** plays an alternating rising/falling robot
"woop" when an object is acquired. Each sound briefly pauses movement for 180 ms;
it plays once per acquisition, with a 3-second cooldown to avoid chatter.

Run all host sensor/motor/tracking tests:

```sh
bazelisk test //tests:all --platforms=@platforms//host --test_output=errors
```

Hardware check: center before powering on, place an object within 1 m, move it
slowly across the scan axis in both directions, then remove it. Confirm local
following, reacquisition, and travel limits. Disconnect the sensor and confirm
that stepping pauses during retries. Tracking tuning still needs this physical
validation; host tests simulate the scene and GPIO sequence.
