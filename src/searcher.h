#pragma once

#include "distance_tracker.h"
#include "stepper_motor.h"
#include "SongPlayer.h"

class PersonSearcher {
public:
    void observe(std::optional<std::uint16_t> distance) {
        const bool wasTracking = tracker_.tracking();
        const int steps = tracker_.observe(distance);
        if (!wasTracking && tracker_.tracking() && time_reached(nextSound_)) {
            // A tiny robot "woop!"; alternate rising and falling on acquisition.
            // Keep the entire sound below 200 ms so sensor polling resumes soon.
            constexpr std::array<std::uint32_t, 4> notes{600, 1000, 1600, 2400};
            for (std::size_t i = 0; i < notes.size(); ++i) {
                buzzer_.playTone(notes[descending_ ? notes.size() - 1 - i : i], 35);
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
