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

// Lognormal tests
// For S=100, σ=0.25, T=1.0 (sigmaRootT=0.25), r=0.05:
// ln(S_T) ~ N(ln(100) + (0.05 - 0.03125)*1, 0.25^2)
// = N(4.6243, 0.0625)
// median = exp(4.6243) = 101.89

TEST(MathUtilsTest, LognormalCdfAtCurrentPrice) {
    // P(S_T < S) should be slightly below 0.5 due to positive drift
    double cdf = MathUtils::lognormalCdf(100.0, 100.0, 0.25, 1.0, 0.05);
    EXPECT_LT(cdf, 0.5);
    EXPECT_GT(cdf, 0.4);
}

TEST(MathUtilsTest, LognormalCdfZeroPrice) {
    EXPECT_NEAR(MathUtils::lognormalCdf(0.0, 100.0, 0.25, 1.0), 0.0, 1e-10);
}

TEST(MathUtilsTest, LognormalCdfVeryHighPrice) {
    double cdf = MathUtils::lognormalCdf(1000.0, 100.0, 0.25, 1.0);
    EXPECT_NEAR(cdf, 1.0, 1e-6);
}

TEST(MathUtilsTest, LognormalProbabilityWideRange) {
    // Nearly all probability should be captured in a wide range
    double prob = MathUtils::lognormalProbability(100.0, 0.25, 1.0, 10.0, 500.0);
    EXPECT_NEAR(prob, 1.0, 0.001);
}

TEST(MathUtilsTest, LognormalProbabilityNarrowRange) {
    double prob = MathUtils::lognormalProbability(100.0, 0.25, 1.0, 99.0, 101.0);
    EXPECT_GT(prob, 0.0);
    EXPECT_LT(prob, 0.1);
}

TEST(MathUtilsTest, LognormalCdfShortDated) {
    // σ=0.25, T=30/365, sigmaRootT = 0.25*sqrt(30/365) ≈ 0.07163
    double sigmaRootT = 0.25 * std::sqrt(30.0 / 365.0);
    double T = 30.0 / 365.0;
    // At the current price, CDF should be near 0.5
    double cdf = MathUtils::lognormalCdf(100.0, 100.0, sigmaRootT, T, 0.05);
    EXPECT_NEAR(cdf, 0.5, 0.02);
}

// Inverse normal CDF tests

TEST(MathUtilsTest, NormalInverseCdfAtHalf) {
    EXPECT_NEAR(MathUtils::normalInverseCdf(0.5), 0.0, 1e-8);
}

TEST(MathUtilsTest, NormalInverseCdfAt84Pct) {
    // normalCdf(1.0) ≈ 0.8413
    EXPECT_NEAR(MathUtils::normalInverseCdf(0.8413), 1.0, 0.001);
}

TEST(MathUtilsTest, NormalInverseCdfAt16Pct) {
    // normalCdf(-1.0) ≈ 0.1587
    EXPECT_NEAR(MathUtils::normalInverseCdf(0.1587), -1.0, 0.001);
}

TEST(MathUtilsTest, NormalInverseCdfAt5Pct) {
    // normalCdf(-1.6449) ≈ 0.05
    EXPECT_NEAR(MathUtils::normalInverseCdf(0.05), -1.6449, 0.001);
}

TEST(MathUtilsTest, NormalInverseCdfRoundTrip) {
    for (double x : {-2.5, -1.0, 0.0, 0.5, 1.5, 2.0}) {
        double p = MathUtils::normalCdf(x);
        EXPECT_NEAR(MathUtils::normalInverseCdf(p), x, 1e-6);
    }
}

// Inverse lognormal CDF tests

TEST(MathUtilsTest, LognormalInverseCdfRoundTrip) {
    double S = 100.0, sigmaRootT = 0.25, T = 1.0, r = 0.05;
    for (double p : {0.05, 0.10, 0.25, 0.50, 0.75, 0.95}) {
        double price = MathUtils::lognormalInverseCdf(
            p, S, sigmaRootT, T, r);
        double recoveredP = MathUtils::lognormalCdf(
            price, S, sigmaRootT, T, r);
        EXPECT_NEAR(recoveredP, p, 1e-6);
    }
}

TEST(MathUtilsTest, LognormalInverseCdfMedian) {
    // Median of lognormal with positive drift should be above spot
    double S = 100.0, sigmaRootT = 0.25, T = 1.0, r = 0.05;
    double median = MathUtils::lognormalInverseCdf(
        0.5, S, sigmaRootT, T, r);
    EXPECT_GT(median, S);
}

TEST(MathUtilsTest, LognormalInverseCdfShortDated) {
    double S = 500.0;
    double sigmaRootT = 0.25 * std::sqrt(30.0 / 365.0);
    double T = 30.0 / 365.0;
    // 5th percentile should be well below current price
    double p5 = MathUtils::lognormalInverseCdf(0.05, S, sigmaRootT, T);
    EXPECT_LT(p5, S);
    EXPECT_GT(p5, 0.0);
    // Verify round-trip
    EXPECT_NEAR(MathUtils::lognormalCdf(p5, S, sigmaRootT, T), 0.05, 1e-6);
}

TEST(MathUtilsTest, LognormalInverseCdfBoundary) {
    EXPECT_NEAR(MathUtils::lognormalInverseCdf(0.0, 100.0, 0.25, 1.0), 0.0, 1e-10);
}

TEST(MathUtilsTest, LognormalProbabilityAsymmetry) {
    // Lognormal should have right skew: P(S > S0) slightly > P(S < S0)
    // when drift is positive
    double T = 1.0;
    double sigmaRootT = 0.25;
    double below = MathUtils::lognormalCdf(100.0, 100.0, sigmaRootT, T, 0.05);
    double above = 1.0 - below;
    EXPECT_GT(above, below);
}
