#include "potentiometer.h"

#include <gtest/gtest.h>

TEST(PotentiometerTriggerTest, StartsAboveUpperThresholdAndStopsBelowLower) {
    PotentiometerTrigger trigger;

    EXPECT_FALSE(trigger.update(0));
    EXPECT_FALSE(trigger.update(PotentiometerTrigger::startThreshold - 1));
    EXPECT_TRUE(trigger.update(PotentiometerTrigger::startThreshold));
    EXPECT_TRUE(trigger.active());

    EXPECT_TRUE(trigger.update(PotentiometerTrigger::stopThreshold + 1));
    EXPECT_FALSE(trigger.update(PotentiometerTrigger::stopThreshold));
    EXPECT_FALSE(trigger.active());
}

TEST(PotentiometerTriggerTest, HysteresisPreventsChatterNearThresholds) {
    PotentiometerTrigger trigger;

    EXPECT_FALSE(trigger.update(PotentiometerTrigger::startThreshold - 1));
    EXPECT_FALSE(trigger.update(PotentiometerTrigger::stopThreshold + 1));
    EXPECT_TRUE(trigger.update(PotentiometerTrigger::startThreshold));
    EXPECT_TRUE(trigger.update(PotentiometerTrigger::startThreshold - 1));
    EXPECT_TRUE(trigger.update(PotentiometerTrigger::stopThreshold + 1));
    EXPECT_FALSE(trigger.update(PotentiometerTrigger::stopThreshold));
}

TEST(PotentiometerTriggerTest, CanRestartAfterTurningBackUp) {
    PotentiometerTrigger trigger;
    EXPECT_TRUE(trigger.update(PotentiometerTrigger::startThreshold));
    EXPECT_FALSE(trigger.update(0));
    EXPECT_TRUE(trigger.update(4095));
}
