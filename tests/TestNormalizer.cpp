#include <gtest/gtest.h>
#include "core/Normalizer.h"

TEST(NormalizerTest, NormalizeEmptyReturnsEmpty) {
    auto result = Normalizer::normalizeScores({});
    EXPECT_TRUE(result.empty());
}

TEST(NormalizerTest, NormalizeSingleReturns100) {
    std::vector<std::pair<int, double>> scores = {{0, 42.0}};
    auto result = Normalizer::normalizeScores(scores);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_NEAR(result[0], 100.0, 0.01);
}

TEST(NormalizerTest, NormalizeTwoElements) {
    std::vector<std::pair<int, double>> scores = {
        {0, 10.0}, {1, 20.0}
    };
    auto result = Normalizer::normalizeScores(scores);
    // Lower gets 0, higher gets 100
    EXPECT_NEAR(result[0], 0.0, 0.01);
    EXPECT_NEAR(result[1], 100.0, 0.01);
}

TEST(NormalizerTest, NormalizeOrderedCorrectly) {
    std::vector<std::pair<int, double>> scores = {
        {0, 30.0}, {1, 10.0}, {2, 20.0}
    };
    auto result = Normalizer::normalizeScores(scores);
    EXPECT_LT(result[1], result[2]);
    EXPECT_LT(result[2], result[0]);
}

TEST(NormalizerTest, NegativeScoresGetZero) {
    std::vector<std::pair<int, double>> scores = {
        {0, -5.0}, {1, 10.0}, {2, 20.0}
    };
    auto result = Normalizer::normalizeScores(scores, true);
    EXPECT_NEAR(result[0], 0.0, 0.01);
}

TEST(NormalizerTest, NegativeScoresKeptWhenNotIgnored) {
    std::vector<std::pair<int, double>> scores = {
        {0, -5.0}, {1, 10.0}, {2, 20.0}
    };
    auto result = Normalizer::normalizeScores(scores, false);
    EXPECT_GT(result[0], 0.0);
}
