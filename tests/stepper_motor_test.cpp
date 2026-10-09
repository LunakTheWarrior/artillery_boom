#include "src/stepper_motor.h"

#include <array>
#include <stdexcept>

namespace {
std::array<bool, 4> coils{};
unsigned elapsedMs{};
void check(bool condition) {
    if (!condition) throw std::runtime_error("Incorrect stepper sequence");
}
}

void gpio_init(unsigned) {}
void gpio_set_dir(unsigned, bool) {}
void gpio_put(unsigned pin, bool value) { coils.at(pin) = value; }
void sleep_ms(std::uint32_t ms) { elapsedMs += ms; }

int main() {
    StepperMotor motor({0, 1, 2, 3});
    motor.stepForward();
    check(coils == std::array{true, true, false, false});
    motor.stepForward();
    check(coils == std::array{false, true, false, false});
    motor.stepBackward();
    check(coils == std::array{true, true, false, false});
    motor.stepBackward();
    check(coils == std::array{true, false, false, false});
    motor.stepBackward();
    check(coils == std::array{true, false, false, true});
    motor.stepForward();
    check(coils == std::array{true, false, false, false});
    for (unsigned i = 0; i < 8; ++i) motor.stepForward();
    check(coils == std::array{true, false, false, false});
    check(elapsedMs == 28);
}
