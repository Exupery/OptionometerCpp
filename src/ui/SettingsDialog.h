#pragma once

#include "services/SettingsManager.h"
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(
        const AppSettings& settings,
        QWidget* parent = nullptr);

    AppSettings getSettings() const;

private:
    void setupUi(const AppSettings& settings);

    QLineEdit* m_apiToken;
    QSpinBox* m_maxStrikes;
    QDoubleSpinBox* m_commission;
    QDoubleSpinBox* m_minAnnualReturn;
    QDoubleSpinBox* m_minProfitAmount;
    QSpinBox* m_pricePointWeight;
    QSpinBox* m_profitPointWeight;
    QSpinBox* m_probabilityWeight;
    QSpinBox* m_profitLossWeight;
    QSpinBox* m_annualReturnWeight;
    QSpinBox* m_deltaWeight;
    QSpinBox* m_hundredTradeWeight;
    QSpinBox* m_maxMargin;
    QSpinBox* m_minBullPutStrikeBelow;
    QSpinBox* m_targetSellStrike;
    QSpinBox* m_maxNakedPutMargin;
    QSpinBox* m_minNakedPutStrikeBelow;
    QSpinBox* m_nakedPutMaxLossProbability;
};
