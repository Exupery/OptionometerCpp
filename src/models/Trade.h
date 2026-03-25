#pragma once

#include "Option.h"
#include <vector>
#include <QString>

class Trade {
public:
    Trade(std::vector<Option> buys, std::vector<Option> sells);

    double profitLossAtPrice(double target) const;
    double requiredMargin() const;
    QString toString() const;

    const std::vector<Option>& getBuys() const { return m_buys; }
    const std::vector<Option>& getSells() const { return m_sells; }

    static void setCommissionPerShare(double commission);
    static double commissionPerShare();

private:
    static double callBuyPl(const Option& call, double target);
    static double callSellPl(const Option& call, double target);
    static double putBuyPl(const Option& put, double target);
    static double putSellPl(const Option& put, double target);

    double calculateMargin(
        const std::vector<Option>& buys,
        const std::vector<Option>& sells) const;
    double calculateMarginPair(
        const Option& buy, const Option& sell) const;

    std::vector<Option> m_buys;
    std::vector<Option> m_sells;
    std::vector<Option> m_callBuys;
    std::vector<Option> m_putBuys;
    std::vector<Option> m_callSells;
    std::vector<Option> m_putSells;

    static double s_commissionPerShare;
};
