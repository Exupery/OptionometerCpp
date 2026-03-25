#include "MarketDataImporter.h"
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <algorithm>

static const QString ROOT_ENDPOINT =
    "https://api.marketdata.app";
static const QString CHAIN_PATH = "/v1/options/chain/";
static const QString RATE_LIMIT_HEADER =
    "X-Api-Ratelimit-Remaining";

MarketDataImporter::MarketDataImporter(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{}

void MarketDataImporter::fetchOptionChains(
    const QString& ticker, const QString& apiToken,
    int minDte, int maxDte, int maxStrikes, bool fridaysOnly)
{
    qint64 from = dteToEpoch(minDte);
    qint64 to = dteToEpoch(maxDte);

    QUrl url(ROOT_ENDPOINT + CHAIN_PATH + ticker + "/");
    QUrlQuery query;
    query.addQueryItem("from", QString::number(from));
    query.addQueryItem("to", QString::number(to));
    query.addQueryItem("strikeLimit", QString::number(maxStrikes));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization",
        ("Bearer " + apiToken).toUtf8());

    auto* reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this,
        [this, reply, ticker, fridaysOnly]() {
            reply->deleteLater();
            MarketDataResult result;

            auto rlHeader = reply->rawHeader(
                RATE_LIMIT_HEADER.toUtf8());
            if (!rlHeader.isEmpty()) {
                result.rateLimitRemaining = rlHeader.toInt();
            }

            if (reply->error() != QNetworkReply::NoError) {
                result.errorMessage = reply->errorString();
                emit finished(result);
                return;
            }

            auto data = reply->readAll();
            result = parseResponse(data, ticker, fridaysOnly);

            if (!rlHeader.isEmpty()) {
                result.rateLimitRemaining = rlHeader.toInt();
            }
            emit finished(result);
        });
}

MarketDataResult MarketDataImporter::parseResponse(
    const QByteArray& data, const QString& ticker,
    bool fridaysOnly)
{
    MarketDataResult result;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        result.errorMessage = "Invalid JSON response";
        return result;
    }

    QJsonObject root = doc.object();
    auto strikes = root["strike"].toArray();
    auto sides = root["side"].toArray();
    auto symbols = root["optionSymbol"].toArray();
    auto expirations = root["expiration"].toArray();
    auto dtes = root["dte"].toArray();
    auto bids = root["bid"].toArray();
    auto asks = root["ask"].toArray();
    auto ivs = root["iv"].toArray();
    auto deltas = root["delta"].toArray();
    auto prices = root["underlyingPrice"].toArray();

    if (strikes.isEmpty() || prices.isEmpty()) {
        result.errorMessage = "Empty option chain";
        return result;
    }

    double underlyingPrice = prices[0].toDouble();

    std::vector<Option> allOptions;
    for (int i = 0; i < strikes.size(); ++i) {
        double iv = ivs[i].toDouble();
        if (iv == 0.0) continue;

        Option opt;
        opt.symbol = symbols[i].toString();
        opt.strike = strikes[i].toDouble();
        opt.side = (sides[i].toString().toLower() == "call")
            ? Side::Call : Side::Put;
        opt.expiry = static_cast<qint64>(
            expirations[i].toDouble());
        opt.dte = dtes[i].toInt();
        opt.bid = bids[i].toDouble();
        opt.ask = asks[i].toDouble();
        opt.impliedVolatility = iv;
        opt.delta = deltas[i].toDouble();
        allOptions.push_back(opt);
    }

    std::map<qint64, std::vector<Option>> byExpiry;
    for (const auto& opt : allOptions) {
        byExpiry[opt.expiry].push_back(opt);
    }

    for (auto& [expiry, options] : byExpiry) {
        if (fridaysOnly && !expiresOnFriday(expiry)) continue;

        OptionChain chain;
        chain.underlying = ticker;
        chain.underlyingPrice = underlyingPrice;
        chain.expiry = expiry;

        for (const auto& opt : options) {
            if (opt.side == Side::Call)
                chain.calls.push_back(opt);
            else
                chain.puts.push_back(opt);
        }

        std::sort(chain.calls.begin(), chain.calls.end(),
            [](const Option& a, const Option& b) {
                return a.strike < b.strike;
            });
        std::sort(chain.puts.begin(), chain.puts.end(),
            [](const Option& a, const Option& b) {
                return a.strike < b.strike;
            });

        result.chains.push_back(std::move(chain));
    }

    std::sort(result.chains.begin(), result.chains.end(),
        [](const OptionChain& a, const OptionChain& b) {
            return a.expiry < b.expiry;
        });

    return result;
}

qint64 MarketDataImporter::dteToEpoch(int dte) {
    return QDateTime::currentSecsSinceEpoch() + dte * 86400LL;
}

bool MarketDataImporter::expiresOnFriday(qint64 epoch) {
    QDateTime dt = QDateTime::fromSecsSinceEpoch(epoch, Qt::UTC);
    return dt.date().dayOfWeek() == 5;
}
