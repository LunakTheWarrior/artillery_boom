#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>

// Positions are half-steps from the existing mechanism's lower travel limit.
// The mechanism must be physically centered before power-on (no homing switch).
class DistanceTracker {
public:
    static constexpr int travel = 11343;
    static constexpr int center = 5672;
    static constexpr int stride = 64;
    static constexpr int radius = 256;
    static constexpr int maxDistanceMm = 1000;
    static constexpr int distanceToleranceMm = 40;

    int position() const { return position_; }
    bool tracking() const { return mode_ != Mode::Search; }

    // Called only with a fresh sample acquired while stationary. Missing/invalid
    // ranges are empty space, never a zero-distance target. Returns signed steps.
    int observe(std::optional<std::uint16_t> distance) {
        const bool visible = distance && *distance <= maxDistanceMm;
        const int previous = position_;
        if (mode_ == Mode::Search) {
            if (visible) {
                beginScan(position_);
            } else {
                if (position_ == travel) direction_ = -1;
                if (position_ == 0) direction_ = 1;
                position_ = std::clamp(position_ + direction_ * stride, 0, travel);
            }
        } else if (mode_ == Mode::Return) {
            if (position_ != target_) {
                moveToward(target_);
            } else if (visible) {
                beginScan(position_);
            } else {
                mode_ = Mode::Search;
            }
        } else if (mode_ == Mode::MoveToStart) {
            if (position_ != lower_) {
                moveToward(lower_);
            } else {
                mode_ = Mode::Scan;
                record(distance);
                advanceScan();
            }
        } else {
            record(distance);
            advanceScan();
        }
        return position_ - previous;
    }

    // Keep physical position across sensor restarts, but discard stale targets.
    void reset() { mode_ = Mode::Search; }

private:
    enum class Mode { Search, MoveToStart, Scan, Return };
    void moveToward(int destination) {
        position_ += std::clamp(destination - position_, -stride, stride);
    }
    void beginScan(int around) {
        lower_ = std::max(0, around - radius);
        upper_ = std::min(travel, around + radius);
        best_ = maxDistanceMm + 1;
        sum_ = count_ = 0;
        mode_ = Mode::MoveToStart;
        moveToward(lower_);
    }
    void record(std::optional<std::uint16_t> distance) {
        if (!distance || *distance > maxDistanceMm) return;
        // Average the angular positions of similarly close returns to avoid
        // chasing millimeter noise on a broad object's surface.
        if (*distance + distanceToleranceMm < best_) {
            best_ = *distance;
            sum_ = count_ = 0;
        }
        if (*distance <= best_ + distanceToleranceMm) {
            sum_ += position_;
            ++count_;
        }
    }
    void advanceScan() {
        if (position_ < upper_) {
            moveToward(upper_);
        } else if (count_) {
            target_ = sum_ / count_;
            mode_ = Mode::Return;
            moveToward(target_);
        } else {
            mode_ = Mode::Search;
        }
    }
    int position_{center};
    int direction_{1};
    Mode mode_{Mode::Search};
    int lower_{}, upper_{}, target_{}, best_{}, sum_{}, count_{};
};
