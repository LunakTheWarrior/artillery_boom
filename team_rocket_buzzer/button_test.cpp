#include "button.h"
#include <gtest/gtest.h>

TEST(ButtonTest, DebouncesPressAndRequiresStableRelease) {
    Button b;
    EXPECT_TRUE(!b.update(true, 0));
    EXPECT_TRUE(!b.update(true, 100)); // Held at boot must not play.
    EXPECT_TRUE(!b.update(false, 110));
    EXPECT_TRUE(!b.update(false, 140));
    EXPECT_TRUE(!b.update(true, 150));
    EXPECT_TRUE(!b.update(false, 155)); // Contact bounce.
    EXPECT_TRUE(!b.update(true, 160));
    EXPECT_TRUE(!b.update(true, 189));
    EXPECT_TRUE(b.update(true, 190));
    EXPECT_TRUE(!b.update(true, 1000)); // No repeat while held.
    EXPECT_TRUE(!b.update(false, 1010));
    EXPECT_TRUE(!b.update(true, 1020)); // Short release must not re-arm.
    EXPECT_TRUE(!b.update(true, 1060));
    EXPECT_TRUE(!b.update(false, 1070));
    EXPECT_TRUE(!b.update(false, 1100));
    EXPECT_TRUE(!b.update(true, 1110));
    EXPECT_TRUE(b.update(true, 1140));
    b = Button{}; // Same reset as playback completion.
    EXPECT_TRUE(!b.update(true, 2000));
    EXPECT_TRUE(!b.update(true, 2100));
    EXPECT_TRUE(!b.update(false, 2200));
    EXPECT_TRUE(!b.update(false, 2230));
    EXPECT_TRUE(!b.update(true, 2240));
    EXPECT_TRUE(b.update(true, 2270));
}
