#ifndef PRICER_H
#define PRICER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <ql/time/date.hpp>
#include <ql/termstructures/yieldtermstructure.hpp>
#include <ql/termstructures/volatility/equityfx/blackvoltermstructure.hpp>
#include <ql/termstructures/yield/zeroyieldstructure.hpp>
#include <ql/termstructures/yield/piecewiseyieldcurve.hpp>


using namespace QuantLib;
class pricer : public QObject
{
    Q_OBJECT
public:
    explicit pricer(QObject *parent = nullptr);

    boost::shared_ptr<ZeroCurve> buildDividendCurve(const Date& evaluationDate,
                                                        const Date& expiration,
                                                        const Date& exDivDate,
                                                        Real underlyingPrice,
                                                        Real annualDividend);

    boost::shared_ptr<YieldTermStructure> buildLiborZeroCurve(const QString apiKey);

    boost::shared_ptr<BlackVolTermStructure> buildVolatilityCurve(const Date& evaluationDate,
                                                                      const std::vector<Real>& strikes,
                                                                      const std::vector<Volatility>& vols,
                                                                      const Date& expiration);

    QByteArray httpSyncRequest(const QUrl url);

signals:

public slots:

private:
    QNetworkAccessManager *qnam_;

};

#endif // PRICER_H
