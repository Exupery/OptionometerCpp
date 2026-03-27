#pragma once

#include "models/Enums.h"
#include "services/ScreenerService.h"
#include "services/SettingsManager.h"
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QSet>
#include <QDate>

class DteCalendar;

class ScreenerPanel : public QWidget {
    Q_OBJECT
public:
    explicit ScreenerPanel(QWidget* parent = nullptr);

    void restoreFromSettings(const AppSettings& settings);
    void saveToSettings(AppSettings& settings) const;
    void setScreening(bool screening);
    void setRateLimitRemaining(int remaining);
    void setClosedDates(const QSet<QDate>& dates);
    ScreenerParams currentParams(
        const AppSettings& settings) const;
    int minDays() const;
    int maxDays() const;

signals:
    void screenRequested(ScreenerParams params);
    void dteRangeExtended(int minDays, int maxDays);
    void showDteHalfChanged(bool checked);

private:
    void setupUi();
    void onModeChanged(int index);
    void updateCalendars();

    QLineEdit* m_symbolInput;
    QComboBox* m_modeCombo;
    QComboBox* m_tradeTypeCombo;
    QSpinBox* m_minProbability;
    QSpinBox* m_minDays;
    QSpinBox* m_maxDays;
    QCheckBox* m_fridaysOnly;
    QCheckBox* m_showDteHalf;
    QPushButton* m_screenButton;
    QLabel* m_rateLimitLabel;
    DteCalendar* m_calendarCurrent;
    DteCalendar* m_calendarNext;
    int m_prevMinDays = 0;
    int m_prevMaxDays = 0;
};
