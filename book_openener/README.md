# Book opener

Prototype for an **Arduino Uno SMD (ATmega328P)**, a Modulino Distance, and a
152 × 230 × 10 mm flimsy paperback, standing 20° back from vertical.

The front cover and first half of the pages move together, exposing one fixed
middle spread. This is not an individual page-turning machine.

**Motor: your confirmed 28BYJ family**, using the turret's eight-pattern
half-step sequence. Wiring and spool dimensions target the common **5 V
28BYJ-48 with a ULN2003 driver**; check the voltage label and shaft dimensions
on your particular unit before wiring or printing the socket.
A four-wire bipolar motor or STEP/DIR driver needs different firmware and wiring.

## What is included

- `firmware/book_openener/book_openener.cpp`: standard C++ translation unit
  with Arduino `setup()`/`loop()` entry points; no `.ino` preprocessing.
- `src/stepper_motor.h`: shared, platform-independent 28BYJ half-step sequence
  used by both the Pico 2 W turret and this Uno firmware. Each platform keeps
  its own GPIO/timing adapter; the coil sequence itself is shared.
- `ModulinoDistance.h/.cpp` beside the sketch: AVR port of **our existing Pico
  driver**, with the same sensor configuration/calibration, timing equations,
  typed registers, result decoding, status mapping and zero-SPAD rejection.
- `Presence.h`: distance hysteresis and dwell logic.
- `enclosure/book_opener.scad`: parametric back support, moving paddle, feet,
  cord guide, motor cradle and spool; Bazel exports the STL files under `bazel-bin/book_openener/enclosure/`.
- Bazel test targets: host tests for the driver, sensor failures, presence timing,
  clock rollover, calibration and motion stops.

This is a first mechanical prototype, not a physically tested assembly. Printed
fit, elastic force, motor torque and opening travel must be tested on your book.
The original turret driver and firmware have not been modified.

## Parts to obtain

Besides the Uno, Modulino and confirmed motor/driver:

- A **bidirectional I²C logic-level shifter**, such as a BSS138-based module,
  with pull-ups on both voltage sides.
- A regulated **5 V motor supply** (1 A gives reasonable headroom for this
  assumed motor); check the motor's voltage label.
- One normally-open lever microswitch for the book's closed position.
- Thin low-stretch cord, a very weak elastic band, fabric hinge tape, soft ribbon
  or small padded binder clips, and cable ties.
- A heavy base board or secure table attachment. Opening the book shifts its
  weight sideways; the printed feet alone are not a stable anchor.

The switch and level shifter are needed before automatic operation. Do not
replace the closed-position switch with a permanently grounded wire.

## Wiring

Disconnect power while assembling. The Uno is 5 V logic; the Modulino is a
3.3 V device. **Do not connect Uno SDA/SCL directly to the Modulino.**

| From | To |
| --- | --- |
| Uno 5V | Level shifter HV |
| Uno 3.3V | Level shifter LV and Modulino 3V3 |
| Uno A4 / SDA | Shifter HV channel 1 |
| Shifter LV channel 1 | Modulino SDA |
| Uno A5 / SCL | Shifter HV channel 2 |
| Shifter LV channel 2 | Modulino SCL |
| Uno GND | Shifter GND, Modulino GND, ULN2003 GND and motor supply negative |
| Uno D8, D9, D10, D11 | ULN2003 IN1, IN2, IN3, IN4 respectively |
| External regulated 5 V positive | ULN2003 motor supply + |
| Motor's keyed plug | ULN2003 motor connector |
| Uno D2 | Microswitch NO contact |
| Uno GND | Microswitch COM contact |

Power the Uno through USB during setup. Do not join the external motor supply's
positive to the USB-powered Uno's 5V pin, and do not power the motor through an
I/O pin or the Uno's 3.3V output. The grounds must be common.

Identify the Modulino's labelled pins, not wire colours. The Uno 3.3 V supply is
reserved for the sensor and low-voltage shifter side here; do not add other loads.

## Mechanical assembly

1. Print one **back**, one **paddle**, **two feet**, one **base**, one **guide**,
   one **motor_mount**, one **spool**, one **uno_tray**, and one **sensor_mount**.
   Import each STL at 100%, millimetres. The Uno tray holds the controller on
   the printed base.
   All files are in print orientation; feet print on their broad triangular side.
   All printable parts fit within a 256 × 256 × 256 mm build volume: the base is
   250 × 245 × 14 mm, the backplate is 164 × 242 mm, and each foot is about
   110 × 227 mm. For the complete picture, build `//book_openener:assembly_view`
   and open `bazel-bin/book_openener/enclosure/assembly.3mf` in Bambu Studio.
   Add ballast under the printed base or fasten it to the table; the feet alone
   are not heavy enough to prevent tipping. In the assembly view, the green
   board between the guide and motor is the ULN2003 driver (a placement aid,
   not a printed part). Put its motor connector toward the motor mount.
2. Stand the two full-height feet on their 110 mm-long base edges, spaced near
   the left and right edges of the backplate. Attach their sloping faces to the
   rear of the backplate, keeping the bottom edges level and the book ledge at
   the bottom. This supports the full height of the upright book at 20° back.
   Secure both feet to the printed base with strong adhesive, then add ballast
   beneath the base or fasten it down so the assembly cannot tip when the paddle
   moves. The guide's four holes line up with the four 4.2 mm holes in the base;
   fasten it through those holes or glue it down. The motor-mount tie slots line
   up with two long slots in the base; run a cable tie through each matching
   slot pair and around the mount. The Uno tray's four tie slots also align with
   openings in the base.
   Facing the open book, the **guide** is on the right-hand side of the base,
   ahead of the book; its eye points along the red cord toward the spool. The
   **motor mount** is immediately to the right of the guide, with the motor
   lying horizontally and its shaft/spool facing forward toward the guide.
   Stick the ULN2003 driver board in the space between them, with its five-pin
   motor socket facing the motor. Keep that board clear of the spool and cord.
   Attach the Uno tray to the base with four small cable ties through
   its slots. Press the Uno onto the four flexible posts, USB-B and barrel-jack
   side facing the tray's open edge. The posts locate and retain the board without
   screws. Connect and route the USB, power, and I/O wires before tightening ties.
   Plug the motor's five-wire keyed connector into the ULN2003 board; the motor
   wires do **not** connect directly to the Uno. Run the driver input wires to
   Uno D8–D11 in IN1–IN4 order, and supply the driver from the separate regulated
   5 V supply. Route the motor lead a short distance from the motor to the driver,
   then route the driver/Uno wiring around the base's outside right and rear
   edges, securing it with cable ties. Keep wires away from the red cord, spool,
   and moving paddle. The assembly preview shows this route in black.
   Push the sensor mount's two clips over the fixed backplate's top edge,
   centered above the book. Snap the Modulino Distance onto its four posts with
   its sensor face pointing toward the visitor. The frame leaves the optical
   center open; route the sensor cable behind the backplate.
3. Put the paperback on the ledge with its back cover against the fixed plate.
   Secure the back cover and rear half of the pages using padded clips or soft
   ribbon through the back's slots. Avoid adhesive on the book itself.
4. Slip the paddle into the chosen middle spread, under the moving front half.
   Secure the front cover and those pages to the paddle with soft ribbon or
   padded clips near the outer edge. Keep the visible middle page clear.
5. Connect the paddle's spine-side edge to the fixed plate using a **loose
   fabric-tape hinge**. Leave a flexible bridge around the book's spine and
   thickness; do not make the paperback follow a tight rigid pivot.
   Test opening and closing by hand before connecting the motor.
6. Use thin, low-stretch **braided cord or fishing line**; this is a flexible
   tether, not a rigid rod or electrical wire. Tie it to the paddle's projecting
   outer-edge tab and route it through the guide eye. The guide is a fixed
   fairlead: it keeps the line clear of the book edge and steers it toward the
   spool. In the assembly view it sits outboard of the moving page edge, with
   its mounting holes on the base. Keep the line clear of the visible spread.
7. Secure the motor body in the cradle with two cable ties, with its output
   shaft pointing toward the spool. The spool sits on the exposed shaft end;
   it does not overlap the motor body. Test the socket before pressing it on:
   adjust `shaft_d` and `shaft_flats` to your measured motor. Tie the cord
   through the spool flange hole and wind neatly on the barrel, leaving a little
   cord on the barrel when the book is closed.
8. Add a weak elastic return between the paddle and fixed support, positioned
   to pull the paddle shut. Move it by hand through the entire intended travel:
   the elastic must always favour closing and must not go over centre.
   Start with a **60–90° opening**, not a fully flat 180° opening.
   The 28BYJ-48 actively reverses to pay the cord out while closing; its gearbox
   is not expected to freewheel. The weak elastic helps the pages follow the
   loosening cord back to closed. Keep the band weak enough that opening does
   not stall the motor.
9. Mount the lever switch so the **paddle** operates it only when closed. Attach
   it with tape/adhesive after experimenting; there is no model-specific switch
   bracket. The switch must operate before the cord becomes slack.
10. Check that the sensor field of view stays clear of the top of the book,
    paddle and cord throughout their movement. Its bracket should remain fixed
    while the book opens and closes.

The fixed plate and paddle are supports, not a complete rigid linkage. Guide
placement and elastic attachment are intentionally adjustable. If the motor
stalls, reduce elastic tension, friction or opening angle; do not force the spine.
Keep fingers away while operating: the distance sensor is a presence trigger,
**not an obstruction detector**. A fault releases the coils, so the weak elastic
may close the book.

Try 0.2 mm layers, 3 walls, 15–20% infill. PETG suits the paddle's flexing.
Inspect the spool socket roof, guide eye, motor cradle and both boards' split
snap posts in the slicer; local support may be needed. The posts match the Uno's
mounting pattern from [Arduino's official board CAD](https://docs.arduino.cc/hardware/uno-rev3-smd)
and the Modulino's 32 × 16 mm, 3.2 mm-hole pattern from its
[datasheet](https://docs.arduino.cc/resources/datasheets/ABX00102-datasheet.pdf).
Test the clips gently; if tight, widen their split or reduce the snap-head
diameter in SCAD. Smooth the guide hole so it does not abrade the cord.
Do not print everything before checking the motor socket and book fit.

Build everything from the repository root:

```sh
bazelisk build //book_openener:book_openener
```

Or build firmware or printed parts separately:

```sh
bazelisk build //book_openener:firmware
bazelisk build //book_openener:enclosure
bazelisk build //book_openener:assembly_view
```

Outputs are `bazel-bin/book_openener/book_openener.hex`, `.elf`, `.map`,
`.size.txt`, notices, `bazel-bin/book_openener/enclosure/*.stl`, and the
assembly preview 3MF. Print the foot twice. Bazel downloads checksum-pinned Arduino AVR core 1.8.8 and AVR GCC
7.3.0 (Arduino 7); no local Arduino IDE, Arduino CLI or installed cores are
required to build. The AVR build currently runs on Linux x86_64 and needs Python 3.
STL targets additionally require `openscad` on PATH; its version is host-provided.
The Uno action supplies its own AVR flags, independent of the root Pico/C++23
defaults. It checks flash and static SRAM capacity after linking.

The sketch is compiled as ordinary C++ with explicit includes/function order;
the Bazel build does not use Arduino's automatic prototype generator.

## Upload and calibrate

1. Build `//book_openener:firmware` with Bazel.
2. Upload the generated HEX using avrdude and the Uno bootloader, substituting
   your actual serial port:

   ```sh
   avrdude -p atmega328p -c arduino -P /dev/ttyACM0 -b 115200 -D \
     -U flash:w:bazel-bin/book_openener/book_openener.hex:i
   ```

   Uploading requires avrdude installed locally and access to the device.
   Bazel builds never flash connected hardware. This is the ordinary Uno target;
   “SMD” does not require a different firmware image.
3. Open a serial terminal at **115200 baud**. At power-on the motor stays off.
   Sensor initialization failures print a fault.
4. With the motor released, close the book by hand, arrange the cord without
   tension/slack, and make sure the closed switch is pressed.
5. Send `z` to establish zero. Send `+` once to wind 200 half-steps, waiting
   for each jog to finish. If this pays cord out, disconnect power and change
   `OPEN_DIRECTION` from `1` to `-1`, re-upload and start again.
6. Jog until the book opens comfortably. `-` unwinds 200 steps but does not
   go below zero. Send `p` to inspect position/switch state. Send `s` to save
   the current open position **in RAM**.
7. Send `c` to close. The motor pays out until the closed switch operates.
   It allows only 400 extra half-steps beyond estimated zero, then faults.
   Correct switch placement/cord winding if this fails; don't increase travel
   to mask a jam or broken switch.
8. With the book closed and valid sensor readings available, send `a` to arm
   automatic operation. Send `!` at any time to stop and release the motor.

Recalibrate after every power cycle/reset. This prototype deliberately does not
restore an unverified position or opening distance from EEPROM. The maximum
jog travel is 20,000 half-steps; each move also has a 120-second timeout.
The closed switch must release within the first 500 opening half-steps.

On a motor/position fault, manually close, correct the cause, and send `z`
to recalibrate. On a sensor/I²C fault, fix wiring and reset the Uno first.
The built-in LED stays on when armed and blinks on a fault.

## Automatic behaviour

- Valid distance **below 300 mm continuously for 500 ms**: open.
- Once open, valid distance **above 450 mm continuously for 2 seconds**: close.
- Between thresholds: keep the existing target.
- Invalid readings cancel a pending threshold dwell. They never mean “away.”
- In automatic mode, **1.5 seconds without a valid measurement** stops and
  releases the motor; I²C errors fault immediately.
- A returning visitor can reverse a close after the approach dwell.

**Aim at a stationary background roughly 600–900 mm away when no visitor is
present.** This gives the sensor a valid “away” reading. Pointing into empty
space outside its range produces invalid readings, so the book faults instead
of assuming it should close. Tune thresholds in `Presence.h` for your setup.

## Driver port and validation

This uses our driver, not Arduino_Modulino or Pololu's library. Its AVR port:

- Replaces Pico I²C with bounded Wire transactions and repeated START reads.
- Uses `millis()`/`delay()` with rollover-safe elapsed-time checks.
- Replaces C++23 expected/span/array/concepts with small allocation-free C++11
  types in `Portable.h`; durations are unsigned milliseconds.
- Retains the 91 configuration bytes, calibration sequence, fixed-point timing
  equations, typed register addresses and detailed measurement/status fields.
- Reads the 15-byte result block within AVR Wire's 32-byte buffer and writes
  configuration one register at a time.
- Keeps the original ST attribution and bundled BSD-3-Clause license.

The port is a separate copy so the Pico project is unaffected. Future fixes to
`src/ModulinoDistance.*` should be reviewed for this port too.

Run the tests through Bazel:

```sh
bazelisk test //book_openener:control_test //book_openener:driver_parity_test \
  --platforms=@platforms//host --test_output=errors
```

The parity test also checks that the port's configuration bytes, status mapping,
and register definitions match our original Pico driver.

Compilation and simulated tests do not validate wiring, actual sensor behaviour,
motor torque or printed fit. Test the sensor and motor on the bench, then run the
mechanism slowly without the book before adding it.

Reference hardware documentation:
[Uno SMD](https://docs.arduino.cc/hardware/uno-rev3-smd),
[Modulino Distance](https://docs.arduino.cc/hardware/modulino-distance/),
[Modulino datasheet](https://docs.arduino.cc/resources/datasheets/ABX00102-datasheet.pdf).
