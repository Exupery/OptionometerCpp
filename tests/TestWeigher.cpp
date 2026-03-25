#include <gtest/gtest.h>
#include "core/Weigher.h"

TEST(WeigherTest, DefaultWeightsEqualSum) {
    Weigher w;
    double score = w.scoreByWeight(10, 20, 30, 40, 50, 60, 70);
    EXPECT_NEAR(score, 280.0, 0.01);
}

TEST(WeigherTest, CustomWeights) {
    Weigher w(2, 1, 1, 1, 1, 1, 1);
    double score = w.scoreByWeight(10, 20, 30, 40, 50, 60, 70);
    // 10*2 + 20 + 30 + 40 + 50 + 60 + 70 = 290
    EXPECT_NEAR(score, 290.0, 0.01);
}

TEST(WeigherTest, ZeroWeightsZeroContribution) {
    Weigher w(0, 0, 0, 0, 0, 0, 0);
    double score = w.scoreByWeight(10, 20, 30, 40, 50, 60, 70);
    EXPECT_NEAR(score, 0.0, 0.01);
}

TEST(WeigherTest, BullPutWeights) {
    Weigher w;
    double score = w.scoreByWeightBullPut(10, 20, 30, 40);
    EXPECT_NEAR(score, 100.0, 0.01);
}

TEST(WeigherTest, HundredTradeWeightAccessor) {
    Weigher w(1, 1, 1, 1, 1, 1, 5);
    EXPECT_EQ(w.hundredTradeWeight(), 5);
}
