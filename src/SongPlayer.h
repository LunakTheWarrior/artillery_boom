#pragma once

#include <array>
#include <cstdio>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <cstdint>

class SongPlayer
{
    public:
        SongPlayer(std::uint32_t buzzerPin);

        void playTone(std::uint32_t frequency, std::uint32_t duration_ms);


        template <std::size_t SongSize>
        void playSong(const std::array<int, SongSize>& melody,
        const std::array<int, SongSize>& durations);

    private:
        std::uint32_t buzzerPin_;
};

inline SongPlayer::SongPlayer(const std::uint32_t buzzerPin)
    : buzzerPin_(buzzerPin)
{
    gpio_init(buzzerPin_);
    gpio_set_dir(buzzerPin_, GPIO_OUT);
    gpio_put(buzzerPin, 0);
}

inline void
SongPlayer::playTone(std::uint32_t frequency, std::uint32_t duration_ms)
{
   if (frequency == 0) {
        sleep_ms(duration_ms);
        return;
    }

    uint32_t half_period_us = 500000 / frequency;

    uint32_t start = time_us_32();

    while ((time_us_32() - start) < duration_ms * 1000) {
        gpio_put(buzzerPin_, 1);
        sleep_us(half_period_us);

        gpio_put(buzzerPin_, 0);
        sleep_us(half_period_us);
    }

    gpio_put(buzzerPin_, 0);
}

template <std::size_t SongSize>
void
SongPlayer::playSong(const std::array<int, SongSize>& melody,
const std::array<int, SongSize>& durations)
{
    int index = 0;
    for (const int note : melody)
    {
        // Original code:
        // 1000 / duration
        uint noteDuration = 1000 / durations[index];

        playTone(note, noteDuration);

        // Original uses 30% separation
        uint pause = noteDuration * 1.30f;
        sleep_ms(pause - noteDuration);
        ++index;
    }
    printf("Done playing da music\n");
    gpio_put(buzzerPin_, 0);
}
