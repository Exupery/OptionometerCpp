#include "SettingsDialog.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(
    const AppSettings& settings, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Settings");
    setMinimumWidth(400);
    setupUi(settings);
}

void SettingsDialog::setupUi(const AppSettings& settings) {
    auto* mainLayout = new QVBoxLayout(this);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    auto* content = new QWidget();
    auto* layout = new QVBoxLayout(content);

    // API section
    auto* apiGroup = new QGroupBox("API", content);
    auto* apiForm = new QFormLayout(apiGroup);
    m_apiToken = new QLineEdit(settings.apiToken, this);
    m_apiToken->setEchoMode(QLineEdit::Password);
    apiForm->addRow("API Token:", m_apiToken);
    layout->addWidget(apiGroup);

    // Trading section
    auto* tradeGroup = new QGroupBox("Trading", content);
    auto* tradeForm = new QFormLayout(tradeGroup);

    m_maxStrikes = new QSpinBox(this);
    m_maxStrikes->setRange(1, 100);
    m_maxStrikes->setValue(settings.maxStrikes);
    tradeForm->addRow("Max Strikes:", m_maxStrikes);

    m_commission = new QDoubleSpinBox(this);
    m_commission->setRange(0.0, 10.0);
    m_commission->setDecimals(2);
    m_commission->setSingleStep(0.05);
    m_commission->setValue(settings.commission);
    tradeForm->addRow("Commission:", m_commission);

    m_minAnnualReturn = new QDoubleSpinBox(this);
    m_minAnnualReturn->setRange(0.0, 100.0);
    m_minAnnualReturn->setDecimals(1);
    m_minAnnualReturn->setValue(settings.minAnnualReturn);
    tradeForm->addRow("Min Annual Return:", m_minAnnualReturn);

    m_minProfitAmount = new QDoubleSpinBox(this);
    m_minProfitAmount->setRange(0.0, 100.0);
    m_minProfitAmount->setDecimals(2);
    m_minProfitAmount->setValue(settings.minProfitAmount);
    tradeForm->addRow("Min Profit Amount:", m_minProfitAmount);

    layout->addWidget(tradeGroup);

    // Weights section
    auto* weightGroup = new QGroupBox("Score Weights", content);
    auto* weightForm = new QFormLayout(weightGroup);

    auto makeWeightSpin = [&](int val) {
        auto* spin = new QSpinBox(this);
        spin->setRange(0, 100);
        spin->setValue(val);
        return spin;
    };

    m_pricePointWeight = makeWeightSpin(
        settings.pricePointWeight);
    weightForm->addRow("Price Point:", m_pricePointWeight);

    m_profitPointWeight = makeWeightSpin(
        settings.profitPointWeight);
    weightForm->addRow("Profit Point:", m_profitPointWeight);

    m_probabilityWeight = makeWeightSpin(
        settings.probabilityWeight);
    weightForm->addRow("Probability:", m_probabilityWeight);

    m_profitLossWeight = makeWeightSpin(
        settings.profitLossWeight);
    weightForm->addRow("Profit/Loss:", m_profitLossWeight);

    m_annualReturnWeight = makeWeightSpin(
        settings.annualReturnWeight);
    weightForm->addRow("Annual Return:", m_annualReturnWeight);

    m_deltaWeight = makeWeightSpin(settings.deltaWeight);
    weightForm->addRow("Delta:", m_deltaWeight);

    m_hundredTradeWeight = makeWeightSpin(
        settings.hundredTradeWeight);
    weightForm->addRow("100 Trades:", m_hundredTradeWeight);

    layout->addWidget(weightGroup);

    // Bull Put section
    auto* bpGroup = new QGroupBox("Bull Put Spreads", content);
    auto* bpForm = new QFormLayout(bpGroup);

    m_maxMargin = new QSpinBox(this);
    m_maxMargin->setRange(100, 1000000);
    m_maxMargin->setSingleStep(1000);
    m_maxMargin->setValue(settings.maxMargin);
    bpForm->addRow("Max Margin:", m_maxMargin);

    m_minBullPutStrikeBelow = new QSpinBox(this);
    m_minBullPutStrikeBelow->setRange(0, 50);
    m_minBullPutStrikeBelow->setSuffix("%");
    m_minBullPutStrikeBelow->setValue(
        settings.minBullPutStrikeBelow);
    bpForm->addRow("Min Strike Below:", m_minBullPutStrikeBelow);

    m_targetSellStrike = new QSpinBox(this);
    m_targetSellStrike->setRange(0, 100000);
    m_targetSellStrike->setSpecialValueText("Auto");
    m_targetSellStrike->setValue(settings.targetSellStrike);
    bpForm->addRow("Target Sell Strike:", m_targetSellStrike);

    layout->addWidget(bpGroup);

    // Naked Puts section
    auto* npGroup = new QGroupBox("Naked Puts", content);
    auto* npForm = new QFormLayout(npGroup);

    m_maxNakedPutMargin = new QSpinBox(this);
    m_maxNakedPutMargin->setRange(100, 1000000);
    m_maxNakedPutMargin->setSingleStep(1000);
    m_maxNakedPutMargin->setValue(settings.maxNakedPutMargin);
    npForm->addRow("Max Margin:", m_maxNakedPutMargin);

    m_minNakedPutStrikeBelow = new QSpinBox(this);
    m_minNakedPutStrikeBelow->setRange(0, 50);
    m_minNakedPutStrikeBelow->setSuffix("%");
    m_minNakedPutStrikeBelow->setValue(
        settings.minNakedPutStrikeBelow);
    npForm->addRow("Min Strike Below:", m_minNakedPutStrikeBelow);

    m_nakedPutMaxLossProbability = new QSpinBox(this);
    m_nakedPutMaxLossProbability->setRange(1, 100);
    m_nakedPutMaxLossProbability->setSuffix("%");
    m_nakedPutMaxLossProbability->setValue(
        settings.nakedPutMaxLossProbability);
    npForm->addRow("Max Loss Probability:",
        m_nakedPutMaxLossProbability);

    layout->addWidget(npGroup);
    layout->addStretch();

    scroll->setWidget(content);
    mainLayout->addWidget(scroll);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted,
        this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected,
        this, &QDialog::reject);
    mainLayout->addWidget(buttons);
}

AppSettings SettingsDialog::getSettings() const {
    AppSettings s;
    s.apiToken = m_apiToken->text();
    s.maxStrikes = m_maxStrikes->value();
    s.commission = m_commission->value();
    s.minAnnualReturn = m_minAnnualReturn->value();
    s.minProfitAmount = m_minProfitAmount->value();
    s.pricePointWeight = m_pricePointWeight->value();
    s.profitPointWeight = m_profitPointWeight->value();
    s.probabilityWeight = m_probabilityWeight->value();
    s.profitLossWeight = m_profitLossWeight->value();
    s.annualReturnWeight = m_annualReturnWeight->value();
    s.deltaWeight = m_deltaWeight->value();
    s.hundredTradeWeight = m_hundredTradeWeight->value();
    s.maxMargin = m_maxMargin->value();
    s.minBullPutStrikeBelow = m_minBullPutStrikeBelow->value();
    s.targetSellStrike = m_targetSellStrike->value();
    s.maxNakedPutMargin = m_maxNakedPutMargin->value();
    s.minNakedPutStrikeBelow = m_minNakedPutStrikeBelow->value();
    s.nakedPutMaxLossProbability =
        m_nakedPutMaxLossProbability->value();
    return s;
}
