# Team Rocket buzzer

Independent Pico 2 W firmware for James's costume prop. All application code,
tests and enclosure sources are in this folder; the repository's existing Bazel
SDK/toolchain setup is shared. No turret hardware is needed.

Plays the supplied nine-note Team Rocket motif in a continuous loop (not the
full theme). Press once to start, then press again to stop. `melody.h` contains
the notes as `{frequency_hz, duration_ms}`; frequency `0` means silence.
The default tempo is 120 BPM: change `quarter_ms` to adjust it. Each note's
duration includes a 25 ms silent articulation gap, including at the loop boundary.

## Build and flash

From the repository root:

```sh
bazelisk build //team_rocket_buzzer:team_rocket_buzzer
bazelisk test //team_rocket_buzzer:button_test --platforms=@platforms//host --test_output=errors
```

Hold BOOTSEL while connecting the Pico by USB, then copy
`bazel-bin/team_rocket_buzzer/team_rocket_buzzer.uf2` to its boot drive.
ELF and BIN files are alongside it. The package's small Bazel transition selects
its own firmware name instead of the turret metadata from the root `.bazelrc`.
Use the public aggregate target above or `:firmware_uf2`, rather than the internal
`:application` target.

## Wiring and behavior

Disconnect power before wiring. GPIO numbers are different from physical pins.

| Connection | Pico 2 W |
| --- | --- |
| Normally-open momentary button, first terminal | GP15, physical pin 20 |
| Button, second terminal | GND, physical pin 18 |
| Bare passive piezo, signal via a 330 ohm series resistor | GP0, physical pin 1 |
| Piezo, other terminal | GND, physical pin 3 |
| Power | Micro-USB, from a USB power bank |

GP15 uses the internal pull-up: pressing connects it to ground. Use the
normally-open contacts of a panel switch. On four-legged tactile switches,
identify the two separate contact sides first.

This direct drive is for a low-current passive **piezo**. A magnetic buzzer or
speaker needs an appropriate transistor/amplifier driver; don't connect that
load directly to a GPIO. An active buzzer cannot reproduce changing pitches.

Power-up is silent. A stable 30 ms press starts looping; the next press stops
playback, even mid-note. Holding the button does not toggle repeatedly; release
for at least 30 ms between presses. Starting again restarts the motif from its
first note. A button held during power-up must
also be released before it can trigger. Edit the pin constants in `main.cpp`
if your wiring differs. Wireless and USB serial are not initialized.

Hardware check: power up, press once, and let the motif repeat. Release and press
again mid-note to confirm it stops. Hold the button to check it stays stopped,
then release and press to restart. Confirm silence at boot as well.

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
[official Pico 2 W mechanical drawing](https://datasheets.raspberrypi.com/picow/pico-2-w-datasheet.pdf).
The design assumes a board without headers, soldered leads, a nominal 12 mm
panel-mount momentary button with a shallow body (at most about 18 mm below the
panel), and a small piezo mounted behind the grille using foam tape. Keep the
Pico antenna end clear of wires and metal. Carry the power bank outside the
enclosure, with its cable entering through the USB opening.

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
