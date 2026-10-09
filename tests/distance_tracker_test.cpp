#include "src/distance_tracker.h"

#include <cstdlib>
#include <stdexcept>

void check(bool condition) {
    if (!condition) throw std::runtime_error("Distance tracking check failed");
}

int main() {
    DistanceTracker tracker;
    bool low = false, high = false;
    for (int i = 0; i < 1000; ++i) {
        int before = tracker.position();
        int steps = tracker.observe(std::nullopt);
        check(tracker.position() == before + steps);
        check(std::abs(steps) <= DistanceTracker::stride);
        check(tracker.position() >= 0 && tracker.position() <= DistanceTracker::travel);
        low |= tracker.position() == 0;
        high |= tracker.position() == DistanceTracker::travel;
        check(!tracker.tracking());
    }
    check(low && high);

    // A near object embedded in a farther background, then moving in either
    // direction. Sample only at the commanded physical position.
    tracker = DistanceTracker{};
    int object = DistanceTracker::center + 128;
    for (int direction : {1, -1}) {
        for (int round = 0; round < 8; ++round) {
            int closest = DistanceTracker::travel;
            for (int sample = 0; sample < 35; ++sample) {
                int offset = std::abs(tracker.position() - object);
                tracker.observe(offset <= 96 ? 400 : 900);
                closest = std::min(closest, std::abs(tracker.position() - object));
            }
            check(tracker.tracking());
            check(closest <= DistanceTracker::stride);
            object += direction * DistanceTracker::stride;
        }
    }
    // Losing all returns abandons the local scan and resumes searching.
    for (int i = 0; i < 40; ++i) tracker.observe(std::nullopt);
    check(!tracker.tracking());
    tracker.observe(500);
    check(tracker.tracking());
    int position = tracker.position();
    tracker.reset();
    check(!tracker.tracking() && tracker.position() == position);

    tracker = DistanceTracker{};
    tracker.observe(1001);
    check(!tracker.tracking());
    tracker.observe(0); // A valid zero is not a sensor failure.
    check(tracker.tracking());
    // Tracking scans at both travel limits must remain bounded too.
    for (int end : {0, DistanceTracker::travel}) {
        tracker = DistanceTracker{};
        for (int i = 0; i < 400 && tracker.position() != end; ++i)
            tracker.observe(std::nullopt);
        check(tracker.position() == end);
        for (int i = 0; i < 100; ++i) {
            auto steps = tracker.observe(400);
            check(std::abs(steps) <= DistanceTracker::stride);
            check(tracker.position() >= 0 && tracker.position() <= DistanceTracker::travel);
        }
    }
}
