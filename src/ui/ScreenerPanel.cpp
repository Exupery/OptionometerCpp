#include "ScreenerPanel.h"
#include "DteCalendar.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDate>

ScreenerPanel::ScreenerPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void ScreenerPanel::setupUi() {
    auto* layout = new QVBoxLayout(this);

    auto* form = new QFormLayout();
    form->setFieldGrowthPolicy(
        QFormLayout::AllNonFixedFieldsGrow);

    m_symbolInput = new QLineEdit("SPY", this);
    form->addRow("Symbol:", m_symbolInput);

    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem(
        screenerModeToString(ScreenerMode::StrategyOptimizer));
    m_modeCombo->addItem(
        screenerModeToString(ScreenerMode::BullPutSpreadScreener));
    m_modeCombo->addItem(
        screenerModeToString(ScreenerMode::BullPutSpreadOptimizer));
    form->addRow("Mode:", m_modeCombo);

    m_tradeTypeCombo = new QComboBox(this);
    m_tradeTypeCombo->addItem(
        tradeTypeToString(TradeType::BullPutSpreads));
    m_tradeTypeCombo->addItem(
        tradeTypeToString(TradeType::Condors));
    m_tradeTypeCombo->addItem(
        tradeTypeToString(TradeType::TwoLeg));
    m_tradeTypeCombo->addItem(
        tradeTypeToString(TradeType::ThreeLeg));
    m_tradeTypeCombo->addItem(
        tradeTypeToString(TradeType::FourLeg));
    form->addRow("Trade Type:", m_tradeTypeCombo);

    m_minProbability = new QSpinBox(this);
    m_minProbability->setRange(0, 100);
    m_minProbability->setValue(25);
    m_minProbability->setSuffix("%");
    form->addRow("Min Probability:", m_minProbability);

    m_minDays = new QSpinBox(this);
    m_minDays->setRange(1, 365);
    m_minDays->setValue(25);
    form->addRow("Min Days:", m_minDays);

    m_maxDays = new QSpinBox(this);
    m_maxDays->setRange(1, 365);
    m_maxDays->setValue(45);
    form->addRow("Max Days:", m_maxDays);

    m_fridaysOnly = new QCheckBox("Fridays Only", this);
    form->addRow("", m_fridaysOnly);

    layout->addLayout(form);

    // DTE range calendars
    QDate today = QDate::currentDate();

    m_calendarCurrent = new DteCalendar(this);
    m_calendarCurrent->setCurrentPage(
        today.year(), today.month());
    m_calendarCurrent->setMaximumHeight(180);
    layout->addWidget(m_calendarCurrent);

    QDate nextMonth = today.addMonths(1);
    m_calendarNext = new DteCalendar(this);
    m_calendarNext->setCurrentPage(
        nextMonth.year(), nextMonth.month());
    m_calendarNext->setMaximumHeight(180);
    layout->addWidget(m_calendarNext);

    updateCalendars();

    layout->addStretch();

    m_screenButton = new QPushButton("Screen", this);
    m_screenButton->setMinimumHeight(36);
    layout->addWidget(m_screenButton);

    m_rateLimitLabel = new QLabel("", this);
    m_rateLimitLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_rateLimitLabel);

    connect(m_modeCombo,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &ScreenerPanel::onModeChanged);

    connect(m_minDays,
        QOverload<int>::of(&QSpinBox::valueChanged),
        this, [this]() { updateCalendars(); });

    connect(m_maxDays,
        QOverload<int>::of(&QSpinBox::valueChanged),
        this, [this]() { updateCalendars(); });

    connect(m_screenButton, &QPushButton::clicked, this,
        [this]() {
            emit screenRequested({});
        });
}

void ScreenerPanel::onModeChanged(int index) {
    auto mode = static_cast<ScreenerMode>(index);
    bool bullPut = isBullPutMode(mode);
    m_tradeTypeCombo->setEnabled(!bullPut);
    if (bullPut) m_tradeTypeCombo->setCurrentIndex(0);
}

void ScreenerPanel::updateCalendars() {
    int minDays = m_minDays->value();
    int maxDays = m_maxDays->value();
    m_calendarCurrent->setDteRange(minDays, maxDays);
    m_calendarNext->setDteRange(minDays, maxDays);
}

void ScreenerPanel::restoreFromSettings(
    const AppSettings& settings)
{
    m_symbolInput->setText(settings.lastTicker);
    m_modeCombo->setCurrentIndex(settings.lastMode);
    m_tradeTypeCombo->setCurrentIndex(settings.lastTradeType);
    m_minProbability->setValue(settings.lastMinProbability);
    m_minDays->setValue(settings.lastMinDays);
    m_maxDays->setValue(settings.lastMaxDays);
    m_fridaysOnly->setChecked(settings.lastFridaysOnly);
    updateCalendars();
}

void ScreenerPanel::saveToSettings(AppSettings& settings) const {
    settings.lastTicker = m_symbolInput->text().toUpper();
    settings.lastMode = m_modeCombo->currentIndex();
    settings.lastTradeType = m_tradeTypeCombo->currentIndex();
    settings.lastMinProbability = m_minProbability->value();
    settings.lastMinDays = m_minDays->value();
    settings.lastMaxDays = m_maxDays->value();
    settings.lastFridaysOnly = m_fridaysOnly->isChecked();
}

void ScreenerPanel::setScreening(bool screening) {
    m_screenButton->setEnabled(!screening);
    m_screenButton->setText(screening ? "Screening..." : "Screen");
}

void ScreenerPanel::setRateLimitRemaining(int remaining) {
    if (remaining >= 0) {
        m_rateLimitLabel->setText(
            QString("Daily rate limit remaining: %1")
                .arg(remaining));
    }
}

ScreenerParams ScreenerPanel::currentParams(
    const AppSettings& settings) const
{
    ScreenerParams p;
    p.ticker = m_symbolInput->text().toUpper();
    p.mode = static_cast<ScreenerMode>(
        m_modeCombo->currentIndex());
    p.tradeType = static_cast<TradeType>(
        m_tradeTypeCombo->currentIndex());
    p.minProbability = m_minProbability->value();
    p.minDays = m_minDays->value();
    p.maxDays = m_maxDays->value();
    p.fridaysOnly = m_fridaysOnly->isChecked();

    p.maxStrikes = settings.maxStrikes;
    p.commission = settings.commission;
    p.minAnnualReturn = settings.minAnnualReturn;
    p.minProfitAmount = settings.minProfitAmount;
    p.pricePointWeight = settings.pricePointWeight;
    p.profitPointWeight = settings.profitPointWeight;
    p.probabilityWeight = settings.probabilityWeight;
    p.profitLossWeight = settings.profitLossWeight;
    p.annualReturnWeight = settings.annualReturnWeight;
    p.deltaWeight = settings.deltaWeight;
    p.hundredTradeWeight = settings.hundredTradeWeight;
    p.maxMargin = settings.maxMargin;
    p.minBullPutStrikeBelow = settings.minBullPutStrikeBelow;
    p.targetSellStrike = settings.targetSellStrike;
    p.apiToken = settings.apiToken;
    return p;
}
