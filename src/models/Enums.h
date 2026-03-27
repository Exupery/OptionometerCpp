#pragma once

#include <QString>
#include <map>

enum class Side { Call, Put };

enum class ScreenerMode {
    StrategyOptimizer,
    BullPutSpreadScreener,
    BullPutSpreadOptimizer,
    NakedPutSell
};

enum class TradeType {
    BullPutSpreads,
    Condors,
    TwoLeg,
    ThreeLeg,
    FourLeg
};

inline QString screenerModeToString(ScreenerMode mode) {
    switch (mode) {
    case ScreenerMode::StrategyOptimizer:       return "Strategy Optimizer";
    case ScreenerMode::BullPutSpreadScreener:   return "Bull Put Spread Screener";
    case ScreenerMode::BullPutSpreadOptimizer:  return "Bull Put Spread Optimizer";
    case ScreenerMode::NakedPutSell:            return "Naked Puts";
    }
    return {};
}

inline QString tradeTypeToString(TradeType type) {
    switch (type) {
    case TradeType::BullPutSpreads: return "Bull Put Spreads";
    case TradeType::Condors:        return "Condors";
    case TradeType::TwoLeg:         return "2-Leg";
    case TradeType::ThreeLeg:       return "3-Leg";
    case TradeType::FourLeg:        return "4-Leg";
    }
    return {};
}

inline bool isBullPutMode(ScreenerMode mode) {
    return mode == ScreenerMode::BullPutSpreadScreener
        || mode == ScreenerMode::BullPutSpreadOptimizer;
}

inline bool isNakedPutMode(ScreenerMode mode) {
    return mode == ScreenerMode::NakedPutSell;
}
