#pragma once

#include "Enums.h"
#include <QString>

struct Option {
    QString symbol;
    double strike = 0.0;
    Side side = Side::Call;
    qint64 expiry = 0;
    int dte = 0;
    double bid = 0.0;
    double ask = 0.0;
    double impliedVolatility = 0.0;
    double delta = 0.0;
};
