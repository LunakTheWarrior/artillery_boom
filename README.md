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

`BUILD.bazel` defines the firmware, optional light driver, binary metadata, and
image conversion. `.bazelrc` selects the Pico 2 W / RP2350 and serial settings.
GitHub Actions builds all targets with the committed lockfile and uploads the
three firmware images.

Two small patches adapt the SDK's native Bazel build:

- `bazel/arm_toolchain.patch` upgrades its Linux x86_64 compiler from 13.2.Rel1
  to the same 15.2.Rel1 archive used by the previous GitHub workflow.
- `bazel/tinyusb_device_only.patch` excludes TinyUSB's example board setup from
  the device library. Pico USB stdio initializes the device stack itself; the
  example setup unnecessarily calls UART stdio even when UART is disabled.

When changing SDK/compiler versions, review these patches, update archive
checksums, run `bazelisk build //...`, and commit the updated module lockfile.

## Sensor reference

The light sensor driver follows the
[LTR-381RGB-01 datasheet](https://optoelectronics.liteon.com/upload/download/DS86-2018-0007/LTR-381RGB-01_Final_DS_V1.8.PDF).
