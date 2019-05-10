#include "pricer.h"
#include <iostream>
#include "QUrl"
#include "QUrlQuery"
#include "QtNetwork/QNetworkReply"
#include "QJsonDocument"
#include "QJsonObject"
#include "QJsonArray"
#include "QEventLoop"

#include <ql/time/date.hpp>
#include <ql/time/calendar.hpp>
#include <ql/time/calendars/unitedstates.hpp>
#include <ql/termstructures/yieldtermstructure.hpp>
#include <ql/termstructures/volatility/equityfx/blackvoltermstructure.hpp>
#include <ql/termstructures/volatility/equityfx/blackvariancesurface.hpp>
#include <ql/termstructures/yield/piecewiseyieldcurve.hpp>
#include <ql/time/daycounters/actualactual.hpp>
#include <ql/indexes/iborindex.hpp>
#include <ql/indexes/ibor/usdlibor.hpp>
#include <ql/termstructures/yield/ratehelpers.hpp>
#include <ql/math/interpolations/cubicinterpolation.hpp>

using namespace QuantLib;

pricer::pricer(QObject *parent) : QObject(parent)
{
    qnam_ = new QNetworkAccessManager(this);
}

boost::shared_ptr<ZeroCurve> pricer::buildDividendCurve(const Date &evaluationDate,
                                                        const Date &expiration,
                                                        const Date &exDivDate,
                                                        Real underlyingPrice,
                                                        Real annualDividend)
{
    const QuantLib::Calendar calendar = UnitedStates(UnitedStates::NYSE);
    Settings::instance().evaluationDate() = evaluationDate;
    Real settlementDays = 2.0;

    Real dividendDiscountDays = (expiration - evaluationDate) + settlementDays;
    Rate dividendYield = (annualDividend/underlyingPrice) * dividendDiscountDays/365;

    // ex div dates and yields
    std::vector<Date> exDivDates;
    std::vector<Rate> dividendYields;

    //last ex div date and yield
    exDivDates.push_back(calendar.advance(exDivDate, Period(-3, Months), ModifiedPreceding, true));
    dividendYields.push_back(dividendYield);

    //currently announced ex div date and yield
    exDivDates.push_back(exDivDate);
    dividendYields.push_back(dividendYield);

    //next ex div date (projected) and yield
    Date projectedNextExDivDate = calendar.advance(exDivDate, Period(3, Months), ModifiedPreceding, true);
    std::cout << "Next projected ex div date: " << projectedNextExDivDate << std::endl;
    exDivDates.push_back(projectedNextExDivDate);
    dividendYields.push_back(dividendYield);

    return boost::shared_ptr<ZeroCurve>(new ZeroCurve(exDivDates, dividendYields, ActualActual(), calendar));

}

boost::shared_ptr<YieldTermStructure> pricer::buildLiborZeroCurve(const QString apiKey)
{
    // Go to the St. Louis federal reserve website to get libor rates: over-night thru 12 month.

    const QStringList liborDuration = (QStringList() << "USDONTD156N" << "USD1WKD156N" << "USD1MTD156N"
                                 << "USD2MTD156N" << "USD3MTD156N" << "USD6MTD156N" << "USD12MD156N");

    QUrl liborUrl("https://api.stlouisfed.org/fred/series/observations");
    QUrlQuery urlQry;

    QuantLib::Date liborEvalDate = QuantLib::Date::todaysDate();
    boost::shared_ptr<IborIndex> libor(new QuantLib::USDLiborON);
    const Calendar &liborCalendar = libor->fixingCalendar();
    Settings::instance().anchorEvaluationDate();//same as Date::todaysDate()
    const DayCounter &liborDayCounter = libor->dayCounter();
    const Date& liborSettlement = liborCalendar.advance(liborEvalDate, 2, Days);

    /* the website's rates are delayed by 1 week. Need to scrape another website
     * if want up to date rates.
     */
    std::vector<QuantLib::Rate> interestRateCurve;
    for (const auto &duration : liborDuration) {
        urlQry.clear();
        urlQry.addQueryItem("series_id", duration);
        urlQry.addQueryItem("sort_order", "desc");
        urlQry.addQueryItem("limit", QString::number(1)); // 1 == most recent rate quote
        urlQry.addQueryItem("api_key", apiKey);
        urlQry.addQueryItem("file_type", "json");
        liborUrl.setQuery(urlQry);
        QByteArray data = httpSyncRequest(liborUrl);
        QJsonDocument qdoc = QJsonDocument::fromJson(data);
        QJsonObject qObject = qdoc.object();
        QJsonArray dataArray = qObject["observations"].toArray();
        QJsonObject jObj = dataArray.at(0).toObject();
        QuantLib::Rate qlRate = jObj.value("value").toVariant().toDouble() / 100.0;
        interestRateCurve.push_back(qlRate);
    }
    std::vector<boost::shared_ptr<QuantLib::RateHelper>> liborRates;
    liborRates.push_back( boost::shared_ptr<QuantLib::RateHelper>(new DepositRateHelper(interestRateCurve[0],
        boost::shared_ptr<IborIndex>(new USDLiborON()))));
    liborRates.push_back(boost::shared_ptr<RateHelper>(new DepositRateHelper(interestRateCurve[1],
        boost::shared_ptr<IborIndex>(new USDLibor(Period(1, Weeks))))));
    liborRates.push_back( boost::shared_ptr<RateHelper>(new DepositRateHelper(interestRateCurve[2],
        boost::shared_ptr<IborIndex>(new USDLibor(Period(1, Months))))));
    liborRates.push_back(boost::shared_ptr<RateHelper>(new DepositRateHelper(interestRateCurve[3],
        boost::shared_ptr<IborIndex>(new USDLibor(Period(2, Months))))));
    liborRates.push_back(boost::shared_ptr<RateHelper>(new DepositRateHelper(interestRateCurve[4],
        boost::shared_ptr<IborIndex>(new USDLibor(Period(3, Months))))));
    liborRates.push_back(boost::shared_ptr<RateHelper>(new DepositRateHelper(interestRateCurve[5],
        boost::shared_ptr<IborIndex>(new USDLibor(Period(6, Months))))));
    liborRates.push_back(boost::shared_ptr<RateHelper>(new DepositRateHelper(interestRateCurve[6],
        boost::shared_ptr<IborIndex>(new USDLibor(Period(12, Months))))));

    boost::shared_ptr<YieldTermStructure> yieldCurve = boost::shared_ptr<YieldTermStructure>
            (new PiecewiseYieldCurve<ZeroYield,Cubic>(liborSettlement, liborRates, liborDayCounter));

    return yieldCurve;
}

boost::shared_ptr<BlackVolTermStructure> pricer::buildVolatilityCurve(const Date &evaluationDate,
                                                                      const std::vector<Real> &strikes,
                                                                      const std::vector<Volatility> &vols,
                                                                      const Date &expiration)
{
    Calendar calendar = UnitedStates(UnitedStates::NYSE);
    std::vector<Date> expirations;
    expirations.push_back(expiration);
    Matrix volMatrix(strikes.size(), 1);

    for (int i=0; i< vols.size(); ++i) {
        volMatrix[i][0] = vols[i];
    }

    return boost::shared_ptr<BlackVolTermStructure>(new BlackVarianceSurface(evaluationDate, calendar, expirations, strikes, volMatrix, Actual365Fixed()));
}

QByteArray pricer::httpSyncRequest(const QUrl url)
{
    /* This function takes a QUrl to a website as an argument and synchronously sends a request
     * for data using a QEventLoop, reads it into a QBytearray and returns it.
     */

    QNetworkReply* reply;
    QNetworkRequest request;
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    QEventLoop loop;
    QObject::connect(qnam_, SIGNAL( finished(QNetworkReply *) ), &loop, SLOT( quit() ) );
    request.setUrl( url );
    reply = qnam_->get( request );
    loop.exec();

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if(statusCode != 200) {
        qDebug() << "ERROR from httpSyncRequest():"<<statusCode<<reply->errorString()<<url;
    }
    reply->deleteLater();
    return reply->readAll();
}
