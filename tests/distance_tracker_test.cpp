#include "src/distance_tracker.h"

#include <gtest/gtest.h>

TEST(DistanceTrackerTest, SearchesAndTracksWithinTravelBounds) {
    DistanceTracker tracker;
    bool low = false, high = false;
    for (int i = 0; i < 1000; ++i) {
        int before = tracker.position();
        int steps = tracker.observe(std::nullopt);
        EXPECT_TRUE(tracker.position() == before + steps);
        EXPECT_TRUE(std::abs(steps) <= DistanceTracker::stride);
        EXPECT_TRUE(tracker.position() >= 0 && tracker.position() <= DistanceTracker::travel);
        low |= tracker.position() == 0;
        high |= tracker.position() == DistanceTracker::travel;
        EXPECT_TRUE(!tracker.tracking());
    }
    EXPECT_TRUE(low && high);

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
            EXPECT_TRUE(tracker.tracking());
            EXPECT_TRUE(closest <= DistanceTracker::stride);
            object += direction * DistanceTracker::stride;
        }
    }
    // Losing all returns abandons the local scan and resumes searching.
    for (int i = 0; i < 40; ++i) tracker.observe(std::nullopt);
    EXPECT_TRUE(!tracker.tracking());
    tracker.observe(500);
    EXPECT_TRUE(tracker.tracking());
    int position = tracker.position();
    tracker.reset();
    EXPECT_TRUE(!tracker.tracking() && tracker.position() == position);

    tracker = DistanceTracker{};
    tracker.observe(1001);
    EXPECT_TRUE(!tracker.tracking());
    tracker.observe(0); // A valid zero is not a sensor failure.
    EXPECT_TRUE(tracker.tracking());
    // Tracking scans at both travel limits must remain bounded too.
    for (int end : {0, DistanceTracker::travel}) {
        tracker = DistanceTracker{};
        for (int i = 0; i < 400 && tracker.position() != end; ++i)
            tracker.observe(std::nullopt);
        EXPECT_TRUE(tracker.position() == end);
        for (int i = 0; i < 100; ++i) {
            auto steps = tracker.observe(400);
            EXPECT_TRUE(std::abs(steps) <= DistanceTracker::stride);
            EXPECT_TRUE(tracker.position() >= 0 && tracker.position() <= DistanceTracker::travel);
        }
    }
}
