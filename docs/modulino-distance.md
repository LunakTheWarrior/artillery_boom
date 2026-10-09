# Modulino Distance (VL53L4CD)

The driver is written for the VL53L4CD on the Arduino Modulino Distance module.
It uses Pico I2C1 with SDA on GPIO 2 and SCL on GPIO 3 in the application. Connect
3.3 V and GND as well. The default 7-bit bus address is `0x29`; ST's `0x52` is the
8-bit address including the read/write bit. XSHUT must be high for the sensor to
operate. The module's normal power-up configuration is used; no separate XSHUT
or GPIO1 connection is needed for polling.

Only the `0xEBAA` model ID is accepted. This driver does not assume that another
sensor with the same I2C address (including a VL53L4ED variant) is compatible.

## API

```cpp
using namespace std::chrono_literals;
ModulinoDistance distance(i2c1, 2, 3);

auto status = distance.begin(20ms).and_then([&] {
    return distance.startRanging();
});
if (!status) {
    printf("Sensor error: %s\n", ModulinoDistance::errorName(status.error()));
    // Retry initialization or report the failure to the application.
} else {
    auto mm = distance.readDistance(100ms);
    if (mm) {
        printf("Distance: %u mm\n", static_cast<unsigned>(*mm));
    } else {
        printf("Read error: %s\n", ModulinoDistance::errorName(mm.error()));
    }
}
```

`begin()` configures the bus and initializes the sensor, leaving it stopped.
`startRanging()` starts measurements and discards the first sample, as in ST's
ULD sequence. `stopRanging()` stops the device. Stop before changing the timing
with `setRangeTiming(budget, interval)`. Budgets are 10–200 ms. An interval of
zero selects continuous operation; a nonzero interval must exceed the budget
and selects autonomous low-power operation.

`readMeasurement()` polls without waiting for a new measurement. It returns
`Error::NotReady` when no new sample is available. A successful result includes:

- Distance in millimeters and ST's normalized range status (`0` means valid).
- Raw range status and sample stream count for diagnostics.
- Effective SPAD count, sigma in millimeters (including quarter-mm precision),
  and signal/ambient rates in kcps, both total and per SPAD.

Check `measurement.valid()` before using the distance. Invalid samples still
carry diagnostics and are acknowledged so the next conversion can proceed.
Unknown status codes and zero-SPAD results are invalid. Rates use 32-bit storage
to avoid overflow when converting the sensor's raw values.

`readDistance(timeout)` waits for a sample and returns only a valid distance;
it reports `InvalidMeasurement` for an invalid sample. A valid distance of zero
is not an error. Neither read API returns a cached sample on failure.

Register handles in `VL53L4CDRegisters.h` carry their value types, so
`readRegister(vl53l4cd::registers::modelId)` reads a 16-bit value, while
`readRegister(vl53l4cd::registers::firmwareStatus)` reads one byte. They cover
identity, boot, calibration, GPIO/interrupts, timing, thresholds, and results.
Register operations use explicit big-endian encoding; they do not depend on the
host CPU's byte order or pointer casts. Raw register access is intended for
inspection after the bus has been configured by `begin()`. Direct writes can
invalidate driver state, especially reset, address, and mode changes.

## Initialization and error handling

Initialization waits up to one second for firmware status `0x03`, verifies the
model ID, stops any previous ranging, writes ST's 91-byte configuration to
`0x002D..0x0087`, starts the VHV calibration cycle, waits for completion, clears
the interrupt, stops, and applies the post-calibration VHV/DSS settings. Timing
registers are calculated from the device's oscillator registers. The factory
offset and crosstalk calibration registers below `0x002D` are preserved.

Reads use a repeated START after the two-byte register index and a STOP after
the data. Each I2C transfer has a 10 ms timeout. Boot/calibration waits are bounded
to one second; ranging waits accept a timeout. A wait can exceed its requested
duration by the final bounded I2C poll. Transfer failures, readiness timeouts,
wrong model IDs, invalid arguments, and invalid measurements have separate
error values. Failed or partial initialization/timing changes require `begin()`
again; the driver attempts to stop the sensor while preserving the original
error. A failed physical bus cannot guarantee that a stop command reaches it.

The application prints live distances over USB serial and retries initialization
on errors. A one-second sample watchdog also restarts a sensor that continues
responding on I2C but stops producing results. Scanning still uses the existing
IR placeholder: millimeters are not passed to the IR threshold logic.

This does not perform target-dependent offset or cover-glass crosstalk
calibration; those require a known physical target/setup. It retains the factory
calibration and performs the required startup VHV calibration.

## Validation

```sh
bazelisk test //tests:modulino_distance_test \
  --platforms=@platforms//host --lockfile_mode=error --test_output=errors
bazelisk build //... --lockfile_mode=error
```

The host test compiles the production C++23 driver against a simulated Pico
interface. It checks register byte order, the initialization sequence and
calibration, timing vectors, interrupt polarity, fresh sample consumption,
invalid statuses, zero-SPAD results, and bus/boot/ranging failure recovery.
The test is skipped in wildcard builds for the Pico platform. CI runs it
explicitly on the host before building firmware.

Hardware validation still requires the physical module: flash the UF2, open USB
serial, verify initialization and plausible distances at known positions, then
check invalid-range behavior and recovery after disconnecting/reconnecting the
sensor. Host tests cannot establish optical accuracy or physical bus behavior.

## References and attribution

- [Arduino Modulino Distance hardware](https://docs.arduino.cc/hardware/modulino-distance)
- [ST VL53L4CD ULD manual, UM2931](https://www.st.com/resource/en/user_manual/um2931-a-guide-to-using-the-vl53l4cd-ultra-lite-driver-uld-stmicroelectronics.pdf)
- [ST's reference driver at revision b6ed0e2](https://github.com/stm32duino/VL53L4CD/tree/b6ed0e21735ccd0f220cd8f35cf896f9e11d463d)

The C++ driver is rewritten for this project. The mandatory configuration bytes,
register definitions, status mapping, and timing equations follow ST's ULD.
ST's BSD-3-Clause notice is in `licenses/STMicroelectronics-BSD-3-Clause.txt` and
is included as `THIRD_PARTY_NOTICES.txt` with the firmware outputs and CI artifacts.
