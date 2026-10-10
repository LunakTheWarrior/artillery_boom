#include "notes.h"

#include <gtest/gtest.h>

TEST(MelodyTest, HasAll309SuppliedRows) {
    EXPECT_EQ(midi1_size, 309U);
}

TEST(MelodyTest, PreservesSuppliedFrequenciesDurationsAndGaps) {
    EXPECT_EQ(midi1[0].frequency_hz, 1047);
    EXPECT_EQ(midi1[0].duration_ms, 100);
    EXPECT_EQ(midi1[0].gap_ms, 0);
    EXPECT_EQ(midi1[6].frequency_hz, 494);
    EXPECT_EQ(midi1[6].duration_ms, 600);
    EXPECT_EQ(midi1[6].gap_ms, 200);
    EXPECT_EQ(midi1[60].frequency_hz, 932);
    EXPECT_EQ(midi1[60].duration_ms, 2350);
    EXPECT_EQ(midi1[60].gap_ms, 0);
    EXPECT_EQ(midi1[midi1_size - 1].frequency_hz, 370);
    EXPECT_EQ(midi1[midi1_size - 1].duration_ms, 100);
    EXPECT_EQ(midi1[midi1_size - 1].gap_ms, 0);
}
