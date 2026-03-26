#include <gtest/gtest.h>
#include "services/SettingsManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

namespace {

class SettingsManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_tempDir.setAutoRemove(true);
    }
    QTemporaryDir m_tempDir;
};

TEST_F(SettingsManagerTest, DefaultSettings) {
    AppSettings s;
    EXPECT_EQ(s.lastTicker, "SPY");
    EXPECT_EQ(s.lastMinProbability, 25);
    EXPECT_EQ(s.lastMinDays, 25);
    EXPECT_EQ(s.lastMaxDays, 45);
    EXPECT_EQ(s.maxStrikes, 20);
    EXPECT_NEAR(s.commission, 0.5, 0.01);
    EXPECT_EQ(s.pricePointWeight, 1);
    EXPECT_EQ(s.maxMargin, 10000);
    EXPECT_EQ(s.minBullPutStrikeBelow, 2);
}

TEST_F(SettingsManagerTest, HiddenColumnsDefault) {
    AppSettings s;
    EXPECT_TRUE(s.hiddenColumns.isEmpty());
}

TEST_F(SettingsManagerTest, ApiTokenDefault) {
    AppSettings s;
    EXPECT_TRUE(s.apiToken.isEmpty());
}

TEST_F(SettingsManagerTest, ChartSizeDefaults) {
    AppSettings s;
    EXPECT_EQ(s.chartWidth, 0);
    EXPECT_EQ(s.chartHeight, 0);
}

} // namespace
