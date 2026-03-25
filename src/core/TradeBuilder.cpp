#include "TradeBuilder.h"
#include <algorithm>

TradeBuilder::TradeBuilder(const OptionChain& chain)
    : m_chain(chain)
{}

std::vector<std::shared_ptr<Trade>> TradeBuilder::spreads() const {
    auto result = buildSpreads(m_chain.calls, m_chain.calls);
    auto pp = buildSpreads(m_chain.puts, m_chain.puts);
    auto cp = buildSpreads(m_chain.calls, m_chain.puts);
    auto pc = buildSpreads(m_chain.puts, m_chain.calls);
    result.insert(result.end(), pp.begin(), pp.end());
    result.insert(result.end(), cp.begin(), cp.end());
    result.insert(result.end(), pc.begin(), pc.end());
    return result;
}

std::vector<std::shared_ptr<Trade>>
TradeBuilder::threeLegTrades() const {
    return buildThreeLegTrades();
}

std::vector<std::shared_ptr<Trade>>
TradeBuilder::fourLegTrades() const {
    return buildFourLegTrades();
}

std::vector<std::shared_ptr<Trade>>
TradeBuilder::enhancedTrades(
    const std::vector<std::shared_ptr<Trade>>& trades) const
{
    std::vector<std::shared_ptr<Trade>> result;
    auto allOptions = m_chain.calls;
    allOptions.insert(allOptions.end(),
        m_chain.puts.begin(), m_chain.puts.end());

    for (const auto& trade : trades) {
        for (const auto& option : allOptions) {
            if (trade->getBuys().size() < trade->getSells().size()) {
                auto newBuys = trade->getBuys();
                newBuys.push_back(option);
                result.push_back(std::make_shared<Trade>(
                    newBuys, trade->getSells()));
            } else {
                auto newSells = trade->getSells();
                newSells.push_back(option);
                result.push_back(std::make_shared<Trade>(
                    trade->getBuys(), newSells));
            }
        }
    }
    return result;
}

std::vector<std::shared_ptr<Trade>>
TradeBuilder::condors() const {
    auto puts = m_chain.puts;
    auto calls = m_chain.calls;
    std::sort(puts.begin(), puts.end(),
        [](const Option& a, const Option& b) {
            return a.strike < b.strike;
        });
    std::sort(calls.begin(), calls.end(),
        [](const Option& a, const Option& b) {
            return a.strike < b.strike;
        });

    auto maxIdx = std::min(puts.size(), calls.size());
    if (maxIdx < 4) return {};

    std::vector<std::pair<Option, Option>> putPairs;
    for (size_t i = 0; i + 3 < maxIdx; ++i) {
        for (size_t j = i + 1; j + 1 < maxIdx; ++j) {
            putPairs.emplace_back(puts[i], puts[j]);
        }
    }

    std::vector<std::pair<Option, Option>> callPairs;
    for (size_t i = 2; i < maxIdx; ++i) {
        for (size_t j = i + 1; j < maxIdx; ++j) {
            callPairs.emplace_back(calls[i], calls[j]);
        }
    }

    std::vector<std::shared_ptr<Trade>> result;
    for (const auto& [longPut, shortPut] : putPairs) {
        for (const auto& [shortCall, longCall] : callPairs) {
            if (shortPut.strike >= shortCall.strike) continue;
            result.push_back(std::make_shared<Trade>(
                std::vector<Option>{longPut, longCall},
                std::vector<Option>{shortPut, shortCall}));
        }
    }
    return result;
}

std::vector<std::shared_ptr<Trade>>
TradeBuilder::bullPutSpreads() const {
    auto puts = m_chain.puts;
    std::sort(puts.begin(), puts.end(),
        [](const Option& a, const Option& b) {
            return a.strike < b.strike;
        });

    std::vector<std::shared_ptr<Trade>> result;
    for (size_t i = 0; i < puts.size(); ++i) {
        for (size_t j = i + 1; j < puts.size(); ++j) {
            result.push_back(std::make_shared<Trade>(
                std::vector<Option>{puts[i]},
                std::vector<Option>{puts[j]}));
        }
    }
    return result;
}

std::vector<std::shared_ptr<Trade>>
TradeBuilder::buildSpreads(
    const std::vector<Option>& sells,
    const std::vector<Option>& buys) const
{
    std::vector<std::shared_ptr<Trade>> result;
    for (const auto& sell : sells) {
        for (const auto& buy : buys) {
            if (buy.strike == sell.strike) continue;
            result.push_back(std::make_shared<Trade>(
                std::vector<Option>{buy},
                std::vector<Option>{sell}));
        }
    }
    return result;
}

std::vector<std::shared_ptr<Trade>>
TradeBuilder::buildThreeLegTrades() const {
    auto sp = spreads();
    auto allOptions = m_chain.puts;
    allOptions.insert(allOptions.end(),
        m_chain.calls.begin(), m_chain.calls.end());

    std::vector<std::shared_ptr<Trade>> result;
    for (const auto& spread : sp) {
        for (const auto& third : allOptions) {
            // Add third as sell
            auto newSells = spread->getSells();
            newSells.push_back(third);
            result.push_back(std::make_shared<Trade>(
                spread->getBuys(), newSells));

            // Add third as buy
            auto newBuys = spread->getBuys();
            newBuys.push_back(third);
            result.push_back(std::make_shared<Trade>(
                newBuys, spread->getSells()));
        }
    }
    return result;
}

std::vector<std::shared_ptr<Trade>>
TradeBuilder::buildFourLegTrades() const {
    auto sp = spreads();
    std::vector<std::shared_ptr<Trade>> result;

    for (const auto& base : sp) {
        for (const auto& other : sp) {
            auto bBuy = other->getBuys()[0].strike;
            auto bSell = other->getSells()[0].strike;
            auto baseBuy = base->getBuys()[0].strike;
            auto baseSell = base->getSells()[0].strike;

            if (bBuy == baseBuy || bBuy == baseSell
                || bSell == baseBuy || bSell == baseSell)
                continue;

            auto combinedBuys = base->getBuys();
            auto otherBuys = other->getBuys();
            combinedBuys.insert(combinedBuys.end(),
                otherBuys.begin(), otherBuys.end());

            auto combinedSells = base->getSells();
            auto otherSells = other->getSells();
            combinedSells.insert(combinedSells.end(),
                otherSells.begin(), otherSells.end());

            result.push_back(std::make_shared<Trade>(
                combinedBuys, combinedSells));
        }
    }
    return result;
}
