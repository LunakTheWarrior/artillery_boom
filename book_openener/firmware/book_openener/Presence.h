#pragma once
#include <stdint.h>

// Two thresholds and uninterrupted dwell periods. Unknown readings never mean "away".
class Presence {
    bool open_ = false;
    bool pending_ = false;
    bool candidate_ = false;
    uint32_t since_ = 0;
public:
    bool wantsOpen() const { return open_; }
    void reset(bool open = false) { open_ = open; pending_ = false; }
    void sample(uint32_t now, bool valid, uint16_t mm) {
        if (!valid) { pending_ = false; return; }
        const bool request = open_ ? mm <= 450 : mm < 300;
        if (request == open_) { pending_ = false; return; }
        if (!pending_ || candidate_ != request) {
            pending_ = true; candidate_ = request; since_ = now;
        }
        const uint32_t dwell = request ? 500 : 2000;
        if (uint32_t(now - since_) >= dwell) { open_ = request; pending_ = false; }
    }
};
