#include "stepper_motor.h"

#include <gtest/gtest.h>

namespace {

void expectCoils(const uint8_t* actual, const uint8_t (&expected)[4]) {
    for (unsigned coil = 0; coil < 4; ++coil) EXPECT_EQ(actual[coil], expected[coil]);
}

TEST(StepperMotorTest, HalfStepSequenceMovesInBothDirections) {
    StepperMotor motor;
    const uint8_t phase0[4] = {1, 0, 0, 0};
    const uint8_t phase1[4] = {1, 1, 0, 0};
    const uint8_t phase2[4] = {0, 1, 0, 0};
    const uint8_t phase7[4] = {1, 0, 0, 1};

    expectCoils(motor.stepForward(), phase1);
    expectCoils(motor.stepForward(), phase2);
    expectCoils(motor.stepBackward(), phase1);
    expectCoils(motor.stepBackward(), phase0);
    expectCoils(motor.stepBackward(), phase7);
    expectCoils(motor.stepForward(), phase0);

    const uint8_t* state = nullptr;
    for (unsigned step = 0; step < 8; ++step) state = motor.stepForward();
    expectCoils(state, phase0);
}

} // namespace
