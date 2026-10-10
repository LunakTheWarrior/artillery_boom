#include "button.h"
#include "melody.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"

namespace {
constexpr unsigned buzzer_pin = 0;
constexpr unsigned button_pin = 15;
constexpr float pwm_divider = 64.0f;

void silence() {
    pwm_set_enabled(pwm_gpio_to_slice_num(buzzer_pin), false);
    gpio_set_function(buzzer_pin, GPIO_FUNC_SIO);
    gpio_put(buzzer_pin, false);
}

// Poll during both notes and gaps so a second press stops promptly.
bool stop_requested(Button& button, std::uint32_t duration_ms) {
    const auto end = time_us_64() / 1000 + duration_ms;
    while (time_us_64() / 1000 < end) {
        if (button.update(!gpio_get(button_pin), time_us_64() / 1000)) return true;
        sleep_ms(1);
    }
    return false;
}

void play_loop(Button& button) {
    const auto slice = pwm_gpio_to_slice_num(buzzer_pin);
    while (true) {
        for (const auto note : melody) {
            if (note.hz != 0) {
                const auto ticks = static_cast<std::uint32_t>(
                    clock_get_hz(clk_sys) / (pwm_divider * note.hz));
                // Keep malformed/out-of-range imported notes silent.
                if (ticks >= 2 && ticks <= 65536) {
                    auto config = pwm_get_default_config();
                    pwm_config_set_clkdiv(&config, pwm_divider);
                    pwm_config_set_wrap(&config, ticks - 1);
                    pwm_init(slice, &config, false);
                    pwm_set_gpio_level(buzzer_pin, ticks / 2);
                    gpio_set_function(buzzer_pin, GPIO_FUNC_PWM);
                    pwm_set_enabled(slice, true);
                }
            }
            // Include the articulation gap within the beat to preserve tempo.
            const auto gap_ms = note.duration_ms < 25 ? note.duration_ms : 25;
            const bool stop = stop_requested(button, note.duration_ms - gap_ms);
            silence();
            if (stop || stop_requested(button, gap_ms)) return;
        }
    }
}
}

int main() {
    gpio_init(buzzer_pin);
    gpio_set_dir(buzzer_pin, GPIO_OUT);
    silence();
    gpio_init(button_pin);
    gpio_set_dir(button_pin, GPIO_IN);
    gpio_pull_up(button_pin);
    Button button;
    while (true) {
        if (button.update(!gpio_get(button_pin), time_us_64() / 1000)) {
            play_loop(button);
        }
        sleep_ms(1);
    }
}
