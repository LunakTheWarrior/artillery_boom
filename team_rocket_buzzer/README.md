# Team Rocket buzzer

Independent Raspberry Pi Pico H firmware for James's costume prop. All application code,
tests and enclosure sources are in this folder; the repository's existing Bazel
SDK/toolchain setup is shared. No turret hardware is needed.

Plays the supplied 309-note Team Rocket sequence in a continuous loop. Turn the
potentiometer above its start threshold to play, then below its stop threshold
to stop. In `notes.h`, each row is stored as
`{frequency_hz, duration_ms, gap_ms}`; a zero frequency is a rest. Note and gap
times use the millisecond values from the supplied MIDI conversion.

## Build and flash

From the repository root:

```sh
bazelisk build //team_rocket_buzzer:team_rocket_buzzer
bazelisk test //team_rocket_buzzer:button_test //team_rocket_buzzer:melody_test //team_rocket_buzzer:potentiometer_test --platforms=@platforms//host --test_output=errors
```

Hold BOOTSEL while connecting the Pico by USB, then copy
`bazel-bin/team_rocket_buzzer/team_rocket_buzzer.uf2` to its boot drive.
ELF and BIN files are alongside it. The package's small Bazel transition selects
its own firmware name instead of the turret metadata from the root `.bazelrc`.
Use the public aggregate target above or `:firmware_uf2`, rather than the internal
`:application` target.

## Wiring and behavior

Disconnect power before wiring. GPIO numbers are different from physical pins.

| Connection | Pico H |
| --- | --- |
| Potentiometer outer terminal | 3V3(OUT), physical pin 36 |
| Other potentiometer outer terminal | GND, physical pin 38 |
| Potentiometer center/wiper | GP26 / ADC0, physical pin 31 |
| Bare passive piezo, signal via a 330 ohm series resistor | GP0, physical pin 1 |
| Piezo, other terminal | GND, physical pin 3 |
| Power | Micro-USB, from a USB power bank |

The potentiometer can be a B10K. Connect only to the Pico's 3.3 V and GND; do
not connect 5 V to the ADC. Turn the knob past about 60% to start the melody,
then below about 40% to stop. The gap between those thresholds prevents jitter
from repeatedly starting and stopping playback. Swap the two outer terminals
if you want the knob direction reversed. The potentiometer is temporary; the
enclosure's panel hole is still intended for the button you plan to buy later.

This direct drive is for a low-current passive **piezo**. A magnetic buzzer or
speaker needs an appropriate transistor/amplifier driver; don't connect that
load directly to a GPIO. An active buzzer cannot reproduce changing pitches.

With the potentiometer turned down, power-up is silent. Turning it above the
upper threshold starts the song loop; turning it below the lower threshold
stops playback, even mid-note. Turning it up again restarts from the first note.
Edit `potentiometer_pin` in `main.cpp` if your wiring differs. USB CDC serial
debugging is enabled. Connect the Pico to your computer with a data-capable USB
cable and open its serial port in a terminal (115200 baud is a common setting;
USB CDC does not depend on that baud rate). The firmware reports when it is
ready, prints the raw ADC reading and idle/playing state about once per second,
and logs each trigger and song start/stop. The readings range from 0 to 4095;
turning the potentiometer should move them across the start/stop thresholds.

Hardware check: set the pot low, power up, and confirm silence. Turn it above
the upper threshold and let the melody repeat; turn it below the lower threshold
mid-note to confirm it stops, then turn it back up to restart.

## Printable Pokédex-style enclosure

`enclosure/pokedex.scad` is a parametric two-piece red handheld costume prop,
82 × 128 × 26.4 mm assembled. It has a recessed decorative screen, lens and
indicator details, a D-pad recess, a real button hole and piezo grille. The
faceplate snaps on with four releasable clips for access to BOOTSEL and wiring;
it is not a hinged model. No enclosure or Pico mounting screws are needed.
Paint the lens blue, indicators yellow/green and screen dark grey, then add a
red or white R decal for the Team Rocket look.

The board rests on four posts and is held by four printed edge clips, with the
USB port toward the top opening. The
mounting grid is 17.78 × 48.26 mm for the 21 × 51 mm PCB, using the
[official Pico mechanical drawing](https://datasheets.raspberrypi.com/pico/pico-datasheet.pdf).
The design assumes a board without headers, soldered leads, a nominal 12 mm
panel-mount momentary button with a shallow body (at most about 18 mm below the
panel), and a small piezo mounted behind the grille using foam tape. Keep the
Pico antenna end clear of wires and metal. Carry the power bank outside the
enclosure, with its cable entering through the USB opening.

The Pico H comes with headers already soldered on. Those headers may interfere
with the enclosure's support posts and board clips; check the fit before
installing it, or use a bare Pico board for this enclosure.

Measure your actual button barrel, nut, buzzer and cable before printing. Adjust
the parameters at the top of the SCAD file. The default hole is 12.4 mm; the
button nut must fit beside the grille and case wall. Leave slack in the button
wires to remove the faceplate. This is an unprinted prototype; verify fit with
your hardware before relying on it for the costume.

With OpenSCAD installed, generate the printable parts through Bazel:

```sh
bazelisk build //team_rocket_buzzer:enclosure
```

Import `bazel-bin/team_rocket_buzzer/stl/base.stl` and `face.stl` into Bambu
Studio at 100% scale as separate objects. Commit the SCAD source and Bazel rules;
generated STL files are ignored by Git. Old manual exports are not updated by
this command.

Both parts are already oriented for printing: base floor down, face exterior
down. Use PETG for the flexible clips; PLA clips may break with repeated opening.
Start with 0.2 mm layers, three perimeters and 15–20% infill.
Support may be needed over the USB opening; the shallow face recesses bridge.
Keep support out of the clip gaps and release windows. The small hook ledges
are intended to print without support. Check these areas in the sliced preview.

Press the bare Pico gently between the edge clips onto its support posts, with
USB facing the top opening. Keep solder and wires clear of the clip contact
areas on both long edges. The design assumes a 1.0 mm PCB; measure yours and
adjust `pcb_thickness` if needed. To remove it, gently flex the clips outward
while lifting the board; do not pull on the USB connector.

Align the lid and press each side until all four hooks engage. To open, press
the hooks inward through the side windows with a small blunt tool while gently
lifting that edge. Leave wire slack so the lid can lift away. Do not force a
tight clip: tune `lid_clearance`, `lid_engagement`, or `pcb_clearance` and reprint.
The button still needs its supplied mounting nut, and the piezo uses foam tape.

This snap-fit revision has not been physically printed or fit-tested. Inspect
`part="assembly"` before exporting and test the clips gently before installing
electronics.
