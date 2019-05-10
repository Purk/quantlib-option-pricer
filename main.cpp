#include <QCoreApplication>
#include "pricer.h"

#include <memory>
#include <boost/shared_ptr.hpp>

#include <ql/pricingengines/all.hpp>
#include <ql/time/calendars/unitedstates.hpp>
#include <ql/quotes/simplequote.hpp>

using namespace QuantLib;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    pricer pricerObj;

    //set up calendar/dates
    Calendar calendar = UnitedStates(UnitedStates::NYSE);
    Date today(10, May, 2019);
    Real settlementDays = 2;
    Date settlement = calendar.advance(today, settlementDays, Days);
    Settings::instance().evaluationDate() = today;

    QString ticker = "CSCO";
    Option::Type type(Option::Put);
    Real underlying = 53.16;
    std::vector<double> strikes{50,51,52,53,54,55,56};

    // volatility for each strike
    std::vector<double> vols{.3022,.2915,.2817,.2735,.2648,.2581,.2488};

    Date expiration(7, June, 2019);
    Date exDivDate(4, April, 2019);
    Real annualDividend = 1.40;

    // get an api key from st. louis federal reserve website
    Handle<YieldTermStructure> yieldTermStructure(pricerObj.buildLiborZeroCurve("apikey"));

    Handle<YieldTermStructure> dividendTermStructure(pricerObj.buildDividendCurve(today,
                                                                                  expiration,
                                                                                  exDivDate,
                                                                                  underlying,
                                                                                  annualDividend));

    Handle<BlackVolTermStructure> volatilityTermStructure(pricerObj.buildVolatilityCurve(today,
                                                                                   strikes,
                                                                                   vols,
                                                                                   expiration));

    //instantiate BSM process
    Handle<Quote> underlyingH(boost::shared_ptr<SimpleQuote>(new SimpleQuote(underlying)));
    boost::shared_ptr<BlackScholesMertonProcess> bsmProcess(new BlackScholesMertonProcess(underlyingH,
                                                                                          dividendTermStructure,
                                                                                          yieldTermStructure,
                                                                                          volatilityTermStructure));

    //instantiate pricing engine
    boost::shared_ptr<PricingEngine> pricingEngine(new FDAmericanEngine<CrankNicolson>(bsmProcess, 801, 800));

    boost::shared_ptr<Exercise> americanExercise(new AmericanExercise(settlement, expiration));
    for (Real strike: strikes) {
        boost::shared_ptr<StrikedTypePayoff> payoff(new PlainVanillaPayoff(type, strike));
        VanillaOption americanOption(payoff, americanExercise);
        americanOption.setPricingEngine(pricingEngine);
        Real tv = americanOption.NPV();
        std::cout << ticker.toStdString() <<" : " << expiration <<" " <<strike <<" "<< type<<" "<< tv << std::endl;
        std::cout << "Delta: " << americanOption.delta() << std::endl;
        std::cout << "Gamma: " << americanOption.gamma() << std::endl;
    }

    return a.exec();
}
