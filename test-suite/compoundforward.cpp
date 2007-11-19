/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2003 RiskMap srl

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

#include "compoundforward.hpp"
#include "utilities.hpp"
#include <ql/legacy/termstructures/compoundforward.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/time/schedule.hpp>
#include <ql/instruments/vanillaswap.hpp>
#include <ql/pricingengines/swap/discountingswapengine.hpp>
#include <ql/indexes/ibor/jibar.hpp>
#include <ql/utilities/dataformatters.hpp>
#include <iomanip>

using namespace QuantLib;
using namespace boost::unit_test_framework;

QL_BEGIN_TEST_LOCALS(CompoundForwardTest)

struct Datum {
    Integer n;
    TimeUnit units;
    Rate rate;
};

Datum depositData[] = {
    { 3, Months, 4.557 },
    { 6, Months, 4.496 },
    { 9, Months, 4.490 }
};

Datum swapData[] = {
    {  1, Years, 4.54 },
    {  2, Years, 4.63 },
    {  3, Years, 4.75 },
    {  4, Years, 4.86 },
    {  5, Years, 4.99 },
    {  6, Years, 5.11 },
    {  7, Years, 5.23 },
    {  8, Years, 5.33 },
    {  9, Years, 5.41 },
    { 10, Years, 5.47 },
    { 12, Years, 5.60 },
    { 15, Years, 5.75 },
    { 20, Years, 5.89 },
    { 25, Years, 5.95 },
    { 30, Years, 5.96 }
};

// test-global variables

Calendar calendar;
Natural settlementDays;
Date today, settlement;
BusinessDayConvention convention;
DayCounter dayCounter;
Frequency frequency;

Size deposits, swaps;
std::vector<Rate> rates;
std::vector<Date> dates;
boost::shared_ptr<CompoundForward> termStructure;

void setup() {

    // data
    calendar = SouthAfrica();
    settlementDays = 0;
    today = calendar.adjust(Date::todaysDate());
    Settings::instance().evaluationDate() = today;
    settlement = calendar.advance(today,settlementDays,Days);
    convention = ModifiedFollowing;
    dayCounter = Actual365Fixed();
    frequency = Semiannual;

    deposits = LENGTH(depositData);
    swaps = LENGTH(swapData);

    // market elements
    rates = std::vector<Rate>(deposits+swaps);
    dates = std::vector<Date>(deposits+swaps);
    Size i;
    for (i=0; i<deposits; i++) {
        rates[i] = depositData[i].rate/100;
        dates[i] = calendar.advance(settlement,
                                    Period(depositData[i].n,
                                           depositData[i].units),
                                    convention);
    }
    for (i=0; i<swaps; i++) {
        rates[i+deposits] = swapData[i].rate/100;
        dates[i+deposits] = calendar.advance(settlement,
                                             Period(swapData[i].n,
                                                    swapData[i].units),
                                             convention);
    }

    termStructure = boost::shared_ptr<CompoundForward>(
                             new CompoundForward(settlement,dates,rates,
                                                 calendar,convention,
                                                 frequency,dayCounter));
}

QL_END_TEST_LOCALS(CompoundForwardTest)


void CompoundForwardTest::testSuppliedRates() {

    BOOST_MESSAGE("Testing consistency of compound-forward curve "
                  "with supplied rates...");

    SavedSettings backup;

    setup();

    Handle<YieldTermStructure> liborHandle =
        Handle<YieldTermStructure>(termStructure);

    Size i;
    // check swaps against original
    boost::shared_ptr<IborIndex> index(new Jibar(Period(frequency),
                                             liborHandle));
    for (i=0; i<swaps; i++) {
        Date maturity = calendar.advance(settlement,
                                         swapData[i].n,swapData[i].units,
                                         convention);
        Schedule schedule(settlement, maturity, Period(frequency), calendar,
                          convention, convention, false, false);
        VanillaSwap swap(VanillaSwap::Payer, 100.0,
                         schedule, 0.0, dayCounter,
                         schedule, index, 0.0, index->dayCounter());
        swap.setPricingEngine(boost::shared_ptr<PricingEngine>(
                                     new DiscountingSwapEngine(liborHandle)));
        Rate expectedRate = swapData[i].rate/100,
             estimatedRate = swap.fairRate();
        if (std::fabs(expectedRate-estimatedRate) > 1.0e-9) {
            BOOST_FAIL(swapData[i].n << " year(s) swap:\n"
                       << std::setprecision(8)
                       << "    estimated rate: "
                       << io::rate(estimatedRate) << "\n"
                       << "    expected rate:  "
                       << io::rate(expectedRate));
        }
    }
}

void CompoundForwardTest::testConvertedRates() {

    BOOST_MESSAGE("Testing consistency of compound-forward curve "
                  "with converted rates...");

    SavedSettings backup;

    setup();

    Handle<YieldTermStructure> liborHandle =
        Handle<YieldTermStructure>(termStructure);

    Size i;
    frequency = Quarterly;
    // check swaps against quarterly rates
    boost::shared_ptr<IborIndex> index(new Jibar(Period(frequency),
                                             liborHandle));
    for (i=0; i<swaps; i++) {
        Date maturity = calendar.advance(settlement,
                                         swapData[i].n,swapData[i].units,
                                         convention);
        Schedule schedule(settlement, maturity, Period(frequency), calendar,
                          convention, convention, false, false);
        VanillaSwap swap(VanillaSwap::Payer, 100.0,
                         schedule, 0.0, dayCounter,
                         schedule, index, 0.0, index->dayCounter());
        swap.setPricingEngine(boost::shared_ptr<PricingEngine>(
                                     new DiscountingSwapEngine(liborHandle)));
        DayCounter tsdc  = termStructure->dayCounter();
        Rate expectedRate = termStructure->compoundForward(swap.maturityDate(),
                                                           frequency);
        Rate estimatedRate = swap.fairRate();
        if (std::fabs(expectedRate-estimatedRate) > 1.0e-9) {
            BOOST_FAIL(swapData[i].n << " year(s) swap:\n"
                       << std::setprecision(8)
                       << "    estimated rate: "
                       << io::rate(estimatedRate) << "\n"
                       << "    compound rate:  "
                       << io::rate(expectedRate));
        }
    }
}


test_suite* CompoundForwardTest::suite() {
    test_suite* suite = BOOST_TEST_SUITE("Compound forward tests");
    suite->add(BOOST_TEST_CASE(&CompoundForwardTest::testSuppliedRates));
    suite->add(BOOST_TEST_CASE(&CompoundForwardTest::testConvertedRates));
    return suite;
}
