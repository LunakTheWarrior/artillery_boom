#include <cstdio>

#include "notes.h"
#include "potentiometer.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/stdio_usb.h"
#include "pico/stdlib.h"

namespace {
constexpr unsigned buzzer_pin = 0;
constexpr unsigned potentiometer_pin = 26; // GP26 / ADC0
constexpr float pwm_divider = 64.0f;

void update_trigger(PotentiometerTrigger& trigger, std::uint16_t reading) {
    static bool announced = false;
    static std::uint64_t last_sample_log_ms = 0;
    const bool was_active = trigger.active();
    const bool is_active = trigger.update(reading);

    if (!stdio_usb_connected()) return;

    if (!announced) {
        std::printf("Team Rocket buzzer ready. Potentiometer: GP26/ADC0; start >= %u, stop <= %u.\r\n",
                    static_cast<unsigned>(PotentiometerTrigger::startThreshold),
                    static_cast<unsigned>(PotentiometerTrigger::stopThreshold));
        announced = true;
    }

    if (was_active != is_active) {
        std::printf("[trigger] ADC=%u: melody %s\r\n",
                    static_cast<unsigned>(reading), is_active ? "START" : "STOP");
    }

    const auto now_ms = time_us_64() / 1000;
    if (now_ms - last_sample_log_ms >= 1000) {
        std::printf("[pot] ADC=%u/4095, state=%s\r\n",
                    static_cast<unsigned>(reading), is_active ? "playing" : "idle");
        last_sample_log_ms = now_ms;
    }
}

void silence() {
    pwm_set_enabled(pwm_gpio_to_slice_num(buzzer_pin), false);
    gpio_set_function(buzzer_pin, GPIO_FUNC_SIO);
    gpio_put(buzzer_pin, false);
}

// Poll during both notes and gaps so a second press stops promptly.
bool stop_requested(PotentiometerTrigger& trigger, std::uint32_t duration_ms) {
    const auto end = time_us_64() / 1000 + duration_ms;
    while (time_us_64() / 1000 < end) {
        update_trigger(trigger, adc_read());
        if (!trigger.active()) return true;
        sleep_ms(1);
    }
    return false;
}

void play_loop(PotentiometerTrigger& trigger) {
    const auto slice = pwm_gpio_to_slice_num(buzzer_pin);
    if (stdio_usb_connected()) std::printf("[song] looping melody (%u notes)\r\n", static_cast<unsigned>(midi1_size));
    while (true) {
        for (const auto& note : midi1) {
            if (note.frequency_hz > 0) {
                const auto ticks = static_cast<std::uint32_t>(
                    clock_get_hz(clk_sys) / (pwm_divider * note.frequency_hz));
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
            const bool stop = stop_requested(trigger, static_cast<std::uint32_t>(note.duration_ms));
            silence();
            if (stop || stop_requested(trigger, static_cast<std::uint32_t>(note.gap_ms))) {
                if (stdio_usb_connected()) std::printf("[song] stopped; turn the potentiometer up to restart\r\n");
                return;
            }
        }
    }
}
}

int main() {
    stdio_init_all();
    gpio_init(buzzer_pin);
    gpio_set_dir(buzzer_pin, GPIO_OUT);
    silence();
    adc_init();
    adc_gpio_init(potentiometer_pin);
    adc_select_input(0);
    PotentiometerTrigger trigger;
    while (true) {
        update_trigger(trigger, adc_read());
        if (trigger.active()) play_loop(trigger);
        sleep_ms(1);
    }
}
