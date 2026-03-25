#pragma once

#include "Option.h"
#include <vector>
#include <QString>

struct OptionChain {
    QString underlying;
    double underlyingPrice = 0.0;
    qint64 expiry = 0;
    std::vector<Option> calls;
    std::vector<Option> puts;
};
