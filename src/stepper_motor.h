#pragma once

#include <cstdint>
#include <array>
#include "hardware/gpio.h"
#include "pico/stdlib.h"

class StepperMotor
{
    public:
        StepperMotor(const std::array<std::uint32_t, 4> pins)
            : pin1_(pins[0])
            , pin2_(pins[1])
            , pin3_(pins[2])
            , pin4_(pins[3])
        {
            gpio_init(pin1_);
            gpio_set_dir(pin1_, GPIO_OUT);
            gpio_init(pin2_);
            gpio_set_dir(pin2_, GPIO_OUT);
            gpio_init(pin3_);
            gpio_set_dir(pin3_, GPIO_OUT);
            gpio_init(pin4_);
            gpio_set_dir(pin4_, GPIO_OUT);
        }

        void
        stepForward();

        void
        stepBackward();


    private:
        std::uint32_t pin1_;
        std::uint32_t pin2_;
        std::uint32_t pin3_;
        std::uint32_t pin4_;

        std::uint32_t position_{0};

        static constexpr std::array<std::array<std::uint8_t, 4>, 8> fullSteps
        {
            {
                {1,0,0,0},
                {1,1,0,0},
                {0,1,0,0},
                {0,1,1,0},
                {0,0,1,0},
                {0,0,1,1},
                {0,0,0,1},
                {1,0,0,1},
            }
        };

        static constexpr std::uint32_t stepsPerFullRotation_{4096}; //When half stepping

};

inline void
StepperMotor::stepForward()
{
    position_ = (position_ + 1) % 8;
    gpio_put(pin1_, fullSteps[position_][0]);
    gpio_put(pin2_, fullSteps[position_][1]);
    gpio_put(pin3_, fullSteps[position_][2]);
    gpio_put(pin4_, fullSteps[position_][3]);

    sleep_ms(2);
}


inline void
StepperMotor::stepBackward()
{
    position_ = (position_ + 7) % 8;
    gpio_put(pin1_, fullSteps[position_][0]);
    gpio_put(pin2_, fullSteps[position_][1]);
    gpio_put(pin3_, fullSteps[position_][2]);
    gpio_put(pin4_, fullSteps[position_][3]);

    sleep_ms(2);
}
