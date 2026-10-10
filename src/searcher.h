#pragma once

#include "distance_tracker.h"
#include "stepper_motor.h"
#include "SongPlayer.h"

class PersonSearcher {
public:
    void testBuzzer() {
        std::printf("Piezo test on GP0: three startup beeps\n");
        for (const auto frequency : {1000u, 2000u, 3000u}) {
            buzzer_.playTone(frequency, 150);
            sleep_ms(100);
        }
    }

    void observe(std::optional<std::uint16_t> distance) {
        const int steps = tracker_.observe(distance);
        if (distance && *distance <= DistanceTracker::maxDistanceMm && time_reached(nextSound_)) {
            std::printf("Piezo GP0: object detected at %u mm\n", static_cast<unsigned>(*distance));
            // Repeat while fresh valid returns see an object, even if tracking
            // never drops back into search mode. Total duration is 320 ms.
            constexpr std::array<std::uint32_t, 4> notes{600, 1000, 1600, 2400};
            for (std::size_t i = 0; i < notes.size(); ++i) {
                buzzer_.playTone(notes[descending_ ? notes.size() - 1 - i : i], 70);
                sleep_ms(10);
            }
            descending_ = !descending_;
            nextSound_ = make_timeout_time_ms(3000);
        }
        for (int i = 0; i < (steps < 0 ? -steps : steps); ++i) {
            if (steps > 0) motor_.stepForward();
            else motor_.stepBackward();
        }
    }
    void reset() { tracker_.reset(); }

private:
    DistanceTracker tracker_;
    StepperMotor motor_{{21, 20, 19, 18}};
    SongPlayer buzzer_{0};
    absolute_time_t nextSound_{make_timeout_time_ms(0)};
    bool descending_{false};
};
