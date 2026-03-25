#include <gtest/gtest.h>
#include "core/MathUtils.h"
#include <cmath>

TEST(MathUtilsTest, NormalCdfAtZero) {
    EXPECT_NEAR(MathUtils::normalCdf(0.0), 0.5, 1e-10);
}

TEST(MathUtilsTest, NormalCdfAtPositiveInfinity) {
    EXPECT_NEAR(MathUtils::normalCdf(6.0), 1.0, 1e-6);
}

TEST(MathUtilsTest, NormalCdfAtNegativeInfinity) {
    EXPECT_NEAR(MathUtils::normalCdf(-6.0), 0.0, 1e-6);
}

TEST(MathUtilsTest, NormalCdfAtOne) {
    EXPECT_NEAR(MathUtils::normalCdf(1.0), 0.8413, 0.001);
}

TEST(MathUtilsTest, NormalCdfAtMinusOne) {
    EXPECT_NEAR(MathUtils::normalCdf(-1.0), 0.1587, 0.001);
}

TEST(MathUtilsTest, NormalCdfAtTwo) {
    EXPECT_NEAR(MathUtils::normalCdf(2.0), 0.9772, 0.001);
}

TEST(MathUtilsTest, NormalCdfSymmetry) {
    double pos = MathUtils::normalCdf(1.5);
    double neg = MathUtils::normalCdf(-1.5);
    EXPECT_NEAR(pos + neg, 1.0, 1e-10);
}

TEST(MathUtilsTest, NormalProbabilityOneSd) {
    double prob = MathUtils::normalProbability(100, 10, 90, 110);
    EXPECT_NEAR(prob, 0.6827, 0.001);
}

TEST(MathUtilsTest, NormalProbabilityTwoSd) {
    double prob = MathUtils::normalProbability(100, 10, 80, 120);
    EXPECT_NEAR(prob, 0.9545, 0.001);
}

TEST(MathUtilsTest, NormalProbabilityNarrowRange) {
    double prob = MathUtils::normalProbability(100, 10, 99, 101);
    EXPECT_GT(prob, 0.0);
    EXPECT_LT(prob, 0.1);
}
