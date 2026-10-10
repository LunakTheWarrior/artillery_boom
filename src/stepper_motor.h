#pragma once

#include <stdint.h>

// Platform-independent half-step sequence for a four-coil unipolar stepper.
// Hardware-specific code applies the returned coil states to GPIO pins and
// decides when to request the next step. This header only needs the C integer
// types so it also builds with the small AVR toolchain used for the Uno.
class StepperMotor {
public:
    const uint8_t* stepForward() {
        position_ = (position_ + 1) % 8;
        return phase(position_);
    }

    const uint8_t* stepBackward() {
        position_ = (position_ + 7) % 8;
        return phase(position_);
    }

private:
    static const uint8_t* phase(uint8_t index) {
        static const uint8_t sequence[8][4] = {
            {1, 0, 0, 0},
            {1, 1, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 1, 0},
            {0, 0, 1, 0},
            {0, 0, 1, 1},
            {0, 0, 0, 1},
            {1, 0, 0, 1},
        };
        return sequence[index];
    }

    uint8_t position_{0};
};
