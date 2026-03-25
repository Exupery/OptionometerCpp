#pragma once

class Weigher {
public:
    Weigher(int ppWeight = 1, int npWeight = 1, int probWeight = 1,
            int plWeight = 1, int arWeight = 1, int deltaWeight = 1,
            int htWeight = 1);

    double scoreByWeight(
        double pricePointScore, double numProfitPointScore,
        double probabilityScore, double maxProfitLossScore,
        double annualReturnScore, double deltaScore,
        double hundredTradesScore) const;

    double scoreByWeightBullPut(
        double numProfitPointScore, double probabilityScore,
        double maxProfitLossScore, double annualReturnScore) const;

    int hundredTradeWeight() const { return m_htWeight; }

private:
    int m_ppWeight;
    int m_npWeight;
    int m_probWeight;
    int m_plWeight;
    int m_arWeight;
    int m_deltaWeight;
    int m_htWeight;
};
