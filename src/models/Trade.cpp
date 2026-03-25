#include "Trade.h"
#include <QStringList>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

double Trade::s_commissionPerShare = 0.005;

void Trade::setCommissionPerShare(double commission) {
    s_commissionPerShare = commission;
}

double Trade::commissionPerShare() {
    return s_commissionPerShare;
}

Trade::Trade(std::vector<Option> buys, std::vector<Option> sells)
    : m_buys(std::move(buys)), m_sells(std::move(sells))
{
    for (const auto& o : m_buys) {
        if (o.side == Side::Call) m_callBuys.push_back(o);
        else m_putBuys.push_back(o);
    }
    for (const auto& o : m_sells) {
        if (o.side == Side::Call) m_callSells.push_back(o);
        else m_putSells.push_back(o);
    }
}

double Trade::profitLossAtPrice(double target) const {
    auto sum = [&](const auto& opts, auto fn) {
        double total = 0.0;
        for (const auto& o : opts) total += fn(o, target);
        return total;
    };
    return sum(m_callBuys, callBuyPl)
         + sum(m_callSells, callSellPl)
         + sum(m_putBuys, putBuyPl)
         + sum(m_putSells, putSellPl);
}

double Trade::callBuyPl(const Option& call, double target) {
    double cost = call.ask + s_commissionPerShare;
    if (call.strike > target) return -cost;
    return target - call.strike - cost;
}

double Trade::callSellPl(const Option& call, double target) {
    double credit = call.bid - s_commissionPerShare;
    if (call.strike > target) return credit;
    return -(target - call.strike) + credit;
}

double Trade::putBuyPl(const Option& put, double target) {
    double cost = put.ask + s_commissionPerShare;
    if (put.strike < target) return -cost;
    return put.strike - target - cost;
}

double Trade::putSellPl(const Option& put, double target) {
    double credit = put.bid - s_commissionPerShare;
    if (put.strike < target) return credit;
    return -(put.strike - target) + credit;
}

double Trade::requiredMargin() const {
    return calculateMargin(m_callBuys, m_callSells)
         + calculateMargin(m_putBuys, m_putSells);
}

double Trade::calculateMargin(
    const std::vector<Option>& buys,
    const std::vector<Option>& sells) const
{
    if (buys.empty() && sells.empty()) return 0.0;

    if (!buys.empty() && !sells.empty()) {
        auto sides = buys.front().side;
        for (const auto& o : sells) {
            if (o.side != sides) {
                throw std::invalid_argument(
                    "Buys and sells must be same side");
            }
        }
    }

    if (sells.empty()) {
        double total = 0.0;
        for (const auto& b : buys) total += b.ask;
        return total * 100.0;
    }
    if (buys.empty()) {
        double total = 0.0;
        for (const auto& s : sells) total += (s.strike - s.bid);
        return total * 100.0;
    }

    auto bSorted = buys;
    auto sSorted = sells;
    std::sort(bSorted.begin(), bSorted.end(),
        [](const Option& a, const Option& b) {
            return a.strike < b.strike;
        });
    std::sort(sSorted.begin(), sSorted.end(),
        [](const Option& a, const Option& b) {
            return a.strike < b.strike;
        });

    if (bSorted.size() == sSorted.size()) {
        double total = 0.0;
        for (size_t i = 0; i < bSorted.size(); ++i) {
            total += calculateMarginPair(bSorted[i], sSorted[i]);
        }
        return total;
    }

    auto minSize = std::min(bSorted.size(), sSorted.size());
    std::vector<Option> bPaired(bSorted.begin(),
        bSorted.begin() + minSize);
    std::vector<Option> sPaired(sSorted.begin(),
        sSorted.begin() + minSize);
    double paired = calculateMargin(bPaired, sPaired);

    double remaining = 0.0;
    if (bSorted.size() > minSize) {
        std::vector<Option> bRem(bSorted.begin() + minSize,
            bSorted.end());
        remaining = calculateMargin(bRem, {});
    } else {
        std::vector<Option> sRem(sSorted.begin() + minSize,
            sSorted.end());
        remaining = calculateMargin({}, sRem);
    }
    return paired + remaining;
}

double Trade::calculateMarginPair(
    const Option& buy, const Option& sell) const
{
    if (buy.ask >= sell.bid) {
        return (buy.ask - sell.bid) * 100.0;
    }
    double diff = std::abs(buy.strike - sell.strike);
    return (diff - (sell.bid - buy.ask)) * 100.0;
}

QString Trade::toString() const {
    QStringList buyParts, sellParts;
    for (const auto& b : m_buys) {
        QString side = (b.side == Side::Call) ? "CALL" : "PUT";
        buyParts << QString("%1 %2").arg(side).arg(b.strike);
    }
    for (const auto& s : m_sells) {
        QString side = (s.side == Side::Call) ? "CALL" : "PUT";
        sellParts << QString("%1 %2").arg(side).arg(s.strike);
    }
    return QString("BUY [%1] - SELL [%2]")
        .arg(buyParts.join(", "), sellParts.join(", "));
}
