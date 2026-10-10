#pragma once
#include <cstdint>

// Require a stable release before arming, including at boot and between presses.
class Button {
public:
    bool update(bool pressed, std::uint64_t now_ms) {
        if (!initialized_ || pressed != candidate_) {
            initialized_ = true;
            candidate_ = pressed;
            changed_at_ = now_ms;
        }
        if (now_ms - changed_at_ < 30) return false;
        if (!candidate_) armed_ = true;
        if (candidate_ && armed_) {
            armed_ = false;
            return true;
        }
        return false;
    }
private:
    bool initialized_ = false;
    bool candidate_ = false;
    bool armed_ = false;
    std::uint64_t changed_at_ = 0;
};
