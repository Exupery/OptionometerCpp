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

class DteCalendar;

class ScreenerPanel : public QWidget {
    Q_OBJECT
public:
    explicit ScreenerPanel(QWidget* parent = nullptr);

    void restoreFromSettings(const AppSettings& settings);
    void saveToSettings(AppSettings& settings) const;
    void setScreening(bool screening);
    void setRateLimitRemaining(int remaining);
    ScreenerParams currentParams(
        const AppSettings& settings) const;

signals:
    void screenRequested(ScreenerParams params);

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
    QPushButton* m_screenButton;
    QLabel* m_rateLimitLabel;
    DteCalendar* m_calendarCurrent;
    DteCalendar* m_calendarNext;
};
