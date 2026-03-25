#include "Weigher.h"

Weigher::Weigher(int ppWeight, int npWeight, int probWeight,
                 int plWeight, int arWeight, int deltaWeight,
                 int htWeight)
    : m_ppWeight(ppWeight)
    , m_npWeight(npWeight)
    , m_probWeight(probWeight)
    , m_plWeight(plWeight)
    , m_arWeight(arWeight)
    , m_deltaWeight(deltaWeight)
    , m_htWeight(htWeight)
{}

double Weigher::scoreByWeight(
    double pricePointScore, double numProfitPointScore,
    double probabilityScore, double maxProfitLossScore,
    double annualReturnScore, double deltaScore,
    double hundredTradesScore) const
{
    return (pricePointScore * m_ppWeight)
        + (numProfitPointScore * m_npWeight)
        + (probabilityScore * m_probWeight)
        + (maxProfitLossScore * m_plWeight)
        + (annualReturnScore * m_arWeight)
        + (deltaScore * m_deltaWeight)
        + (hundredTradesScore * m_htWeight);
}

double Weigher::scoreByWeightBullPut(
    double numProfitPointScore, double probabilityScore,
    double maxProfitLossScore, double annualReturnScore) const
{
    return (numProfitPointScore * m_npWeight)
        + (probabilityScore * m_probWeight)
        + (maxProfitLossScore * m_plWeight)
        + (annualReturnScore * m_arWeight);
}
