#pragma once

#include "models/OptionChain.h"
#include "models/Trade.h"
#include <vector>
#include <memory>

class TradeBuilder {
public:
    explicit TradeBuilder(const OptionChain& chain);

    std::vector<std::shared_ptr<Trade>> spreads() const;
    std::vector<std::shared_ptr<Trade>> threeLegTrades() const;
    std::vector<std::shared_ptr<Trade>> fourLegTrades() const;
    std::vector<std::shared_ptr<Trade>> condors() const;
    std::vector<std::shared_ptr<Trade>> bullPutSpreads() const;
    std::vector<std::shared_ptr<Trade>> nakedPuts() const;
    std::vector<std::shared_ptr<Trade>> enhancedTrades(
        const std::vector<std::shared_ptr<Trade>>& trades) const;

    double underlyingPrice() const { return m_chain.underlyingPrice; }

private:
    std::vector<std::shared_ptr<Trade>> buildSpreads(
        const std::vector<Option>& sells,
        const std::vector<Option>& buys) const;
    std::vector<std::shared_ptr<Trade>> buildThreeLegTrades() const;
    std::vector<std::shared_ptr<Trade>> buildFourLegTrades() const;

    OptionChain m_chain;
};
