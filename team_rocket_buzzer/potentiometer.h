#pragma once

#include <cstdint>

class PotentiometerTrigger final {
public:
    static constexpr std::uint16_t startThreshold = 2457; // About 60% of 12-bit ADC range.
    static constexpr std::uint16_t stopThreshold = 1638;  // About 40%.

    // High readings start playback; low readings stop it. The gap is hysteresis.
    constexpr bool update(std::uint16_t reading) {
        if (!active_ && reading >= startThreshold) active_ = true;
        else if (active_ && reading <= stopThreshold) active_ = false;
        return active_;
    }

    constexpr bool active() const { return active_; }

private:
    bool active_ = false;
};
