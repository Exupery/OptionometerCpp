#pragma once

#include "models/ScoredTrade.h"
#include "models/ScoredBullPut.h"
#include "Weigher.h"
#include <vector>

namespace Normalizer {

std::vector<ScoredTrade> normalize(
    const std::vector<RawScoredTrade>& rawScored,
    const Weigher& weigher = Weigher());

std::vector<ScoredBullPut> normalizeBullPuts(
    const std::vector<RawScoredBullPut>& rawScored,
    const Weigher& weigher = Weigher());

std::map<int, double> normalizeScores(
    const std::vector<std::pair<int, double>>& indexedScores,
    bool ignoreNegative = true);

} // namespace Normalizer
