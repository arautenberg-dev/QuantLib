/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2007 Chiara Fornarola

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

#include "assetswap.hpp"
#include "utilities.hpp"
#include <ql/time/schedule.hpp>
#include <ql/instruments/assetswap.hpp>
#include <ql/instruments/bond.hpp>
#include <ql/instruments/bonds/fixedratebond.hpp>
#include <ql/instruments/bonds/floatingratebond.hpp>
#include <ql/instruments/bonds/cmsratebond.hpp>
#include <ql/instruments/bonds/zerocouponbond.hpp>
#include <ql/index.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/time/calendars/nullcalendar.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/time/daycounters/thirty360.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/time/daycounters/actual360.hpp>
#include <ql/time/daycounters/actualactual.hpp>
#include <ql/time/daycounters/simpledaycounter.hpp>
#include <ql/indexes/ibor/euribor.hpp>
#include <ql/indexes/swapindex.hpp>
#include <ql/cashflows/fixedratecoupon.hpp>
#include <ql/cashflows/iborcoupon.hpp>
#include <ql/cashflows/cmscoupon.hpp>
#include <ql/cashflows/couponpricer.hpp>
#include <ql/cashflows/conundrumpricer.hpp>
#include <ql/termstructures/volatility/optionlet/constantoptionletvol.hpp>
#include <ql/termstructures/volatility/swaption/swaptionconstantvol.hpp>
#include <ql/termstructures/volatility/swaption/swaptionvolmatrix.hpp>
#include <ql/termstructures/volatility/swaption/swaptionvolcube2.hpp>
#include <ql/termstructures/volatility/swaption/swaptionvolcube1.hpp>
#include <ql/termstructures/volatility/swaption/swaptionvolcube.hpp>
#include <ql/utilities/dataformatters.hpp>
#include <ql/cashflows/cashflows.hpp>
#include <ql/cashflows/simplecashflow.hpp>
#include <ql/pricingengines/bond/discountingbondengine.hpp>

using namespace QuantLib;
using namespace boost::unit_test_framework;

QL_BEGIN_TEST_LOCALS(AssetSwapTest)

struct CommonVars {
    // common data
    boost::shared_ptr<IborIndex> iborIndex;
    boost::shared_ptr<SwapIndex> swapIndex;
    boost::shared_ptr<IborCouponPricer> pricer;
    boost::shared_ptr<CmsCouponPricer> cmspricer;
    Spread spread;
    Spread nonnullspread;
    Real faceAmount;
    Compounding compounding;
    RelinkableHandle<YieldTermStructure> termStructure;

    // clean-up
    SavedSettings backup;
    IndexHistoryCleaner indexCleaner;

    // initial setup
    CommonVars() {
        Natural swapSettlementDays = 2;
        faceAmount = 100.0;
        BusinessDayConvention fixedConvention = Unadjusted;
        compounding = Continuous;
        Frequency fixedFrequency = Annual;
        Frequency floatingFrequency = Semiannual;
        iborIndex = boost::shared_ptr<IborIndex>(
                     new Euribor(Period(floatingFrequency), termStructure));
        Calendar calendar = iborIndex->fixingCalendar();
        swapIndex= boost::shared_ptr<SwapIndex>(
                new SwapIndex("EuriborSwapFixA", 10*Years, swapSettlementDays,
                              iborIndex->currency(), calendar,
                              Period(fixedFrequency), fixedConvention,
                              iborIndex->dayCounter(), iborIndex));
        spread = 0.0;
        nonnullspread = 0.003;
        Date today(24,April,2007);
        Settings::instance().evaluationDate() = today;
        termStructure.linkTo(flatRate(today, 0.05, Actual365Fixed()));
        pricer = boost::shared_ptr<IborCouponPricer>(
                                                   new BlackIborCouponPricer);
        Handle<SwaptionVolatilityStructure> swaptionVolatilityStructure(
            boost::shared_ptr<SwaptionVolatilityStructure>(
                           new SwaptionConstantVolatility(today,0.2,
                                                          Actual365Fixed())));
        Handle<Quote> meanReversionQuote(
                             boost::shared_ptr<Quote>(new SimpleQuote(0.01)));
        cmspricer = boost::shared_ptr<CmsCouponPricer>(
                       new ConundrumPricerByBlack(swaptionVolatilityStructure,
                                                  GFunctionFactory::Standard,
                                                  meanReversionQuote));
    }
};

QL_END_TEST_LOCALS(AssetSwapTest)


void AssetSwapTest::testImpliedValue() {

    BOOST_MESSAGE("Testing bond implied value against asset-swap fair price"
                  " with null spread...");

    CommonVars vars;

    Calendar bondCalendar = TARGET();
    Natural settlementDays = 3;
    Natural fixingDays = 2;
    bool payFixedRate = true;
    bool parAssetSwap = true;
    bool inArrears = false;

    // Fixed Underlying bond (Isin: DE0001135275 DBR 4 01/04/37)
    // maturity doesn't occur on a business day

    Schedule fixedBondSchedule1(Date(4,January,2005),
                                Date(4,January,2037),
                                Period(Annual), bondCalendar,
                                Unadjusted, Unadjusted,
                                DateGeneration::Backward, false);
    boost::shared_ptr<Bond> fixedBond1(
                         new FixedRateBond(settlementDays, vars.faceAmount,
                                           fixedBondSchedule1,
                                           std::vector<Rate>(1, 0.04),
                                           ActualActual(ActualActual::ISDA),
                                           Following,
                                           100.0, Date(4,January,2005)));

    boost::shared_ptr<PricingEngine> bondEngine(
                            new DiscountingBondEngine(vars.termStructure));
    fixedBond1->setPricingEngine(bondEngine);

    Real fixedBondPrice1 = fixedBond1->cleanPrice();
    AssetSwap fixedBondAssetSwap1(payFixedRate,
                                  fixedBond1, fixedBondPrice1,
                                  vars.iborIndex, vars.spread,
                                  vars.termStructure,
                                  Schedule(),
                                  vars.iborIndex->dayCounter(),
                                  parAssetSwap);
    Real fixedBondAssetSwapPrice1 = fixedBondAssetSwap1.fairPrice();
    Real tolerance = 1.0e-13;
    Real error1 = std::fabs(fixedBondAssetSwapPrice1-fixedBondPrice1);

    if (error1>tolerance) {
        BOOST_ERROR("wrong zero spread asset swap price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's clean price:      " << fixedBondPrice1
                    << "\n  asset swap fair price: " << fixedBondAssetSwapPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error1
                    << "\n  tolerance:             " << tolerance);
    }

    // Fixed Underlying bond (Isin: IT0006527060 IBRD 5 02/05/19)
    // maturity occurs on a business day

    Schedule fixedBondSchedule2(Date(5,February,2005),
                                Date(5,February,2019),
                                Period(Annual), bondCalendar,
                                Unadjusted, Unadjusted,
                                DateGeneration::Backward, false);
    boost::shared_ptr<Bond> fixedBond2(
                         new FixedRateBond(settlementDays, vars.faceAmount,
                                           fixedBondSchedule2,
                                           std::vector<Rate>(1, 0.05),
                                           Thirty360(Thirty360::BondBasis),
                                           Following,
                                           100.0, Date(5,February,2005)));

    fixedBond2->setPricingEngine(bondEngine);

    Real fixedBondPrice2 = fixedBond2->cleanPrice();
    AssetSwap fixedBondAssetSwap2(payFixedRate,
                                  fixedBond2, fixedBondPrice2,
                                  vars.iborIndex, vars.spread,
                                  vars.termStructure,
                                  Schedule(),
                                  vars.iborIndex->dayCounter(),
                                  parAssetSwap);
    Real fixedBondAssetSwapPrice2 = fixedBondAssetSwap2.fairPrice();
    Real error2 = std::fabs(fixedBondAssetSwapPrice2-fixedBondPrice2);

    if (error2>tolerance) {
        BOOST_ERROR("wrong zero spread asset swap price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's clean price:      " << fixedBondPrice2
                    << "\n  asset swap fair price: " << fixedBondAssetSwapPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error2
                    << "\n  tolerance:             " << tolerance);
    }

    // FRN Underlying bond (Isin: IT0003543847 ISPIM 0 09/29/13)
    // maturity doesn't occur on a business day

    Schedule floatingBondSchedule1(Date(29,September,2003),
                                   Date(29,September,2013),
                                   Period(Semiannual), bondCalendar,
                                   Unadjusted, Unadjusted,
                                   DateGeneration::Backward, false);

    boost::shared_ptr<Bond> floatingBond1(
                      new FloatingRateBond(settlementDays, vars.faceAmount,
                                           floatingBondSchedule1,
                                           vars.iborIndex, Actual360(),
                                           Following, fixingDays,
                                           std::vector<Real>(1,1),
                                           std::vector<Spread>(1,0.0056),
                                           std::vector<Rate>(),
                                           std::vector<Rate>(),
                                           inArrears,
                                           100.0, Date(29,September,2003)));

    floatingBond1->setPricingEngine(bondEngine);

    setCouponPricer(floatingBond1->cashflows(), vars.pricer);
    vars.iborIndex->addFixing(Date(27,March,2007), 0.0402);
    Real floatingBondPrice1 = floatingBond1->cleanPrice();
    AssetSwap floatingBondAssetSwap1(payFixedRate,
                                     floatingBond1, floatingBondPrice1,
                                     vars.iborIndex, vars.spread,
                                     vars.termStructure,
                                     Schedule(),
                                     vars.iborIndex->dayCounter(),
                                     parAssetSwap);
    Real floatingBondAssetSwapPrice1 = floatingBondAssetSwap1.fairPrice();
    Real error3 = std::fabs(floatingBondAssetSwapPrice1-floatingBondPrice1);

    if (error3>tolerance) {
        BOOST_ERROR("wrong zero spread asset swap price for floater:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's clean price:      " << floatingBondPrice1
                    << "\n  asset swap fair price: "
                    << floatingBondAssetSwapPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error3
                    << "\n  tolerance:             " << tolerance);
    }

    // FRN Underlying bond (Isin: XS0090566539 COE 0 09/24/18)
    // maturity occurs on a business day

    Schedule floatingBondSchedule2(Date(24,September,2004),
                                   Date(24,September,2018),
                                   Period(Semiannual), bondCalendar,
                                   ModifiedFollowing, ModifiedFollowing,
                                   DateGeneration::Backward, false);
    boost::shared_ptr<Bond> floatingBond2(
                      new FloatingRateBond(settlementDays, vars.faceAmount,
                                           floatingBondSchedule2,
                                           vars.iborIndex, Actual360(),
                                           ModifiedFollowing, fixingDays,
                                           std::vector<Real>(1,1),
                                           std::vector<Spread>(1,0.0025),
                                           std::vector<Rate>(),
                                           std::vector<Rate>(),
                                           inArrears,
                                           100.0, Date(24,September,2004)));

    floatingBond2->setPricingEngine(bondEngine);

    setCouponPricer(floatingBond2->cashflows(), vars.pricer);
    vars.iborIndex->addFixing(Date(22,March,2007), 0.04013);
    Real currentCoupon=0.04013+0.0025;
    Real floatingCurrentCoupon= floatingBond2->currentCoupon();
    Real error4= std::fabs(floatingCurrentCoupon-currentCoupon);
    if (error4>tolerance) {
        BOOST_ERROR("wrong current coupon is returned for floater bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's calculated current coupon:      "
                    << currentCoupon
                    << "\n  current coupon asked to the bond: "
                    << floatingCurrentCoupon
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error4
                    << "\n  tolerance:             " << tolerance);
    }

    Real floatingBondPrice2 = floatingBond2->cleanPrice();
    AssetSwap floatingBondAssetSwap2(payFixedRate,
                                     floatingBond2, floatingBondPrice2,
                                     vars.iborIndex, vars.spread,
                                     vars.termStructure,
                                     Schedule(),
                                     vars.iborIndex->dayCounter(),
                                     parAssetSwap);
    Real floatingBondAssetSwapPrice2 = floatingBondAssetSwap2.fairPrice();
    Real error5 = std::fabs(floatingBondAssetSwapPrice2-floatingBondPrice2);

    if (error5>tolerance) {
        BOOST_ERROR("wrong zero spread asset swap price for floater:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's clean price:      "
                    << floatingBondPrice2
                    << "\n  asset swap fair price: "
                    << floatingBondAssetSwapPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error5
                    << "\n  tolerance:             " << tolerance);
    }

    // CMS Underlying bond (Isin: XS0228052402 CRDIT 0 8/22/20)
    // maturity doesn't occur on a business day

    Schedule cmsBondSchedule1(Date(22,August,2005),
                              Date(22,August,2020),
                              Period(Annual), bondCalendar,
                              Unadjusted, Unadjusted,
                              DateGeneration::Backward, false);
    boost::shared_ptr<Bond> cmsBond1(
                          new CmsRateBond(settlementDays, vars.faceAmount,
                                          cmsBondSchedule1,
                                          vars.swapIndex, Thirty360(),
                                          Following, fixingDays,
                                          std::vector<Real>(1,1.0),
                                          std::vector<Spread>(1,0.0),
                                          std::vector<Rate>(1,0.055),
                                          std::vector<Rate>(1,0.025),
                                          inArrears,
                                          100.0, Date(22,August,2005)));

    cmsBond1->setPricingEngine(bondEngine);

    setCouponPricer(cmsBond1->cashflows(), vars.cmspricer);
    vars.swapIndex->addFixing(Date(18,August,2006), 0.04158);
    Real cmsBondPrice1 = cmsBond1->cleanPrice();
    AssetSwap cmsBondAssetSwap1(payFixedRate,
                                cmsBond1, cmsBondPrice1,
                                vars.iborIndex, vars.spread,
                                vars.termStructure,
                                Schedule(),
                                vars.iborIndex->dayCounter(),
                                parAssetSwap);
    Real cmsBondAssetSwapPrice1 = cmsBondAssetSwap1.fairPrice();
    Real error6 = std::fabs(cmsBondAssetSwapPrice1-cmsBondPrice1);

    if (error6>tolerance) {
        BOOST_ERROR("wrong zero spread asset swap price for cms bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's clean price:      " << cmsBondPrice1
                    << "\n  asset swap fair price: " << cmsBondAssetSwapPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error6
                    << "\n  tolerance:             " << tolerance);
    }

     // CMS Underlying bond (Isin: XS0218766664 ISPIM 0 5/6/15)
     // maturity occurs on a business day

    Schedule cmsBondSchedule2(Date(06,May,2005),
                              Date(06,May,2015),
                              Period(Annual), bondCalendar,
                              Unadjusted, Unadjusted,
                              DateGeneration::Backward, false);
    boost::shared_ptr<Bond> cmsBond2(new
        CmsRateBond(settlementDays, vars.faceAmount, cmsBondSchedule2,
                    vars.swapIndex, Thirty360(),
                    Following, fixingDays,
                    std::vector<Real>(1,0.84), std::vector<Spread>(1,0.0),
                    std::vector<Rate>(), std::vector<Rate>(),
                    inArrears,
                    100.0, Date(06,May,2005)));

    cmsBond2->setPricingEngine(bondEngine);

    setCouponPricer(cmsBond2->cashflows(), vars.cmspricer);
    vars.swapIndex->addFixing(Date(04,May,2006), 0.04217);
    Real cmsBondPrice2 = cmsBond2->cleanPrice();
    AssetSwap cmsBondAssetSwap2(payFixedRate,
                                cmsBond2, cmsBondPrice2,
                                vars.iborIndex, vars.spread,
                                vars.termStructure,
                                Schedule(),
                                vars.iborIndex->dayCounter(),
                                parAssetSwap);
    Real cmsBondAssetSwapPrice2 = cmsBondAssetSwap2.fairPrice();
    Real error7 = std::fabs(cmsBondAssetSwapPrice2-cmsBondPrice2);

    if (error7>tolerance) {
        BOOST_ERROR("wrong zero spread asset swap price for cms bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's clean price:      " << cmsBondPrice2
                    << "\n  asset swap fair price: " << cmsBondAssetSwapPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error7
                    << "\n  tolerance:             " << tolerance);
    }

    // Zero Coupon bond (Isin: DE0004771662 IBRD 0 12/20/15)
    // maturity doesn't occur on a business day

    boost::shared_ptr<Bond> zeroCpnBond1(new
        ZeroCouponBond(settlementDays, bondCalendar, vars.faceAmount,
                       Date(20,December,2015),
                       Following,
                       100.0, Date(19,December,1985)));

    zeroCpnBond1->setPricingEngine(bondEngine);

    Real zeroCpnBondPrice1 = zeroCpnBond1->cleanPrice();
    AssetSwap zeroCpnAssetSwap1(payFixedRate,
                                zeroCpnBond1, zeroCpnBondPrice1,
                                vars.iborIndex, vars.spread,
                                vars.termStructure,
                                Schedule(),
                                vars.iborIndex->dayCounter(),
                                parAssetSwap);
    Real zeroCpnBondAssetSwapPrice1 = zeroCpnAssetSwap1.fairPrice();
    Real error8 = std::fabs(cmsBondAssetSwapPrice1-cmsBondPrice1);

    if (error8>tolerance) {
        BOOST_ERROR("wrong zero spread asset swap price for zero cpn bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's clean price:      " << zeroCpnBondPrice1
                    << "\n  asset swap fair price: "
                    << zeroCpnBondAssetSwapPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error8
                    << "\n  tolerance:             " << tolerance);
    }

    // Zero Coupon bond (Isin: IT0001200390 ISPIM 0 02/17/28)
    // maturity occurs on a business day

    boost::shared_ptr<Bond> zeroCpnBond2(new
        ZeroCouponBond(settlementDays, bondCalendar, vars.faceAmount,
                       Date(17,February,2028),
                       Following,
                       100.0, Date(17,February,1998)));

    zeroCpnBond2->setPricingEngine(bondEngine);

    Real zeroCpnBondPrice2 = zeroCpnBond2->cleanPrice();
    AssetSwap zeroCpnAssetSwap2(payFixedRate,
                                zeroCpnBond2, zeroCpnBondPrice2,
                                vars.iborIndex, vars.spread,
                                vars.termStructure,
                                Schedule(),
                                vars.iborIndex->dayCounter(),
                                parAssetSwap);
    Real zeroCpnBondAssetSwapPrice2 = zeroCpnAssetSwap2.fairPrice();
    Real error9 = std::fabs(cmsBondAssetSwapPrice2-cmsBondPrice2);

    if (error9>tolerance) {
        BOOST_ERROR("wrong zero spread asset swap price for zero cpn bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's clean price:      " << zeroCpnBondPrice2
                    << "\n  asset swap fair price: "
                    << zeroCpnBondAssetSwapPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error9
                    << "\n  tolerance:             " << tolerance);
    }
}


void AssetSwapTest::testMarketASWSpread() {

    BOOST_MESSAGE("Testing relationship between market asset swap"
                  " and par asset swap...");

    CommonVars vars;

    Calendar bondCalendar = TARGET();
    Natural settlementDays = 3;
    Natural fixingDays = 2;
    bool payFixedRate = true;
    bool parAssetSwap = true;
    bool mktAssetSwap = false;
    bool inArrears = false;

    // Fixed Underlying bond (Isin: DE0001135275 DBR 4 01/04/37)
    // maturity doesn't occur on a business day

    Schedule fixedBondSchedule1(Date(4,January,2005),
                                Date(4,January,2037),
                                Period(Annual), bondCalendar,
                                Unadjusted, Unadjusted,
                                DateGeneration::Backward, false);
    boost::shared_ptr<Bond> fixedBond1(new
        FixedRateBond(settlementDays, vars.faceAmount, fixedBondSchedule1,
                      std::vector<Rate>(1, 0.04),
                      ActualActual(ActualActual::ISDA), Following,
                      100.0, Date(4,January,2005)));

    boost::shared_ptr<PricingEngine> bondEngine(
                            new DiscountingBondEngine(vars.termStructure));
    fixedBond1->setPricingEngine(bondEngine);

    Real fixedBondMktPrice1 = 89.22 ; // market price observed on 7th June 2007
    Real fixedBondMktFullPrice1=fixedBondMktPrice1+fixedBond1->accruedAmount();
    AssetSwap fixedBondParAssetSwap1(payFixedRate,
                                     fixedBond1, fixedBondMktPrice1,
                                     vars.iborIndex, vars.spread,
                                     vars.termStructure,
                                     Schedule(),
                                     vars.iborIndex->dayCounter(),
                                     parAssetSwap);
    Real fixedBondParAssetSwapSpread1 = fixedBondParAssetSwap1.fairSpread();
    AssetSwap fixedBondMktAssetSwap1(payFixedRate,
                                     fixedBond1, fixedBondMktPrice1,
                                     vars.iborIndex, vars.spread,
                                     vars.termStructure,
                                     Schedule(),
                                     vars.iborIndex->dayCounter(),
                                     mktAssetSwap);
    Real fixedBondMktAssetSwapSpread1 = fixedBondMktAssetSwap1.fairSpread();

    Real tolerance = 1.0e-13;
    Real error1 =
        std::fabs(fixedBondMktAssetSwapSpread1-
                  100*fixedBondParAssetSwapSpread1/fixedBondMktFullPrice1);

    if (error1>tolerance) {
        BOOST_ERROR("wrong asset swap spreads for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: "
                    << fixedBondMktAssetSwapSpread1
                    << "\n  par asset swap spread: "
                    << fixedBondParAssetSwapSpread1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error1
                    << "\n  tolerance:             " << tolerance);
    }

    // Fixed Underlying bond (Isin: IT0006527060 IBRD 5 02/05/19)
    // maturity occurs on a business day

    Schedule fixedBondSchedule2(Date(5,February,2005),
                                Date(5,February,2019),
                                Period(Annual), bondCalendar,
                                Unadjusted, Unadjusted,
                                DateGeneration::Backward, false);
    boost::shared_ptr<Bond> fixedBond2(new
        FixedRateBond(settlementDays, vars.faceAmount, fixedBondSchedule2,
                      std::vector<Rate>(1, 0.05),
                      Thirty360(Thirty360::BondBasis), Following,
                      100.0, Date(5,February,2005)));

    fixedBond2->setPricingEngine(bondEngine);

    Real fixedBondMktPrice2 = 99.98 ; // market price observed on 7th June 2007
    Real fixedBondMktFullPrice2=fixedBondMktPrice2+fixedBond2->accruedAmount();
    AssetSwap fixedBondParAssetSwap2(payFixedRate,
                                     fixedBond2, fixedBondMktPrice2,
                                     vars.iborIndex, vars.spread,
                                     vars.termStructure,
                                     Schedule(),
                                     vars.iborIndex->dayCounter(),
                                     parAssetSwap);
    Real fixedBondParAssetSwapSpread2 = fixedBondParAssetSwap2.fairSpread();
    AssetSwap fixedBondMktAssetSwap2(payFixedRate,
                                     fixedBond2, fixedBondMktPrice2,
                                     vars.iborIndex, vars.spread,
                                     vars.termStructure,
                                     Schedule(),
                                     vars.iborIndex->dayCounter(),
                                     mktAssetSwap);
    Real fixedBondMktAssetSwapSpread2 = fixedBondMktAssetSwap2.fairSpread();
    Real error2 =
        std::fabs(fixedBondMktAssetSwapSpread2-
                  100*fixedBondParAssetSwapSpread2/fixedBondMktFullPrice2);

    if (error2>tolerance) {
        BOOST_ERROR("wrong asset swap spreads for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: "
                    << fixedBondMktAssetSwapSpread2
                    << "\n  par asset swap spread: "
                    << fixedBondParAssetSwapSpread2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error2
                    << "\n  tolerance:             " << tolerance);
    }

    // FRN Underlying bond (Isin: IT0003543847 ISPIM 0 09/29/13)
    // maturity doesn't occur on a business day

    Schedule floatingBondSchedule1(Date(29,September,2003),
                                   Date(29,September,2013),
                                   Period(Semiannual), bondCalendar,
                                   Unadjusted, Unadjusted,
                                   DateGeneration::Backward, false);

    boost::shared_ptr<Bond> floatingBond1(new
        FloatingRateBond(settlementDays, vars.faceAmount,
                         floatingBondSchedule1,
                         vars.iborIndex, Actual360(),
                         Following, fixingDays,
                         std::vector<Real>(1,1), std::vector<Spread>(1,0.0056),
                         std::vector<Rate>(), std::vector<Rate>(),
                         inArrears,
                         100.0, Date(29,September,2003)));

    floatingBond1->setPricingEngine(bondEngine);

    setCouponPricer(floatingBond1->cashflows(), vars.pricer);
    vars.iborIndex->addFixing(Date(27,March,2007), 0.0402);
    // market price observed on 7th June 2007
    Real floatingBondMktPrice1 = 101.64 ;
    Real floatingBondMktFullPrice1 =
        floatingBondMktPrice1+floatingBond1->accruedAmount();
    AssetSwap floatingBondParAssetSwap1(payFixedRate,
                                        floatingBond1, floatingBondMktPrice1,
                                        vars.iborIndex, vars.spread,
                                        vars.termStructure,
                                        Schedule(),
                                        vars.iborIndex->dayCounter(),
                                        parAssetSwap);
    Real floatingBondParAssetSwapSpread1 =
        floatingBondParAssetSwap1.fairSpread();
    AssetSwap floatingBondMktAssetSwap1(payFixedRate,
                                        floatingBond1, floatingBondMktPrice1,
                                        vars.iborIndex, vars.spread,
                                        vars.termStructure,
                                        Schedule(),
                                        vars.iborIndex->dayCounter(),
                                        mktAssetSwap);
    Real floatingBondMktAssetSwapSpread1 =
        floatingBondMktAssetSwap1.fairSpread();
    Real error3 =
        std::fabs(floatingBondMktAssetSwapSpread1-
                  100*floatingBondParAssetSwapSpread1/floatingBondMktFullPrice1);

    if (error3>tolerance) {
        BOOST_ERROR("wrong asset swap spreads for floating bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: "
                    << floatingBondMktAssetSwapSpread1
                    << "\n  par asset swap spread: "
                    << floatingBondParAssetSwapSpread1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error3
                    << "\n  tolerance:             " << tolerance);
    }

    // FRN Underlying bond (Isin: XS0090566539 COE 0 09/24/18)
    // maturity occurs on a business day

    Schedule floatingBondSchedule2(Date(24,September,2004),
                                   Date(24,September,2018),
                                   Period(Semiannual), bondCalendar,
                                   ModifiedFollowing, ModifiedFollowing,
                                   DateGeneration::Backward, false);
    boost::shared_ptr<Bond> floatingBond2(new
        FloatingRateBond(settlementDays, vars.faceAmount,
                         floatingBondSchedule2,
                         vars.iborIndex, Actual360(),
                         ModifiedFollowing, fixingDays,
                         std::vector<Real>(1,1), std::vector<Spread>(1,0.0025),
                         std::vector<Rate>(), std::vector<Rate>(),
                         inArrears,
                         100.0, Date(24,September,2004)));

    floatingBond2->setPricingEngine(bondEngine);

    setCouponPricer(floatingBond2->cashflows(), vars.pricer);
    vars.iborIndex->addFixing(Date(22,March,2007), 0.04013);
    // market price observed on 7th June 2007
    Real floatingBondMktPrice2 = 101.248 ;
    Real floatingBondMktFullPrice2 =
        floatingBondMktPrice2+floatingBond2->accruedAmount();
    AssetSwap floatingBondParAssetSwap2(payFixedRate,
                                        floatingBond2, floatingBondMktPrice2,
                                        vars.iborIndex, vars.spread,
                                        vars.termStructure,
                                        Schedule(),
                                        vars.iborIndex->dayCounter(),
                                        parAssetSwap);
    Spread floatingBondParAssetSwapSpread2 =
        floatingBondParAssetSwap2.fairSpread();
    AssetSwap floatingBondMktAssetSwap2(payFixedRate,
                                        floatingBond2, floatingBondMktPrice2,
                                        vars.iborIndex, vars.spread,
                                        vars.termStructure,
                                        Schedule(),
                                        vars.iborIndex->dayCounter(),
                                        mktAssetSwap);
    Real floatingBondMktAssetSwapSpread2 =
        floatingBondMktAssetSwap2.fairSpread();
    Real error4 =
        std::fabs(floatingBondMktAssetSwapSpread2-
                  100*floatingBondParAssetSwapSpread2/floatingBondMktFullPrice2);

    if (error4>tolerance) {
        BOOST_ERROR("wrong asset swap spreads for floating bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: "
                    << floatingBondMktAssetSwapSpread2
                    << "\n  par asset swap spread: "
                    << floatingBondParAssetSwapSpread2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error4
                    << "\n  tolerance:             " << tolerance);
    }

    // CMS Underlying bond (Isin: XS0228052402 CRDIT 0 8/22/20)
    // maturity doesn't occur on a business day

    Schedule cmsBondSchedule1(Date(22,August,2005),
                              Date(22,August,2020),
                              Period(Annual), bondCalendar,
                              Unadjusted, Unadjusted,
                              DateGeneration::Backward, false);
    boost::shared_ptr<Bond> cmsBond1(new
        CmsRateBond(settlementDays, vars.faceAmount, cmsBondSchedule1,
                    vars.swapIndex, Thirty360(),
                    Following, fixingDays,
                    std::vector<Real>(1,1.0), std::vector<Spread>(1,0.0),
                    std::vector<Rate>(1,0.055), std::vector<Rate>(1,0.025),
                    inArrears,
                    100.0, Date(22,August,2005)));

    cmsBond1->setPricingEngine(bondEngine);

    setCouponPricer(cmsBond1->cashflows(), vars.cmspricer);
    vars.swapIndex->addFixing(Date(18,August,2006), 0.04158);
    Real cmsBondMktPrice1 = 88.45 ; // market price observed on 7th June 2007
    Real cmsBondMktFullPrice1 = cmsBondMktPrice1+cmsBond1->accruedAmount();
    AssetSwap cmsBondParAssetSwap1(payFixedRate,
                                   cmsBond1, cmsBondMktPrice1,
                                   vars.iborIndex, vars.spread,
                                   vars.termStructure,
                                   Schedule(),
                                   vars.iborIndex->dayCounter(),
                                   parAssetSwap);
    Real cmsBondParAssetSwapSpread1 = cmsBondParAssetSwap1.fairSpread();
    AssetSwap cmsBondMktAssetSwap1(payFixedRate,
                                   cmsBond1, cmsBondMktPrice1,
                                   vars.iborIndex, vars.spread,
                                   vars.termStructure,
                                   Schedule(),
                                   vars.iborIndex->dayCounter(),
                                   mktAssetSwap);
    Real cmsBondMktAssetSwapSpread1 = cmsBondMktAssetSwap1.fairSpread();
    Real error5 =
        std::fabs(cmsBondMktAssetSwapSpread1-
                  100*cmsBondParAssetSwapSpread1/cmsBondMktFullPrice1);

    if (error5>tolerance) {
        BOOST_ERROR("wrong asset swap spreads for cms bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: "
                    << cmsBondMktAssetSwapSpread1
                    << "\n  par asset swap spread: "
                    << cmsBondParAssetSwapSpread1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error5
                    << "\n  tolerance:             " << tolerance);
    }

     // CMS Underlying bond (Isin: XS0218766664 ISPIM 0 5/6/15)
     // maturity occurs on a business day

    Schedule cmsBondSchedule2(Date(06,May,2005),
                              Date(06,May,2015),
                              Period(Annual), bondCalendar,
                              Unadjusted, Unadjusted,
                              DateGeneration::Backward, false);
    boost::shared_ptr<Bond> cmsBond2(new
        CmsRateBond(settlementDays, vars.faceAmount, cmsBondSchedule2,
                    vars.swapIndex, Thirty360(),
                    Following, fixingDays,
                    std::vector<Real>(1,0.84), std::vector<Spread>(1,0.0),
                    std::vector<Rate>(), std::vector<Rate>(),
                    inArrears,
                    100.0, Date(06,May,2005)));

    cmsBond2->setPricingEngine(bondEngine);

    setCouponPricer(cmsBond2->cashflows(), vars.cmspricer);
    vars.swapIndex->addFixing(Date(04,May,2006), 0.04217);
    Real cmsBondMktPrice2 = 94.08 ; // market price observed on 7th June 2007
    Real cmsBondMktFullPrice2 = cmsBondMktPrice2+cmsBond2->accruedAmount();
    AssetSwap cmsBondParAssetSwap2(payFixedRate,
                                   cmsBond2, cmsBondMktPrice2,
                                   vars.iborIndex, vars.spread,
                                   vars.termStructure,
                                   Schedule(),
                                   vars.iborIndex->dayCounter(),
                                   parAssetSwap);
    Spread cmsBondParAssetSwapSpread2 = cmsBondParAssetSwap2.fairSpread();
    AssetSwap cmsBondMktAssetSwap2(payFixedRate,
                                   cmsBond2, cmsBondMktPrice2,
                                   vars.iborIndex, vars.spread,
                                   vars.termStructure,
                                   Schedule(),
                                   vars.iborIndex->dayCounter(),
                                   mktAssetSwap);
    Real cmsBondMktAssetSwapSpread2 = cmsBondMktAssetSwap2.fairSpread();
    Real error6 =
        std::fabs(cmsBondMktAssetSwapSpread2-
                  100*cmsBondParAssetSwapSpread2/cmsBondMktFullPrice2);

    if (error6>tolerance) {
        BOOST_ERROR("wrong asset swap spreads for cms bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: "
                    << cmsBondMktAssetSwapSpread2
                    << "\n  par asset swap spread: "
                    << cmsBondParAssetSwapSpread2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error6
                    << "\n  tolerance:             " << tolerance);
    }

    // Zero Coupon bond (Isin: DE0004771662 IBRD 0 12/20/15)
    // maturity doesn't occur on a business day

    boost::shared_ptr<Bond> zeroCpnBond1(new
        ZeroCouponBond(settlementDays, bondCalendar, vars.faceAmount,
                       Date(20,December,2015),
                       Following,
                       100.0, Date(19,December,1985)));

    zeroCpnBond1->setPricingEngine(bondEngine);

    // market price observed on 12th June 2007
    Real zeroCpnBondMktPrice1 = 70.436 ;
    Real zeroCpnBondMktFullPrice1 =
        zeroCpnBondMktPrice1+zeroCpnBond1->accruedAmount();
    AssetSwap zeroCpnBondParAssetSwap1(payFixedRate,zeroCpnBond1,
                                       zeroCpnBondMktPrice1, vars.iborIndex,
                                       vars.spread, vars.termStructure,
                                       Schedule(),
                                       vars.iborIndex->dayCounter(),
                                       parAssetSwap);
    Real zeroCpnBondParAssetSwapSpread1 = zeroCpnBondParAssetSwap1.fairSpread();
    AssetSwap zeroCpnBondMktAssetSwap1(payFixedRate,zeroCpnBond1,
                                       zeroCpnBondMktPrice1,vars.iborIndex,
                                       vars.spread, vars.termStructure,
                                       Schedule(),
                                       vars.iborIndex->dayCounter(),
                                       mktAssetSwap);
    Real zeroCpnBondMktAssetSwapSpread1 = zeroCpnBondMktAssetSwap1.fairSpread();
    Real error7 =
        std::fabs(zeroCpnBondMktAssetSwapSpread1-
                  100*zeroCpnBondParAssetSwapSpread1/zeroCpnBondMktFullPrice1);

    if (error7>tolerance) {
        BOOST_ERROR("wrong asset swap spreads for zero cpn bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: "
                    << zeroCpnBondMktAssetSwapSpread1
                    << "\n  par asset swap spread: "
                    << zeroCpnBondParAssetSwapSpread1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error7
                    << "\n  tolerance:             " << tolerance);
    }

    // Zero Coupon bond (Isin: IT0001200390 ISPIM 0 02/17/28)
    // maturity occurs on a business day

    boost::shared_ptr<Bond> zeroCpnBond2(new
        ZeroCouponBond(settlementDays, bondCalendar, vars.faceAmount,
                       Date(17,February,2028),
                       Following,
                       100.0, Date(17,February,1998)));

    zeroCpnBond2->setPricingEngine(bondEngine);

    // Real zeroCpnBondPrice2 = zeroCpnBond2->cleanPrice();

    // market price observed on 12th June 2007
    Real zeroCpnBondMktPrice2 = 35.160 ;
    Real zeroCpnBondMktFullPrice2 =
        zeroCpnBondMktPrice2+zeroCpnBond2->accruedAmount();
    AssetSwap zeroCpnBondParAssetSwap2(payFixedRate,zeroCpnBond2,
                                       zeroCpnBondMktPrice2, vars.iborIndex,
                                       vars.spread, vars.termStructure,
                                       Schedule(),
                                       vars.iborIndex->dayCounter(),
                                       parAssetSwap);
    Real zeroCpnBondParAssetSwapSpread2 = zeroCpnBondParAssetSwap2.fairSpread();
    AssetSwap zeroCpnBondMktAssetSwap2(payFixedRate,zeroCpnBond2,
                                       zeroCpnBondMktPrice2,vars.iborIndex,
                                       vars.spread, vars.termStructure,
                                       Schedule(),
                                       vars.iborIndex->dayCounter(),
                                       mktAssetSwap);
    Real zeroCpnBondMktAssetSwapSpread2 = zeroCpnBondMktAssetSwap2.fairSpread();
    Real error8 =
        std::fabs(zeroCpnBondMktAssetSwapSpread2-
                  100*zeroCpnBondParAssetSwapSpread2/zeroCpnBondMktFullPrice2);

    if (error8>tolerance) {
        BOOST_ERROR("wrong asset swap spreads for zero cpn bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: "
                    << zeroCpnBondMktAssetSwapSpread2
                    << "\n  par asset swap spread: "
                    << zeroCpnBondParAssetSwapSpread2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error8
                    << "\n  tolerance:             " << tolerance);
    }
}


void AssetSwapTest::testZSpread() {

    BOOST_MESSAGE("Testing clean and dirty price with null Z-spread "
                  "against theoretical prices...");

    CommonVars vars;

    Calendar bondCalendar = TARGET();
    Natural settlementDays = 3;
    Natural fixingDays = 2;
    bool inArrears = false;

    // Fixed bond (Isin: DE0001135275 DBR 4 01/04/37)
    // maturity doesn't occur on a business day

    Schedule fixedBondSchedule1(Date(4,January,2005),
                                Date(4,January,2037),
                                Period(Annual), bondCalendar,
                                Unadjusted, Unadjusted,
                                DateGeneration::Backward, false);
    boost::shared_ptr<Bond> fixedBond1(new
        FixedRateBond(settlementDays, vars.faceAmount, fixedBondSchedule1,
                      std::vector<Rate>(1, 0.04),
                      ActualActual(ActualActual::ISDA), Following,
                      100.0, Date(4,January,2005)));

    boost::shared_ptr<PricingEngine> bondEngine(
                            new DiscountingBondEngine(vars.termStructure));
    fixedBond1->setPricingEngine(bondEngine);

    Real fixedBondImpliedValue1 = fixedBond1->cleanPrice();
    Date fixedBondSettlementDate1= fixedBond1->settlementDate();
    // standard market conventions:
    // bond's frequency + coumpounding and daycounter of the YC...
    Real fixedBondCleanPrice1= fixedBond1->cleanPriceFromZSpread(vars.spread,
         Actual365Fixed(), vars.compounding, Annual,
         fixedBondSettlementDate1);
    Real fixedBondDirtyPrice1= fixedBond1->dirtyPriceFromZSpread(vars.spread,
         Actual365Fixed(), vars.compounding, Annual,
         fixedBondSettlementDate1);
    Real tolerance = 1.0e-13;
    Real error1 = std::fabs(fixedBondImpliedValue1-fixedBondCleanPrice1);
    if (error1>tolerance) {
        BOOST_ERROR("wrong clean price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: "
                    << fixedBondImpliedValue1
                    << "\n  par asset swap spread: " << fixedBondCleanPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error1
                    << "\n  tolerance:             " << tolerance);
    }
    Real fixedBondImpliedDirty1 =
        fixedBondImpliedValue1+fixedBond1->accruedAmount();
    Real error2 = std::fabs(fixedBondImpliedDirty1-fixedBondDirtyPrice1);
    if (error2>tolerance) {
        BOOST_ERROR("wrong dirty price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " <<
                    fixedBondImpliedDirty1
                    << "\n  par asset swap spread: " << fixedBondDirtyPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error2
                    << "\n  tolerance:             " << tolerance);
    }

    // Fixed bond (Isin: IT0006527060 IBRD 5 02/05/19)
    // maturity occurs on a business day

    Schedule fixedBondSchedule2(Date(5,February,2005),
                                Date(5,February,2019),
                                Period(Annual), bondCalendar,
                                Unadjusted, Unadjusted,
                                DateGeneration::Backward, false);
    boost::shared_ptr<Bond> fixedBond2(new
        FixedRateBond(settlementDays, vars.faceAmount, fixedBondSchedule2,
                      std::vector<Rate>(1, 0.05),
                      Thirty360(Thirty360::BondBasis), Following,
                      100.0, Date(5,February,2005)));

    fixedBond2->setPricingEngine(bondEngine);

    Real fixedBondImpliedValue2 = fixedBond2->cleanPrice();
    Date fixedBondSettlementDate2= fixedBond2->settlementDate();
    // standard market conventions:
    // bond's frequency + coumpounding and daycounter of the YieldCurve
    Real fixedBondCleanPrice2= fixedBond2->cleanPriceFromZSpread(vars.spread,
         Actual365Fixed(), vars.compounding, Annual,
         fixedBondSettlementDate2);
    Real fixedBondDirtyPrice2= fixedBond2->dirtyPriceFromZSpread(vars.spread,
         Actual365Fixed(), vars.compounding, Annual, //FIXME ??
         fixedBondSettlementDate2);
    Real error3 = std::fabs(fixedBondImpliedValue2-fixedBondCleanPrice2);
    if (error3>tolerance) {
        BOOST_ERROR("wrong clean price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " <<
                    fixedBondImpliedValue2
                    << "\n  par asset swap spread: " << fixedBondCleanPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error3
                    << "\n  tolerance:             " << tolerance);
    }
    Real fixedBondImpliedDirty2 =
        fixedBondImpliedValue2+fixedBond2->accruedAmount();
    Real error4 = std::fabs(fixedBondImpliedDirty2-fixedBondDirtyPrice2);
    if (error4>tolerance) {
        BOOST_ERROR("wrong dirty price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " <<
                    fixedBondImpliedDirty2
                    << "\n  par asset swap spread: " << fixedBondDirtyPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error4
                    << "\n  tolerance:             " << tolerance);
    }

    // FRN bond (Isin: IT0003543847 ISPIM 0 09/29/13)
    // maturity doesn't occur on a business day

    Schedule floatingBondSchedule1(Date(29,September,2003),
                                   Date(29,September,2013),
                                   Period(Semiannual), bondCalendar,
                                   Unadjusted, Unadjusted,
                                   DateGeneration::Backward, false);

    boost::shared_ptr<Bond> floatingBond1(new
        FloatingRateBond(settlementDays, vars.faceAmount,
                         floatingBondSchedule1,
                         vars.iborIndex, Actual360(),
                         Following, fixingDays,
                         std::vector<Real>(1,1), std::vector<Spread>(1,0.0056),
                         std::vector<Rate>(), std::vector<Rate>(),
                         inArrears,
                         100.0, Date(29,September,2003)));

    floatingBond1->setPricingEngine(bondEngine);

    setCouponPricer(floatingBond1->cashflows(), vars.pricer);
    vars.iborIndex->addFixing(Date(27,March,2007), 0.0402);
    Real floatingBondImpliedValue1 = floatingBond1->cleanPrice();
    Date floatingBondSettlementDate1= floatingBond1->settlementDate();
    // standard market conventions:
    // bond's frequency + coumpounding and daycounter of the YieldCurve
    Real floatingBondCleanPrice1= floatingBond1->cleanPriceFromZSpread(
        vars.spread, Actual365Fixed(), vars.compounding, Semiannual,
        fixedBondSettlementDate1);
    Real floatingBondDirtyPrice1= floatingBond1->dirtyPriceFromZSpread(
        vars.spread, Actual365Fixed(), vars.compounding, Semiannual,
        floatingBondSettlementDate1);
    Real error5 = std::fabs(floatingBondImpliedValue1-floatingBondCleanPrice1);
    if (error5>tolerance) {
        BOOST_ERROR("wrong clean price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " <<
                    floatingBondImpliedValue1
                    << "\n  par asset swap spread: " << floatingBondCleanPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error5
                    << "\n  tolerance:             " << tolerance);
    }
    Real floatingBondImpliedDirty1 = floatingBondImpliedValue1+
                                     floatingBond1->accruedAmount();
    Real error6 = std::fabs(floatingBondImpliedDirty1-floatingBondDirtyPrice1);
    if (error6>tolerance) {
        BOOST_ERROR("wrong dirty price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " <<
                    floatingBondImpliedDirty1
                    << "\n  par asset swap spread: " << floatingBondDirtyPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error6
                    << "\n  tolerance:             " << tolerance);
    }

    // FRN bond (Isin: XS0090566539 COE 0 09/24/18)
    // maturity occurs on a business day

    Schedule floatingBondSchedule2(Date(24,September,2004),
                                   Date(24,September,2018),
                                   Period(Semiannual), bondCalendar,
                                   ModifiedFollowing, ModifiedFollowing,
                                   DateGeneration::Backward, false);
    boost::shared_ptr<Bond> floatingBond2(new
        FloatingRateBond(settlementDays, vars.faceAmount,
                         floatingBondSchedule2,
                         vars.iborIndex, Actual360(),
                         ModifiedFollowing, fixingDays,
                         std::vector<Real>(1,1), std::vector<Spread>(1,0.0025),
                         std::vector<Rate>(), std::vector<Rate>(),
                         inArrears,
                         100.0, Date(24,September,2004)));

    floatingBond2->setPricingEngine(bondEngine);

    setCouponPricer(floatingBond2->cashflows(), vars.pricer);
    vars.iborIndex->addFixing(Date(22,March,2007), 0.04013);
    Real floatingBondImpliedValue2 = floatingBond2->cleanPrice();
    Date floatingBondSettlementDate2= floatingBond2->settlementDate();
    // standard market conventions:
    // bond's frequency + coumpounding and daycounter of the YieldCurve
    Real floatingBondCleanPrice2= floatingBond2->cleanPriceFromZSpread(
        vars.spread, Actual365Fixed(), vars.compounding, Semiannual,
        fixedBondSettlementDate1);
    Real floatingBondDirtyPrice2= floatingBond2->dirtyPriceFromZSpread(
        vars.spread, Actual365Fixed(), vars.compounding, Semiannual,
        floatingBondSettlementDate2);
    Real error7 = std::fabs(floatingBondImpliedValue2-floatingBondCleanPrice2);
    if (error7>tolerance) {
        BOOST_ERROR("wrong clean price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " <<
                    floatingBondImpliedValue2
                    << "\n  par asset swap spread: " << floatingBondCleanPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error7
                    << "\n  tolerance:             " << tolerance);
    }
    Real floatingBondImpliedDirty2 = floatingBondImpliedValue2+
                                     floatingBond2->accruedAmount();
    Real error8 = std::fabs(floatingBondImpliedDirty2-floatingBondDirtyPrice2);
    if (error8>tolerance) {
        BOOST_ERROR("wrong dirty price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " <<
                    floatingBondImpliedDirty2
                    << "\n  par asset swap spread: " << floatingBondDirtyPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error8
                    << "\n  tolerance:             " << tolerance);
    }


    //// CMS bond (Isin: XS0228052402 CRDIT 0 8/22/20)
    //// maturity doesn't occur on a business day

    Schedule cmsBondSchedule1(Date(22,August,2005),
                              Date(22,August,2020),
                              Period(Annual), bondCalendar,
                              Unadjusted, Unadjusted,
                              DateGeneration::Backward, false);
    boost::shared_ptr<Bond> cmsBond1(new
        CmsRateBond(settlementDays, vars.faceAmount, cmsBondSchedule1,
                    vars.swapIndex, Thirty360(),
                    Following, fixingDays,
                    std::vector<Real>(1,1.0), std::vector<Spread>(1,0.0),
                    std::vector<Rate>(1,0.055), std::vector<Rate>(1,0.025),
                    inArrears,
                    100.0, Date(22,August,2005)));

    cmsBond1->setPricingEngine(bondEngine);

    setCouponPricer(cmsBond1->cashflows(), vars.cmspricer);
    vars.swapIndex->addFixing(Date(18,August,2006), 0.04158);
    Real cmsBondImpliedValue1 = cmsBond1->cleanPrice();
    Date cmsBondSettlementDate1= cmsBond1->settlementDate();
    // standard market conventions:
    // bond's frequency + coumpounding and daycounter of the YieldCurve
    Real cmsBondCleanPrice1= cmsBond1->cleanPriceFromZSpread(vars.spread,
        Actual365Fixed(), vars.compounding, Annual,
        cmsBondSettlementDate1);
    Real cmsBondDirtyPrice1= cmsBond1->dirtyPriceFromZSpread(vars.spread,
        Actual365Fixed(), vars.compounding, Annual,
        fixedBondSettlementDate1);
    Real error9 = std::fabs(cmsBondImpliedValue1-cmsBondCleanPrice1);
    if (error9>tolerance) {
        BOOST_ERROR("wrong clean price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " << cmsBondImpliedValue1
                    << "\n  par asset swap spread: " << cmsBondCleanPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error9
                    << "\n  tolerance:             " << tolerance);
    }
    Real cmsBondImpliedDirty1 = cmsBondImpliedValue1+cmsBond1->accruedAmount();
    Real error10 = std::fabs(cmsBondImpliedDirty1-cmsBondDirtyPrice1);
    if (error10>tolerance) {
        BOOST_ERROR("wrong dirty price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " << cmsBondImpliedDirty1
                    << "\n  par asset swap spread: " << cmsBondDirtyPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error10
                    << "\n  tolerance:             " << tolerance);
    }

     // CMS bond (Isin: XS0218766664 ISPIM 0 5/6/15)
     // maturity occurs on a business day

    Schedule cmsBondSchedule2(Date(06,May,2005),
                              Date(06,May,2015),
                              Period(Annual), bondCalendar,
                              Unadjusted, Unadjusted,
                              DateGeneration::Backward, false);
    boost::shared_ptr<Bond> cmsBond2(new
        CmsRateBond(settlementDays, vars.faceAmount, cmsBondSchedule2,
                    vars.swapIndex, Thirty360(),
                    Following, fixingDays,
                    std::vector<Real>(1,0.84), std::vector<Spread>(1,0.0),
                    std::vector<Rate>(), std::vector<Rate>(),
                    inArrears,
                    100.0, Date(06,May,2005)));

    cmsBond2->setPricingEngine(bondEngine);

    setCouponPricer(cmsBond2->cashflows(), vars.cmspricer);
    vars.swapIndex->addFixing(Date(04,May,2006), 0.04217);
    Real cmsBondImpliedValue2 = cmsBond2->cleanPrice();
    Date cmsBondSettlementDate2= cmsBond2->settlementDate();
    // standard market conventions:
    // bond's frequency + coumpounding and daycounter of the YieldCurve
    Real cmsBondCleanPrice2= cmsBond2->cleanPriceFromZSpread(vars.spread,
         Actual365Fixed(), vars.compounding, Annual,
         cmsBondSettlementDate2);
    Real cmsBondDirtyPrice2= cmsBond2->dirtyPriceFromZSpread(vars.spread,
         Actual365Fixed(), vars.compounding, Annual,
         fixedBondSettlementDate2);
    Real error11 = std::fabs(cmsBondImpliedValue2-cmsBondCleanPrice2);
    if (error11>tolerance) {
        BOOST_ERROR("wrong clean price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " << cmsBondImpliedValue2
                    << "\n  par asset swap spread: " << cmsBondCleanPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error11
                    << "\n  tolerance:             " << tolerance);
    }
    Real cmsBondImpliedDirty2 = cmsBondImpliedValue2+cmsBond2->accruedAmount();
    Real error12 = std::fabs(cmsBondImpliedDirty2-cmsBondDirtyPrice2);
    if (error12>tolerance) {
        BOOST_ERROR("wrong dirty price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " << cmsBondImpliedDirty2
                    << "\n  par asset swap spread: " << cmsBondDirtyPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error12
                    << "\n  tolerance:             " << tolerance);
    }

    // Zero-Coupon bond (Isin: DE0004771662 IBRD 0 12/20/15)
    // maturity doesn't occur on a business day

    boost::shared_ptr<Bond> zeroCpnBond1(new
        ZeroCouponBond(settlementDays, bondCalendar, vars.faceAmount,
                       Date(20,December,2015),
                       Following,
                       100.0, Date(19,December,1985)));

    zeroCpnBond1->setPricingEngine(bondEngine);

    Real zeroCpnBondImpliedValue1 = zeroCpnBond1->cleanPrice();
    Date zeroCpnBondSettlementDate1= zeroCpnBond1->settlementDate();
    // standard market conventions:
    // bond's frequency + coumpounding and daycounter of the YieldCurve
    Real zeroCpnBondCleanPrice1 =
        zeroCpnBond1->cleanPriceFromZSpread(vars.spread,
                                            Actual365Fixed(),
                                            vars.compounding, Annual,
                                            zeroCpnBondSettlementDate1);
    Real zeroCpnBondDirtyPrice1 =
        zeroCpnBond1->dirtyPriceFromZSpread(vars.spread,
                                            Actual365Fixed(),
                                            vars.compounding, Annual,
                                            zeroCpnBondSettlementDate1);
    Real error13 = std::fabs(zeroCpnBondImpliedValue1-zeroCpnBondCleanPrice1);
    if (error13>tolerance) {
        BOOST_ERROR("wrong clean price for zero coupon bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  zero cpn implied value: " <<
                    zeroCpnBondImpliedValue1
                    << "\n  zero cpn price: " << zeroCpnBondCleanPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error13
                    << "\n  tolerance:             " << tolerance);
    }
    Real zeroCpnBondImpliedDirty1 = zeroCpnBondImpliedValue1+
                                    zeroCpnBond1->accruedAmount();
    Real error14 = std::fabs(zeroCpnBondImpliedDirty1-zeroCpnBondDirtyPrice1);
    if (error14>tolerance) {
        BOOST_ERROR("wrong dirty price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's implied dirty price: " <<
                    zeroCpnBondImpliedDirty1
                    << "\n  bond's full price: " << zeroCpnBondDirtyPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error14
                    << "\n  tolerance:             " << tolerance);
    }
    // Zero Coupon bond (Isin: IT0001200390 ISPIM 0 02/17/28)
    // maturity doesn't occur on a business day

    boost::shared_ptr<Bond> zeroCpnBond2(new
        ZeroCouponBond(settlementDays, bondCalendar, vars.faceAmount,
                       Date(17,February,2028),
                       Following,
                       100.0, Date(17,February,1998)));

    zeroCpnBond2->setPricingEngine(bondEngine);

    Real zeroCpnBondImpliedValue2 = zeroCpnBond2->cleanPrice();
    Date zeroCpnBondSettlementDate2= zeroCpnBond2->settlementDate();
    // standard market conventions:
    // bond's frequency + coumpounding and daycounter of the YieldCurve
    Real zeroCpnBondCleanPrice2 =
        zeroCpnBond2->cleanPriceFromZSpread(vars.spread,
                                            Actual365Fixed(),
                                            vars.compounding, Annual,
                                            zeroCpnBondSettlementDate2);
    Real zeroCpnBondDirtyPrice2 =
        zeroCpnBond2->dirtyPriceFromZSpread(vars.spread,
                                            Actual365Fixed(),
                                            vars.compounding, Annual,
                                            zeroCpnBondSettlementDate2);
    Real error15 = std::fabs(zeroCpnBondImpliedValue2-zeroCpnBondCleanPrice2);
    if (error15>tolerance) {
        BOOST_ERROR("wrong clean price for zero coupon bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  zero cpn implied value: " <<
                    zeroCpnBondImpliedValue2
                    << "\n  zero cpn price: " << zeroCpnBondCleanPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error15
                    << "\n  tolerance:             " << tolerance);
    }
    Real zeroCpnBondImpliedDirty2 = zeroCpnBondImpliedValue2+
                                    zeroCpnBond2->accruedAmount();
    Real error16 = std::fabs(zeroCpnBondImpliedDirty2-zeroCpnBondDirtyPrice2);
    if (error16>tolerance) {
        BOOST_ERROR("wrong dirty price for zero coupon bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's implied dirty price: " <<
                    zeroCpnBondImpliedDirty2
                    << "\n  bond's full price: " << zeroCpnBondDirtyPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error16
                    << "\n  tolerance:             " << tolerance);
    }
}


void AssetSwapTest::testGenericBondImplied() {

    BOOST_MESSAGE("Testing generic bond implied value against"
                  " asset-swap fair price with null spread...");

    CommonVars vars;

    Calendar bondCalendar = TARGET();
    Natural settlementDays = 3;
    Natural fixingDays = 2;
    bool payFixedRate = true;
    bool parAssetSwap = true;
    bool inArrears = false;

    // Fixed Underlying bond (Isin: DE0001135275 DBR 4 01/04/37)
    // maturity doesn't occur on a business day
    Date fixedBondStartDate1 = Date(4,January,2005);
    Date fixedBondMaturityDate1 = Date(4,January,2037);
    Schedule fixedBondSchedule1(fixedBondStartDate1,
                                fixedBondMaturityDate1,
                                Period(Annual), bondCalendar,
                                Unadjusted, Unadjusted,
                                DateGeneration::Backward, false);
    Leg fixedBondLeg1 = FixedRateLeg(fixedBondSchedule1,
                                     ActualActual(ActualActual::ISDA))
        .withNotionals(vars.faceAmount)
        .withCouponRates(0.04);
    Date fixedbondRedemption1 = bondCalendar.adjust(fixedBondMaturityDate1,
                                                    Following);
    fixedBondLeg1.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, fixedbondRedemption1)));
    boost::shared_ptr<Bond> fixedBond1(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             fixedBondMaturityDate1, fixedBondStartDate1,
             fixedBondLeg1));
    boost::shared_ptr<PricingEngine> bondEngine(new
        DiscountingBondEngine(vars.termStructure));
    fixedBond1->setPricingEngine(bondEngine);

    Real fixedBondPrice1 = fixedBond1->cleanPrice();
    AssetSwap fixedBondAssetSwap1(payFixedRate,
                                  fixedBond1, fixedBondPrice1,
                                  vars.iborIndex, vars.spread,
                                  vars.termStructure,
                                  Schedule(),
                                  vars.iborIndex->dayCounter(),
                                  parAssetSwap);
    Real fixedBondAssetSwapPrice1 = fixedBondAssetSwap1.fairPrice();
    Real tolerance = 1.0e-13;
    Real error1 = std::fabs(fixedBondAssetSwapPrice1-fixedBondPrice1);

    if (error1>tolerance) {
        BOOST_ERROR("wrong zero spread asset swap price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's clean price:      " << fixedBondPrice1
                    << "\n  asset swap fair price: " << fixedBondAssetSwapPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error1
                    << "\n  tolerance:             " << tolerance);
    }

    // Fixed Underlying bond (Isin: IT0006527060 IBRD 5 02/05/19)
    // maturity occurs on a business day
    Date fixedBondStartDate2 = Date(5,February,2005);
    Date fixedBondMaturityDate2 = Date(5,February,2019);
    Schedule fixedBondSchedule2(fixedBondStartDate2,
                                fixedBondMaturityDate2,
                                Period(Annual), bondCalendar,
                                Unadjusted, Unadjusted,
                                DateGeneration::Backward, false);
    Leg fixedBondLeg2 = FixedRateLeg(fixedBondSchedule2,
                                     Thirty360(Thirty360::BondBasis))
        .withNotionals(vars.faceAmount)
        .withCouponRates(0.05);
    Date fixedbondRedemption2 = bondCalendar.adjust(fixedBondMaturityDate2,
                                                    Following);
    fixedBondLeg2.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, fixedbondRedemption2)));
    boost::shared_ptr<Bond> fixedBond2(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             fixedBondMaturityDate2, fixedBondStartDate2, fixedBondLeg2));
    fixedBond2->setPricingEngine(bondEngine);

    Real fixedBondPrice2 = fixedBond2->cleanPrice();
    AssetSwap fixedBondAssetSwap2(payFixedRate,
                                  fixedBond2, fixedBondPrice2,
                                  vars.iborIndex, vars.spread,
                                  vars.termStructure,
                                  Schedule(),
                                  vars.iborIndex->dayCounter(),
                                  parAssetSwap);
    Real fixedBondAssetSwapPrice2 = fixedBondAssetSwap2.fairPrice();
    Real error2 = std::fabs(fixedBondAssetSwapPrice2-fixedBondPrice2);

    if (error2>tolerance) {
        BOOST_ERROR("wrong zero spread asset swap price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's clean price:      " << fixedBondPrice2
                    << "\n  asset swap fair price: " << fixedBondAssetSwapPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error2
                    << "\n  tolerance:             " << tolerance);
    }

    // FRN Underlying bond (Isin: IT0003543847 ISPIM 0 09/29/13)
    // maturity doesn't occur on a business day
    Date floatingBondStartDate1 = Date(29,September,2003);
    Date floatingBondMaturityDate1 = Date(29,September,2013);
    Schedule floatingBondSchedule1(floatingBondStartDate1,
                                   floatingBondMaturityDate1,
                                   Period(Semiannual), bondCalendar,
                                   Unadjusted, Unadjusted,
                                   DateGeneration::Backward, false);
    Leg floatingBondLeg1 = IborLeg(floatingBondSchedule1, vars.iborIndex)
        .withNotionals(vars.faceAmount)
        .withPaymentDayCounter(Actual360())
        .withFixingDays(fixingDays)
        .withSpreads(0.0056)
        .inArrears(inArrears);
    Date floatingbondRedemption1 =
        bondCalendar.adjust(floatingBondMaturityDate1, Following);
    floatingBondLeg1.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, floatingbondRedemption1)));
    boost::shared_ptr<Bond> floatingBond1(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             floatingBondMaturityDate1, floatingBondStartDate1,
             floatingBondLeg1));
    floatingBond1->setPricingEngine(bondEngine);

    setCouponPricer(floatingBond1->cashflows(), vars.pricer);
    vars.iborIndex->addFixing(Date(27,March,2007), 0.0402);
    Real floatingBondPrice1 = floatingBond1->cleanPrice();
    AssetSwap floatingBondAssetSwap1(payFixedRate,
                                     floatingBond1, floatingBondPrice1,
                                     vars.iborIndex, vars.spread,
                                     vars.termStructure,
                                     Schedule(),
                                     vars.iborIndex->dayCounter(),
                                     parAssetSwap);
    Real floatingBondAssetSwapPrice1 = floatingBondAssetSwap1.fairPrice();
    Real error3 = std::fabs(floatingBondAssetSwapPrice1-floatingBondPrice1);

    if (error3>tolerance) {
        BOOST_ERROR("wrong zero spread asset swap price for floater:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's clean price:      " << floatingBondPrice1
                    << "\n  asset swap fair price: " <<
                    floatingBondAssetSwapPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error3
                    << "\n  tolerance:             " << tolerance);
    }

    // FRN Underlying bond (Isin: XS0090566539 COE 0 09/24/18)
    // maturity occurs on a business day
    Date floatingBondStartDate2 = Date(24,September,2004);
    Date floatingBondMaturityDate2 = Date(24,September,2018);
    Schedule floatingBondSchedule2(floatingBondStartDate2,
                                   floatingBondMaturityDate2,
                                   Period(Semiannual), bondCalendar,
                                   ModifiedFollowing, ModifiedFollowing,
                                   DateGeneration::Backward, false);
    Leg floatingBondLeg2 = IborLeg(floatingBondSchedule2, vars.iborIndex)
        .withNotionals(vars.faceAmount)
        .withPaymentDayCounter(Actual360())
        .withPaymentAdjustment(ModifiedFollowing)
        .withFixingDays(fixingDays)
        .withSpreads(0.0025)
        .inArrears(inArrears);
    Date floatingbondRedemption2 =
        bondCalendar.adjust(floatingBondMaturityDate2, ModifiedFollowing);
    floatingBondLeg2.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, floatingbondRedemption2)));
    boost::shared_ptr<Bond> floatingBond2(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             floatingBondMaturityDate2, floatingBondStartDate2,
             floatingBondLeg2));
    floatingBond2->setPricingEngine(bondEngine);

    setCouponPricer(floatingBond2->cashflows(), vars.pricer);
    vars.iborIndex->addFixing(Date(22,March,2007), 0.04013);
    Real currentCoupon=0.04013+0.0025;
    Real floatingCurrentCoupon= floatingBond2->currentCoupon();
    Real error4= std::fabs(floatingCurrentCoupon-currentCoupon);
    if (error4>tolerance) {
        BOOST_ERROR("wrong current coupon is returned for floater bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's calculated current coupon:      " <<
                    currentCoupon
                    << "\n  current coupon asked to the bond: " <<
                    floatingCurrentCoupon
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error4
                    << "\n  tolerance:             " << tolerance);
    }

    Real floatingBondPrice2 = floatingBond2->cleanPrice();
    AssetSwap floatingBondAssetSwap2(payFixedRate,
                                     floatingBond2, floatingBondPrice2,
                                     vars.iborIndex, vars.spread,
                                     vars.termStructure,
                                     Schedule(),
                                     vars.iborIndex->dayCounter(),
                                     parAssetSwap);
    Real floatingBondAssetSwapPrice2 = floatingBondAssetSwap2.fairPrice();
    Real error5 = std::fabs(floatingBondAssetSwapPrice2-floatingBondPrice2);

    if (error5>tolerance) {
        BOOST_ERROR("wrong zero spread asset swap price for floater:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's clean price:      " << floatingBondPrice2
                    << "\n  asset swap fair price: " <<
                    floatingBondAssetSwapPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error5
                    << "\n  tolerance:             " << tolerance);
    }

    // CMS Underlying bond (Isin: XS0228052402 CRDIT 0 8/22/20)
    // maturity doesn't occur on a business day
    Date cmsBondStartDate1 = Date(22,August,2005);
    Date cmsBondMaturityDate1 = Date(22,August,2020);
    Schedule cmsBondSchedule1(cmsBondStartDate1,
                              cmsBondMaturityDate1,
                              Period(Annual), bondCalendar,
                              Unadjusted, Unadjusted,
                              DateGeneration::Backward, false);
    Leg cmsBondLeg1 = CmsLeg(cmsBondSchedule1, vars.swapIndex)
        .withNotionals(vars.faceAmount)
        .withPaymentDayCounter(Thirty360())
        .withFixingDays(fixingDays)
        .withCaps(0.055)
        .withFloors(0.025)
        .inArrears(inArrears);
    Date cmsbondRedemption1 = bondCalendar.adjust(cmsBondMaturityDate1,
                                                  Following);
    cmsBondLeg1.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, cmsbondRedemption1)));
    boost::shared_ptr<Bond> cmsBond1(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             cmsBondMaturityDate1, cmsBondStartDate1, cmsBondLeg1));
    cmsBond1->setPricingEngine(bondEngine);

    setCouponPricer(cmsBond1->cashflows(), vars.cmspricer);
    vars.swapIndex->addFixing(Date(18,August,2006), 0.04158);
    Real cmsBondPrice1 = cmsBond1->cleanPrice();
    AssetSwap cmsBondAssetSwap1(payFixedRate,
                                cmsBond1, cmsBondPrice1,
                                vars.iborIndex, vars.spread,
                                vars.termStructure,
                                Schedule(),
                                vars.iborIndex->dayCounter(),
                                parAssetSwap);
    Real cmsBondAssetSwapPrice1 = cmsBondAssetSwap1.fairPrice();
    Real error6 = std::fabs(cmsBondAssetSwapPrice1-cmsBondPrice1);

    if (error6>tolerance) {
        BOOST_ERROR("wrong zero spread asset swap price for cms bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's clean price:      " << cmsBondPrice1
                    << "\n  asset swap fair price: " << cmsBondAssetSwapPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error6
                    << "\n  tolerance:             " << tolerance);
    }

    // CMS Underlying bond (Isin: XS0218766664 ISPIM 0 5/6/15)
    // maturity occurs on a business day
    Date cmsBondStartDate2 = Date(06,May,2005);
    Date cmsBondMaturityDate2 = Date(06,May,2015);
    Schedule cmsBondSchedule2(cmsBondStartDate2,
                              cmsBondMaturityDate2,
                              Period(Annual), bondCalendar,
                              Unadjusted, Unadjusted,
                              DateGeneration::Backward, false);
    Leg cmsBondLeg2 = CmsLeg(cmsBondSchedule2, vars.swapIndex)
        .withNotionals(vars.faceAmount)
        .withPaymentDayCounter(Thirty360())
        .withFixingDays(fixingDays)
        .withGearings(0.84)
        .inArrears(inArrears);
    Date cmsbondRedemption2 = bondCalendar.adjust(cmsBondMaturityDate2,
                                                  Following);
    cmsBondLeg2.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, cmsbondRedemption2)));
    boost::shared_ptr<Bond> cmsBond2(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             cmsBondMaturityDate2, cmsBondStartDate2, cmsBondLeg2));
    cmsBond2->setPricingEngine(bondEngine);

    setCouponPricer(cmsBond2->cashflows(), vars.cmspricer);
    vars.swapIndex->addFixing(Date(04,May,2006), 0.04217);
    Real cmsBondPrice2 = cmsBond2->cleanPrice();
    AssetSwap cmsBondAssetSwap2(payFixedRate,
                                cmsBond2, cmsBondPrice2,
                                vars.iborIndex, vars.spread,
                                vars.termStructure,
                                Schedule(),
                                vars.iborIndex->dayCounter(),
                                parAssetSwap);
    Real cmsBondAssetSwapPrice2 = cmsBondAssetSwap2.fairPrice();
    Real error7 = std::fabs(cmsBondAssetSwapPrice2-cmsBondPrice2);

    if (error7>tolerance) {
        BOOST_ERROR("wrong zero spread asset swap price for cms bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's clean price:      " << cmsBondPrice2
                    << "\n  asset swap fair price: " << cmsBondAssetSwapPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error7
                    << "\n  tolerance:             " << tolerance);
    }

    // Zero Coupon bond (Isin: DE0004771662 IBRD 0 12/20/15)
    // maturity doesn't occur on a business day
    Date zeroCpnBondStartDate1 = Date(19,December,1985);
    Date zeroCpnBondMaturityDate1 = Date(20,December,2015);
    Date zeroCpnBondRedemption1 = bondCalendar.adjust(zeroCpnBondMaturityDate1,
                                                      Following);
    Leg zeroCpnBondLeg1 = Leg(1, boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, zeroCpnBondRedemption1)));
    boost::shared_ptr<Bond> zeroCpnBond1(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             zeroCpnBondMaturityDate1, zeroCpnBondStartDate1, zeroCpnBondLeg1));
    zeroCpnBond1->setPricingEngine(bondEngine);

    Real zeroCpnBondPrice1 = zeroCpnBond1->cleanPrice();
    AssetSwap zeroCpnAssetSwap1(payFixedRate,
                                zeroCpnBond1, zeroCpnBondPrice1,
                                vars.iborIndex, vars.spread,
                                vars.termStructure,
                                Schedule(),
                                vars.iborIndex->dayCounter(),
                                parAssetSwap);
    Real zeroCpnBondAssetSwapPrice1 = zeroCpnAssetSwap1.fairPrice();
    Real error8 = std::fabs(zeroCpnBondAssetSwapPrice1-zeroCpnBondPrice1);

    if (error8>tolerance) {
        BOOST_ERROR("wrong zero spread asset swap price for zero cpn bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's clean price:      " << zeroCpnBondPrice1
                    << "\n  asset swap fair price: " <<
                    zeroCpnBondAssetSwapPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error8
                    << "\n  tolerance:             " << tolerance);
    }

    // Zero Coupon bond (Isin: IT0001200390 ISPIM 0 02/17/28)
    // maturity occurs on a business day
    Date zeroCpnBondStartDate2 = Date(17,February,1998);
    Date zeroCpnBondMaturityDate2 = Date(17,February,2028);
    Date zerocpbondRedemption2 = bondCalendar.adjust(zeroCpnBondMaturityDate2,
                                                      Following);
    Leg zeroCpnBondLeg2 = Leg(1, boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, zerocpbondRedemption2)));
    boost::shared_ptr<Bond> zeroCpnBond2(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             zeroCpnBondMaturityDate2, zeroCpnBondStartDate2, zeroCpnBondLeg2));
    zeroCpnBond2->setPricingEngine(bondEngine);

    Real zeroCpnBondPrice2 = zeroCpnBond2->cleanPrice();
    AssetSwap zeroCpnAssetSwap2(payFixedRate,
                                zeroCpnBond2, zeroCpnBondPrice2,
                                vars.iborIndex, vars.spread,
                                vars.termStructure,
                                Schedule(),
                                vars.iborIndex->dayCounter(),
                                parAssetSwap);
    Real zeroCpnBondAssetSwapPrice2 = zeroCpnAssetSwap2.fairPrice();
    Real error9 = std::fabs(cmsBondAssetSwapPrice2-cmsBondPrice2);

    if (error9>tolerance) {
        BOOST_ERROR("wrong zero spread asset swap price for zero cpn bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's clean price:      " << zeroCpnBondPrice2
                    << "\n  asset swap fair price: " <<
                    zeroCpnBondAssetSwapPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error9
                    << "\n  tolerance:             " << tolerance);
    }
}


void AssetSwapTest::testMASWWithGenericBond() {

    BOOST_MESSAGE("Testing market asset swap against par asset swap "
                  "with generic bond...");

    CommonVars vars;

    Calendar bondCalendar = TARGET();
    Natural settlementDays = 3;
    Natural fixingDays = 2;
    bool payFixedRate = true;
    bool parAssetSwap = true;
    bool mktAssetSwap = false;
    bool inArrears = false;

    // Fixed Underlying bond (Isin: DE0001135275 DBR 4 01/04/37)
    // maturity doesn't occur on a business day

    Date fixedBondStartDate1 = Date(4,January,2005);
    Date fixedBondMaturityDate1 = Date(4,January,2037);
    Schedule fixedBondSchedule1(fixedBondStartDate1,
                                fixedBondMaturityDate1,
                                Period(Annual), bondCalendar,
                                Unadjusted, Unadjusted,
                                DateGeneration::Backward, false);
    Leg fixedBondLeg1 = FixedRateLeg(fixedBondSchedule1,
                                     ActualActual(ActualActual::ISDA))
        .withNotionals(vars.faceAmount)
        .withCouponRates(0.04);
    Date fixedbondRedemption1 = bondCalendar.adjust(fixedBondMaturityDate1,
                                                    Following);
    fixedBondLeg1.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, fixedbondRedemption1)));
    boost::shared_ptr<Bond> fixedBond1(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             fixedBondMaturityDate1, fixedBondStartDate1,
             fixedBondLeg1));
    boost::shared_ptr<PricingEngine> bondEngine(new
        DiscountingBondEngine(vars.termStructure));
    fixedBond1->setPricingEngine(bondEngine);

    Real fixedBondMktPrice1 = 89.22 ; // market price observed on 7th June 2007
    Real fixedBondMktFullPrice1=fixedBondMktPrice1+fixedBond1->accruedAmount();
    AssetSwap fixedBondParAssetSwap1(payFixedRate,
                                     fixedBond1, fixedBondMktPrice1,
                                     vars.iborIndex, vars.spread,
                                     vars.termStructure,
                                     Schedule(),
                                     vars.iborIndex->dayCounter(),
                                     parAssetSwap);
    Real fixedBondParAssetSwapSpread1 = fixedBondParAssetSwap1.fairSpread();
    AssetSwap fixedBondMktAssetSwap1(payFixedRate,
                                     fixedBond1, fixedBondMktPrice1,
                                     vars.iborIndex, vars.spread,
                                     vars.termStructure,
                                     Schedule(),
                                     vars.iborIndex->dayCounter(),
                                     mktAssetSwap);
    Real fixedBondMktAssetSwapSpread1 = fixedBondMktAssetSwap1.fairSpread();

    Real tolerance = 1.0e-13;
    Real error1 =
        std::fabs(fixedBondMktAssetSwapSpread1-
                  100*fixedBondParAssetSwapSpread1/fixedBondMktFullPrice1);

    if (error1>tolerance) {
        BOOST_ERROR("wrong asset swap spreads for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " <<
                    fixedBondMktAssetSwapSpread1
                    << "\n  par asset swap spread: " <<
                    fixedBondParAssetSwapSpread1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error1
                    << "\n  tolerance:             " << tolerance);
    }

    // Fixed Underlying bond (Isin: IT0006527060 IBRD 5 02/05/19)
    // maturity occurs on a business day

    Date fixedBondStartDate2 = Date(5,February,2005);
    Date fixedBondMaturityDate2 = Date(5,February,2019);
    Schedule fixedBondSchedule2(fixedBondStartDate2,
                                fixedBondMaturityDate2,
                                Period(Annual), bondCalendar,
                                Unadjusted, Unadjusted,
                                DateGeneration::Backward, false);
    Leg fixedBondLeg2 = FixedRateLeg(fixedBondSchedule2,
                                     Thirty360(Thirty360::BondBasis))
        .withNotionals(vars.faceAmount)
        .withCouponRates(0.05);
    Date fixedbondRedemption2 = bondCalendar.adjust(fixedBondMaturityDate2,
                                                    Following);
    fixedBondLeg2.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, fixedbondRedemption2)));
    boost::shared_ptr<Bond> fixedBond2(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             fixedBondMaturityDate2, fixedBondStartDate2, fixedBondLeg2));
    fixedBond2->setPricingEngine(bondEngine);

    Real fixedBondMktPrice2 = 99.98 ; // market price observed on 7th June 2007
    Real fixedBondMktFullPrice2=fixedBondMktPrice2+fixedBond2->accruedAmount();
    AssetSwap fixedBondParAssetSwap2(payFixedRate,
                                     fixedBond2, fixedBondMktPrice2,
                                     vars.iborIndex, vars.spread,
                                     vars.termStructure,
                                     Schedule(),
                                     vars.iborIndex->dayCounter(),
                                     parAssetSwap);
    Real fixedBondParAssetSwapSpread2 = fixedBondParAssetSwap2.fairSpread();
    AssetSwap fixedBondMktAssetSwap2(payFixedRate,
                                     fixedBond2, fixedBondMktPrice2,
                                     vars.iborIndex, vars.spread,
                                     vars.termStructure,
                                     Schedule(),
                                     vars.iborIndex->dayCounter(),
                                     mktAssetSwap);
    Real fixedBondMktAssetSwapSpread2 = fixedBondMktAssetSwap2.fairSpread();
    Real error2 =
        std::fabs(fixedBondMktAssetSwapSpread2-
                  100*fixedBondParAssetSwapSpread2/fixedBondMktFullPrice2);

    if (error2>tolerance) {
        BOOST_ERROR("wrong asset swap spreads for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " <<
                    fixedBondMktAssetSwapSpread2
                    << "\n  par asset swap spread: " <<
                    fixedBondParAssetSwapSpread2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error2
                    << "\n  tolerance:             " << tolerance);
    }

    // FRN Underlying bond (Isin: IT0003543847 ISPIM 0 09/29/13)
    // maturity doesn't occur on a business day

    Date floatingBondStartDate1 = Date(29,September,2003);
    Date floatingBondMaturityDate1 = Date(29,September,2013);
    Schedule floatingBondSchedule1(floatingBondStartDate1,
                                   floatingBondMaturityDate1,
                                   Period(Semiannual), bondCalendar,
                                   Unadjusted, Unadjusted,
                                   DateGeneration::Backward, false);
    Leg floatingBondLeg1 = IborLeg(floatingBondSchedule1, vars.iborIndex)
        .withNotionals(vars.faceAmount)
        .withPaymentDayCounter(Actual360())
        .withFixingDays(fixingDays)
        .withSpreads(0.0056)
        .inArrears(inArrears);
    Date floatingbondRedemption1 =
        bondCalendar.adjust(floatingBondMaturityDate1, Following);
    floatingBondLeg1.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, floatingbondRedemption1)));
    boost::shared_ptr<Bond> floatingBond1(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             floatingBondMaturityDate1, floatingBondStartDate1,
             floatingBondLeg1));
    floatingBond1->setPricingEngine(bondEngine);

    setCouponPricer(floatingBond1->cashflows(), vars.pricer);
    vars.iborIndex->addFixing(Date(27,March,2007), 0.0402);
    // market price observed on 7th June 2007
    Real floatingBondMktPrice1 = 101.64 ;
    Real floatingBondMktFullPrice1 =
        floatingBondMktPrice1+floatingBond1->accruedAmount();
    AssetSwap floatingBondParAssetSwap1(payFixedRate,
                                        floatingBond1, floatingBondMktPrice1,
                                        vars.iborIndex, vars.spread,
                                        vars.termStructure,
                                        Schedule(),
                                        vars.iborIndex->dayCounter(),
                                        parAssetSwap);
    Real floatingBondParAssetSwapSpread1 =
        floatingBondParAssetSwap1.fairSpread();
    AssetSwap floatingBondMktAssetSwap1(payFixedRate,
                                        floatingBond1, floatingBondMktPrice1,
                                        vars.iborIndex, vars.spread,
                                        vars.termStructure,
                                        Schedule(),
                                        vars.iborIndex->dayCounter(),
                                        mktAssetSwap);
    Real floatingBondMktAssetSwapSpread1 =
        floatingBondMktAssetSwap1.fairSpread();
    Real error3 =
        std::fabs(floatingBondMktAssetSwapSpread1-
                  100*floatingBondParAssetSwapSpread1/floatingBondMktFullPrice1);

    if (error3>tolerance) {
        BOOST_ERROR("wrong asset swap spreads for floating bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " <<
                    floatingBondMktAssetSwapSpread1
                    << "\n  par asset swap spread: " <<
                    floatingBondParAssetSwapSpread1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error3
                    << "\n  tolerance:             " << tolerance);
    }

    // FRN Underlying bond (Isin: XS0090566539 COE 0 09/24/18)
    // maturity occurs on a business day

    Date floatingBondStartDate2 = Date(24,September,2004);
    Date floatingBondMaturityDate2 = Date(24,September,2018);
    Schedule floatingBondSchedule2(floatingBondStartDate2,
                                   floatingBondMaturityDate2,
                                   Period(Semiannual), bondCalendar,
                                   ModifiedFollowing, ModifiedFollowing,
                                   DateGeneration::Backward, false);
    Leg floatingBondLeg2 = IborLeg(floatingBondSchedule2, vars.iborIndex)
        .withNotionals(vars.faceAmount)
        .withPaymentDayCounter(Actual360())
        .withPaymentAdjustment(ModifiedFollowing)
        .withFixingDays(fixingDays)
        .withSpreads(0.0025)
        .inArrears(inArrears);
    Date floatingbondRedemption2 =
        bondCalendar.adjust(floatingBondMaturityDate2, ModifiedFollowing);
    floatingBondLeg2.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, floatingbondRedemption2)));
    boost::shared_ptr<Bond> floatingBond2(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             floatingBondMaturityDate2, floatingBondStartDate2,
             floatingBondLeg2));
    floatingBond2->setPricingEngine(bondEngine);

    setCouponPricer(floatingBond2->cashflows(), vars.pricer);
    vars.iborIndex->addFixing(Date(22,March,2007), 0.04013);
    // market price observed on 7th June 2007
    Real floatingBondMktPrice2 = 101.248 ;
    Real floatingBondMktFullPrice2 =
        floatingBondMktPrice2+floatingBond2->accruedAmount();
    AssetSwap floatingBondParAssetSwap2(payFixedRate,
                                        floatingBond2, floatingBondMktPrice2,
                                        vars.iborIndex, vars.spread,
                                        vars.termStructure,
                                        Schedule(),
                                        vars.iborIndex->dayCounter(),
                                        parAssetSwap);
    Spread floatingBondParAssetSwapSpread2 =
        floatingBondParAssetSwap2.fairSpread();
    AssetSwap floatingBondMktAssetSwap2(payFixedRate,
                                        floatingBond2, floatingBondMktPrice2,
                                        vars.iborIndex, vars.spread,
                                        vars.termStructure,
                                        Schedule(),
                                        vars.iborIndex->dayCounter(),
                                        mktAssetSwap);
    Real floatingBondMktAssetSwapSpread2 =
        floatingBondMktAssetSwap2.fairSpread();
    Real error4 =
        std::fabs(floatingBondMktAssetSwapSpread2-
                  100*floatingBondParAssetSwapSpread2/floatingBondMktFullPrice2);

    if (error4>tolerance) {
        BOOST_ERROR("wrong asset swap spreads for floating bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " <<
                    floatingBondMktAssetSwapSpread2
                    << "\n  par asset swap spread: " <<
                    floatingBondParAssetSwapSpread2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error4
                    << "\n  tolerance:             " << tolerance);
    }

    // CMS Underlying bond (Isin: XS0228052402 CRDIT 0 8/22/20)
    // maturity doesn't occur on a business day

    Date cmsBondStartDate1 = Date(22,August,2005);
    Date cmsBondMaturityDate1 = Date(22,August,2020);
    Schedule cmsBondSchedule1(cmsBondStartDate1,
                              cmsBondMaturityDate1,
                              Period(Annual), bondCalendar,
                              Unadjusted, Unadjusted,
                              DateGeneration::Backward, false);
    Leg cmsBondLeg1 = CmsLeg(cmsBondSchedule1, vars.swapIndex)
        .withNotionals(vars.faceAmount)
        .withPaymentDayCounter(Thirty360())
        .withFixingDays(fixingDays)
        .withCaps(0.055)
        .withFloors(0.025)
        .inArrears(inArrears);
    Date cmsbondRedemption1 = bondCalendar.adjust(cmsBondMaturityDate1,
                                                  Following);
    cmsBondLeg1.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, cmsbondRedemption1)));
    boost::shared_ptr<Bond> cmsBond1(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             cmsBondMaturityDate1, cmsBondStartDate1, cmsBondLeg1));
    cmsBond1->setPricingEngine(bondEngine);

    setCouponPricer(cmsBond1->cashflows(), vars.cmspricer);
    vars.swapIndex->addFixing(Date(18,August,2006), 0.04158);
    Real cmsBondMktPrice1 = 88.45 ; // market price observed on 7th June 2007
    Real cmsBondMktFullPrice1 = cmsBondMktPrice1+cmsBond1->accruedAmount();
    AssetSwap cmsBondParAssetSwap1(payFixedRate,
                                   cmsBond1, cmsBondMktPrice1,
                                   vars.iborIndex, vars.spread,
                                   vars.termStructure,
                                   Schedule(),
                                   vars.iborIndex->dayCounter(),
                                   parAssetSwap);
    Real cmsBondParAssetSwapSpread1 = cmsBondParAssetSwap1.fairSpread();
    AssetSwap cmsBondMktAssetSwap1(payFixedRate,
                                   cmsBond1, cmsBondMktPrice1,
                                   vars.iborIndex, vars.spread,
                                   vars.termStructure,
                                   Schedule(),
                                   vars.iborIndex->dayCounter(),
                                   mktAssetSwap);
    Real cmsBondMktAssetSwapSpread1 = cmsBondMktAssetSwap1.fairSpread();
    Real error5 =
        std::fabs(cmsBondMktAssetSwapSpread1-
                  100*cmsBondParAssetSwapSpread1/cmsBondMktFullPrice1);

    if (error5>tolerance) {
        BOOST_ERROR("wrong asset swap spreads for cms bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " <<
                    cmsBondMktAssetSwapSpread1
                    << "\n  par asset swap spread: " <<
                    cmsBondParAssetSwapSpread1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error5
                    << "\n  tolerance:             " << tolerance);
    }

     // CMS Underlying bond (Isin: XS0218766664 ISPIM 0 5/6/15)
     // maturity occurs on a business day

    Date cmsBondStartDate2 = Date(06,May,2005);
    Date cmsBondMaturityDate2 = Date(06,May,2015);
    Schedule cmsBondSchedule2(cmsBondStartDate2,
                              cmsBondMaturityDate2,
                              Period(Annual), bondCalendar,
                              Unadjusted, Unadjusted,
                              DateGeneration::Backward, false);
    Leg cmsBondLeg2 = CmsLeg(cmsBondSchedule2, vars.swapIndex)
        .withNotionals(vars.faceAmount)
        .withPaymentDayCounter(Thirty360())
        .withFixingDays(fixingDays)
        .withGearings(0.84)
        .inArrears(inArrears);
    Date cmsbondRedemption2 = bondCalendar.adjust(cmsBondMaturityDate2,
                                                  Following);
    cmsBondLeg2.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, cmsbondRedemption2)));
    boost::shared_ptr<Bond> cmsBond2(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             cmsBondMaturityDate2, cmsBondStartDate2, cmsBondLeg2));
    cmsBond2->setPricingEngine(bondEngine);

    setCouponPricer(cmsBond2->cashflows(), vars.cmspricer);
    vars.swapIndex->addFixing(Date(04,May,2006), 0.04217);
    Real cmsBondMktPrice2 = 94.08 ; // market price observed on 7th June 2007
    Real cmsBondMktFullPrice2 = cmsBondMktPrice2+cmsBond2->accruedAmount();
    AssetSwap cmsBondParAssetSwap2(payFixedRate,
                                   cmsBond2, cmsBondMktPrice2,
                                   vars.iborIndex, vars.spread,
                                   vars.termStructure,
                                   Schedule(),
                                   vars.iborIndex->dayCounter(),
                                   parAssetSwap);
    Spread cmsBondParAssetSwapSpread2 = cmsBondParAssetSwap2.fairSpread();
    AssetSwap cmsBondMktAssetSwap2(payFixedRate,
                                   cmsBond2, cmsBondMktPrice2,
                                   vars.iborIndex, vars.spread,
                                   vars.termStructure,
                                   Schedule(),
                                   vars.iborIndex->dayCounter(),
                                   mktAssetSwap);
    Real cmsBondMktAssetSwapSpread2 = cmsBondMktAssetSwap2.fairSpread();
    Real error6 =
        std::fabs(cmsBondMktAssetSwapSpread2-
                  100*cmsBondParAssetSwapSpread2/cmsBondMktFullPrice2);

    if (error6>tolerance) {
        BOOST_ERROR("wrong asset swap spreads for cms bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " <<
                    cmsBondMktAssetSwapSpread2
                    << "\n  par asset swap spread: " <<
                    cmsBondParAssetSwapSpread2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error6
                    << "\n  tolerance:             " << tolerance);
    }

    // Zero Coupon bond (Isin: DE0004771662 IBRD 0 12/20/15)
    // maturity doesn't occur on a business day

    Date zeroCpnBondStartDate1 = Date(19,December,1985);
    Date zeroCpnBondMaturityDate1 = Date(20,December,2015);
    Date zeroCpnBondRedemption1 = bondCalendar.adjust(zeroCpnBondMaturityDate1,
                                                      Following);
    Leg zeroCpnBondLeg1 = Leg(1, boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, zeroCpnBondRedemption1)));
    boost::shared_ptr<Bond> zeroCpnBond1(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             zeroCpnBondMaturityDate1, zeroCpnBondStartDate1, zeroCpnBondLeg1));
    zeroCpnBond1->setPricingEngine(bondEngine);

    // market price observed on 12th June 2007
    Real zeroCpnBondMktPrice1 = 70.436 ;
    Real zeroCpnBondMktFullPrice1 =
        zeroCpnBondMktPrice1+zeroCpnBond1->accruedAmount();
    AssetSwap zeroCpnBondParAssetSwap1(payFixedRate,zeroCpnBond1,
                                       zeroCpnBondMktPrice1, vars.iborIndex,
                                       vars.spread, vars.termStructure,
                                       Schedule(),
                                       vars.iborIndex->dayCounter(),
                                       parAssetSwap);
    Real zeroCpnBondParAssetSwapSpread1 = zeroCpnBondParAssetSwap1.fairSpread();
    AssetSwap zeroCpnBondMktAssetSwap1(payFixedRate,zeroCpnBond1,
                                       zeroCpnBondMktPrice1,vars.iborIndex,
                                       vars.spread, vars.termStructure,
                                       Schedule(),
                                       vars.iborIndex->dayCounter(),
                                       mktAssetSwap);
    Real zeroCpnBondMktAssetSwapSpread1 = zeroCpnBondMktAssetSwap1.fairSpread();
    Real error7 =
        std::fabs(zeroCpnBondMktAssetSwapSpread1-
                  100*zeroCpnBondParAssetSwapSpread1/zeroCpnBondMktFullPrice1);

    if (error7>tolerance) {
        BOOST_ERROR("wrong asset swap spreads for zero cpn bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " <<
                    zeroCpnBondMktAssetSwapSpread1
                    << "\n  par asset swap spread: " <<
                    zeroCpnBondParAssetSwapSpread1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error7
                    << "\n  tolerance:             " << tolerance);
    }

    // Zero Coupon bond (Isin: IT0001200390 ISPIM 0 02/17/28)
    // maturity occurs on a business day

    Date zeroCpnBondStartDate2 = Date(17,February,1998);
    Date zeroCpnBondMaturityDate2 = Date(17,February,2028);
    Date zerocpbondRedemption2 = bondCalendar.adjust(zeroCpnBondMaturityDate2,
                                                      Following);
    Leg zeroCpnBondLeg2 = Leg(1, boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, zerocpbondRedemption2)));
    boost::shared_ptr<Bond> zeroCpnBond2(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             zeroCpnBondMaturityDate2, zeroCpnBondStartDate2, zeroCpnBondLeg2));
    zeroCpnBond2->setPricingEngine(bondEngine);

    // Real zeroCpnBondPrice2 = zeroCpnBond2->cleanPrice();
    // market price observed on 12th June 2007
    Real zeroCpnBondMktPrice2 = 35.160 ;
    Real zeroCpnBondMktFullPrice2 =
        zeroCpnBondMktPrice2+zeroCpnBond2->accruedAmount();
    AssetSwap zeroCpnBondParAssetSwap2(payFixedRate,zeroCpnBond2,
                                       zeroCpnBondMktPrice2, vars.iborIndex,
                                       vars.spread, vars.termStructure,
                                       Schedule(),
                                       vars.iborIndex->dayCounter(),
                                       parAssetSwap);
    Real zeroCpnBondParAssetSwapSpread2 = zeroCpnBondParAssetSwap2.fairSpread();
    AssetSwap zeroCpnBondMktAssetSwap2(payFixedRate,zeroCpnBond2,
                                       zeroCpnBondMktPrice2,vars.iborIndex,
                                       vars.spread, vars.termStructure,
                                       Schedule(),
                                       vars.iborIndex->dayCounter(),
                                       mktAssetSwap);
    Real zeroCpnBondMktAssetSwapSpread2 = zeroCpnBondMktAssetSwap2.fairSpread();
    Real error8 =
        std::fabs(zeroCpnBondMktAssetSwapSpread2-
                  100*zeroCpnBondParAssetSwapSpread2/zeroCpnBondMktFullPrice2);

    if (error8>tolerance) {
        BOOST_ERROR("wrong asset swap spreads for zero cpn bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " <<
                    zeroCpnBondMktAssetSwapSpread2
                    << "\n  par asset swap spread: " <<
                    zeroCpnBondParAssetSwapSpread2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error8
                    << "\n  tolerance:             " << tolerance);
    }
}


void AssetSwapTest::testZSpreadWithGenericBond() {

    BOOST_MESSAGE("Testing clean and dirty price with null Z-spread "
                  "against theoretical prices...");

    CommonVars vars;

    Calendar bondCalendar = TARGET();
    Natural settlementDays = 3;
    Natural fixingDays = 2;
    bool inArrears = false;

    // Fixed Underlying bond (Isin: DE0001135275 DBR 4 01/04/37)
    // maturity doesn't occur on a business day

    Date fixedBondStartDate1 = Date(4,January,2005);
    Date fixedBondMaturityDate1 = Date(4,January,2037);
    Schedule fixedBondSchedule1(fixedBondStartDate1,
                                fixedBondMaturityDate1,
                                Period(Annual), bondCalendar,
                                Unadjusted, Unadjusted,
                                DateGeneration::Backward, false);
    Leg fixedBondLeg1 = FixedRateLeg(fixedBondSchedule1,
                                     ActualActual(ActualActual::ISDA))
        .withNotionals(vars.faceAmount)
        .withCouponRates(0.04);
    Date fixedbondRedemption1 = bondCalendar.adjust(fixedBondMaturityDate1,
                                                    Following);
    fixedBondLeg1.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, fixedbondRedemption1)));
    boost::shared_ptr<Bond> fixedBond1(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             fixedBondMaturityDate1, fixedBondStartDate1,
             fixedBondLeg1));
    boost::shared_ptr<PricingEngine> bondEngine(new
        DiscountingBondEngine(vars.termStructure));
    fixedBond1->setPricingEngine(bondEngine);

    Real fixedBondImpliedValue1 = fixedBond1->cleanPrice();
    Date fixedBondSettlementDate1= fixedBond1->settlementDate();
    // standard market conventions:
    // bond's frequency + coumpounding and daycounter of the YieldCurve
    Real fixedBondCleanPrice1= fixedBond1->cleanPriceFromZSpread(vars.spread,
         Actual365Fixed(), vars.compounding, Annual,
         fixedBondSettlementDate1);
    Real fixedBondDirtyPrice1= fixedBond1->dirtyPriceFromZSpread(vars.spread,
         Actual365Fixed(), vars.compounding, Annual,
         fixedBondSettlementDate1);
    Real tolerance = 1.0e-13;
    Real error1 = std::fabs(fixedBondImpliedValue1-fixedBondCleanPrice1);
    if (error1>tolerance) {
        BOOST_ERROR("wrong clean price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: "
                    << fixedBondImpliedValue1
                    << "\n  par asset swap spread: " << fixedBondCleanPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error1
                    << "\n  tolerance:             " << tolerance);
    }
    Real fixedBondImpliedDirty1 =
        fixedBondImpliedValue1+fixedBond1->accruedAmount();
    Real error2 = std::fabs(fixedBondImpliedDirty1-fixedBondDirtyPrice1);
    if (error2>tolerance) {
        BOOST_ERROR("wrong dirty price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: "
                    << fixedBondImpliedDirty1
                    << "\n  par asset swap spread: " << fixedBondDirtyPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error2
                    << "\n  tolerance:             " << tolerance);
    }

    // Fixed Underlying bond (Isin: IT0006527060 IBRD 5 02/05/19)
    // maturity occurs on a business day

    Date fixedBondStartDate2 = Date(5,February,2005);
    Date fixedBondMaturityDate2 = Date(5,February,2019);
    Schedule fixedBondSchedule2(fixedBondStartDate2,
                                fixedBondMaturityDate2,
                                Period(Annual), bondCalendar,
                                Unadjusted, Unadjusted,
                                DateGeneration::Backward, false);
    Leg fixedBondLeg2 = FixedRateLeg(fixedBondSchedule2,
                                     Thirty360(Thirty360::BondBasis))
        .withNotionals(vars.faceAmount)
        .withCouponRates(0.05);
    Date fixedbondRedemption2 = bondCalendar.adjust(fixedBondMaturityDate2,
                                                    Following);
    fixedBondLeg2.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, fixedbondRedemption2)));
    boost::shared_ptr<Bond> fixedBond2(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             fixedBondMaturityDate2, fixedBondStartDate2, fixedBondLeg2));
    fixedBond2->setPricingEngine(bondEngine);

    Real fixedBondImpliedValue2 = fixedBond2->cleanPrice();
    Date fixedBondSettlementDate2= fixedBond2->settlementDate();
    // standard market conventions:
    // bond's frequency + coumpounding and daycounter of the YieldCurve

    Real fixedBondCleanPrice2= fixedBond2->cleanPriceFromZSpread(vars.spread,
         Actual365Fixed(), vars.compounding, Annual,
         fixedBondSettlementDate2);
    Real fixedBondDirtyPrice2= fixedBond2->dirtyPriceFromZSpread(vars.spread,
         Actual365Fixed(), vars.compounding, Annual, //FIXME ??
         fixedBondSettlementDate2);
    Real error3 = std::fabs(fixedBondImpliedValue2-fixedBondCleanPrice2);
    if (error3>tolerance) {
        BOOST_ERROR("wrong clean price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: "
                    << fixedBondImpliedValue2
                    << "\n  par asset swap spread: " << fixedBondCleanPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error3
                    << "\n  tolerance:             " << tolerance);
    }
    Real fixedBondImpliedDirty2 =
        fixedBondImpliedValue2+fixedBond2->accruedAmount();
    Real error4 = std::fabs(fixedBondImpliedDirty2-fixedBondDirtyPrice2);
    if (error4>tolerance) {
        BOOST_ERROR("wrong dirty price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: "
                    << fixedBondImpliedDirty2
                    << "\n  par asset swap spread: " << fixedBondDirtyPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error4
                    << "\n  tolerance:             " << tolerance);
    }

    // FRN Underlying bond (Isin: IT0003543847 ISPIM 0 09/29/13)
    // maturity doesn't occur on a business day

    Date floatingBondStartDate1 = Date(29,September,2003);
    Date floatingBondMaturityDate1 = Date(29,September,2013);
    Schedule floatingBondSchedule1(floatingBondStartDate1,
                                   floatingBondMaturityDate1,
                                   Period(Semiannual), bondCalendar,
                                   Unadjusted, Unadjusted,
                                   DateGeneration::Backward, false);
    Leg floatingBondLeg1 = IborLeg(floatingBondSchedule1, vars.iborIndex)
        .withNotionals(vars.faceAmount)
        .withPaymentDayCounter(Actual360())
        .withFixingDays(fixingDays)
        .withSpreads(0.0056)
        .inArrears(inArrears);
    Date floatingbondRedemption1 =
        bondCalendar.adjust(floatingBondMaturityDate1, Following);
    floatingBondLeg1.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, floatingbondRedemption1)));
    boost::shared_ptr<Bond> floatingBond1(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             floatingBondMaturityDate1, floatingBondStartDate1,
             floatingBondLeg1));
    floatingBond1->setPricingEngine(bondEngine);

    setCouponPricer(floatingBond1->cashflows(), vars.pricer);
    vars.iborIndex->addFixing(Date(27,March,2007), 0.0402);
    Real floatingBondImpliedValue1 = floatingBond1->cleanPrice();
    Date floatingBondSettlementDate1= floatingBond1->settlementDate();
    // standard market conventions:
    // bond's frequency + coumpounding and daycounter of the YieldCurve
    Real floatingBondCleanPrice1= floatingBond1->cleanPriceFromZSpread(
        vars.spread, Actual365Fixed(), vars.compounding, Semiannual,
        fixedBondSettlementDate1);
    Real floatingBondDirtyPrice1= floatingBond1->dirtyPriceFromZSpread(
        vars.spread, Actual365Fixed(), vars.compounding, Semiannual,
        floatingBondSettlementDate1);
    Real error5 = std::fabs(floatingBondImpliedValue1-floatingBondCleanPrice1);
    if (error5>tolerance) {
        BOOST_ERROR("wrong clean price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " <<
                    floatingBondImpliedValue1
                    << "\n  par asset swap spread: " << floatingBondCleanPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error5
                    << "\n  tolerance:             " << tolerance);
    }
    Real floatingBondImpliedDirty1 = floatingBondImpliedValue1+
                                     floatingBond1->accruedAmount();
    Real error6 = std::fabs(floatingBondImpliedDirty1-floatingBondDirtyPrice1);
    if (error6>tolerance) {
        BOOST_ERROR("wrong dirty price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " <<
                    floatingBondImpliedDirty1
                    << "\n  par asset swap spread: " << floatingBondDirtyPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error6
                    << "\n  tolerance:             " << tolerance);
    }

    // FRN Underlying bond (Isin: XS0090566539 COE 0 09/24/18)
    // maturity occurs on a business day

    Date floatingBondStartDate2 = Date(24,September,2004);
    Date floatingBondMaturityDate2 = Date(24,September,2018);
    Schedule floatingBondSchedule2(floatingBondStartDate2,
                                   floatingBondMaturityDate2,
                                   Period(Semiannual), bondCalendar,
                                   ModifiedFollowing, ModifiedFollowing,
                                   DateGeneration::Backward, false);
    Leg floatingBondLeg2 = IborLeg(floatingBondSchedule2, vars.iborIndex)
        .withNotionals(vars.faceAmount)
        .withPaymentDayCounter(Actual360())
        .withPaymentAdjustment(ModifiedFollowing)
        .withFixingDays(fixingDays)
        .withSpreads(0.0025)
        .inArrears(inArrears);
    Date floatingbondRedemption2 =
        bondCalendar.adjust(floatingBondMaturityDate2, ModifiedFollowing);
    floatingBondLeg2.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, floatingbondRedemption2)));
    boost::shared_ptr<Bond> floatingBond2(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             floatingBondMaturityDate2, floatingBondStartDate2,
             floatingBondLeg2));
    floatingBond2->setPricingEngine(bondEngine);

    setCouponPricer(floatingBond2->cashflows(), vars.pricer);
    vars.iborIndex->addFixing(Date(22,March,2007), 0.04013);
    Real floatingBondImpliedValue2 = floatingBond2->cleanPrice();
    Date floatingBondSettlementDate2= floatingBond2->settlementDate();
    // standard market conventions:
    // bond's frequency + coumpounding and daycounter of the YieldCurve
    Real floatingBondCleanPrice2= floatingBond2->cleanPriceFromZSpread(
        vars.spread, Actual365Fixed(), vars.compounding, Semiannual,
        fixedBondSettlementDate1);
    Real floatingBondDirtyPrice2= floatingBond2->dirtyPriceFromZSpread(
        vars.spread, Actual365Fixed(), vars.compounding, Semiannual,
        floatingBondSettlementDate2);
    Real error7 = std::fabs(floatingBondImpliedValue2-floatingBondCleanPrice2);
    if (error7>tolerance) {
        BOOST_ERROR("wrong clean price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " <<
                    floatingBondImpliedValue2
                    << "\n  par asset swap spread: " << floatingBondCleanPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error7
                    << "\n  tolerance:             " << tolerance);
    }
    Real floatingBondImpliedDirty2 = floatingBondImpliedValue2+
                                     floatingBond2->accruedAmount();
    Real error8 = std::fabs(floatingBondImpliedDirty2-floatingBondDirtyPrice2);
    if (error8>tolerance) {
        BOOST_ERROR("wrong dirty price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " <<
                    floatingBondImpliedDirty2
                    << "\n  par asset swap spread: " << floatingBondDirtyPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error8
                    << "\n  tolerance:             " << tolerance);
    }


    // CMS Underlying bond (Isin: XS0228052402 CRDIT 0 8/22/20)
    // maturity doesn't occur on a business day

    Date cmsBondStartDate1 = Date(22,August,2005);
    Date cmsBondMaturityDate1 = Date(22,August,2020);
    Schedule cmsBondSchedule1(cmsBondStartDate1,
                              cmsBondMaturityDate1,
                              Period(Annual), bondCalendar,
                              Unadjusted, Unadjusted,
                              DateGeneration::Backward, false);
    Leg cmsBondLeg1 = CmsLeg(cmsBondSchedule1, vars.swapIndex)
        .withNotionals(vars.faceAmount)
        .withPaymentDayCounter(Thirty360())
        .withFixingDays(fixingDays)
        .withCaps(0.055)
        .withFloors(0.025)
        .inArrears(inArrears);
    Date cmsbondRedemption1 = bondCalendar.adjust(cmsBondMaturityDate1,
                                                  Following);
    cmsBondLeg1.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, cmsbondRedemption1)));
    boost::shared_ptr<Bond> cmsBond1(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             cmsBondMaturityDate1, cmsBondStartDate1, cmsBondLeg1));
    cmsBond1->setPricingEngine(bondEngine);

    setCouponPricer(cmsBond1->cashflows(), vars.cmspricer);
    vars.swapIndex->addFixing(Date(18,August,2006), 0.04158);
    Real cmsBondImpliedValue1 = cmsBond1->cleanPrice();
    Date cmsBondSettlementDate1= cmsBond1->settlementDate();
    // standard market conventions:
    // bond's frequency + coumpounding and daycounter of the YieldCurve
    Real cmsBondCleanPrice1= cmsBond1->cleanPriceFromZSpread(vars.spread,
         Actual365Fixed(), vars.compounding, Annual,
         cmsBondSettlementDate1);
    Real cmsBondDirtyPrice1= cmsBond1->dirtyPriceFromZSpread(vars.spread,
         Actual365Fixed(), vars.compounding, Annual,
         fixedBondSettlementDate1);
    Real error9 = std::fabs(cmsBondImpliedValue1-cmsBondCleanPrice1);
    if (error9>tolerance) {
        BOOST_ERROR("wrong clean price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " << cmsBondImpliedValue1
                    << "\n  par asset swap spread: " << cmsBondCleanPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error9
                    << "\n  tolerance:             " << tolerance);
    }
    Real cmsBondImpliedDirty1 = cmsBondImpliedValue1+cmsBond1->accruedAmount();
    Real error10 = std::fabs(cmsBondImpliedDirty1-cmsBondDirtyPrice1);
    if (error10>tolerance) {
        BOOST_ERROR("wrong dirty price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " << cmsBondImpliedDirty1
                    << "\n  par asset swap spread: " << cmsBondDirtyPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error10
                    << "\n  tolerance:             " << tolerance);
    }

    // CMS Underlying bond (Isin: XS0218766664 ISPIM 0 5/6/15)
    // maturity occurs on a business day

    Date cmsBondStartDate2 = Date(06,May,2005);
    Date cmsBondMaturityDate2 = Date(06,May,2015);
    Schedule cmsBondSchedule2(cmsBondStartDate2,
                              cmsBondMaturityDate2,
                              Period(Annual), bondCalendar,
                              Unadjusted, Unadjusted,
                              DateGeneration::Backward, false);
    Leg cmsBondLeg2 = CmsLeg(cmsBondSchedule2, vars.swapIndex)
        .withNotionals(vars.faceAmount)
        .withPaymentDayCounter(Thirty360())
        .withFixingDays(fixingDays)
        .withGearings(0.84)
        .inArrears(inArrears);
    Date cmsbondRedemption2 = bondCalendar.adjust(cmsBondMaturityDate2,
                                                  Following);
    cmsBondLeg2.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, cmsbondRedemption2)));
    boost::shared_ptr<Bond> cmsBond2(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             cmsBondMaturityDate2, cmsBondStartDate2, cmsBondLeg2));
    cmsBond2->setPricingEngine(bondEngine);

    setCouponPricer(cmsBond2->cashflows(), vars.cmspricer);
    vars.swapIndex->addFixing(Date(04,May,2006), 0.04217);
    Real cmsBondImpliedValue2 = cmsBond2->cleanPrice();
    Date cmsBondSettlementDate2= cmsBond2->settlementDate();
    // standard market conventions:
    // bond's frequency + coumpounding and daycounter of the YieldCurve
    Real cmsBondCleanPrice2= cmsBond2->cleanPriceFromZSpread(vars.spread,
         Actual365Fixed(), vars.compounding, Annual,
         cmsBondSettlementDate2);
    Real cmsBondDirtyPrice2= cmsBond2->dirtyPriceFromZSpread(vars.spread,
         Actual365Fixed(), vars.compounding, Annual,
         fixedBondSettlementDate2);
    Real error11 = std::fabs(cmsBondImpliedValue2-cmsBondCleanPrice2);
    if (error11>tolerance) {
        BOOST_ERROR("wrong clean price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " << cmsBondImpliedValue2
                    << "\n  par asset swap spread: " << cmsBondCleanPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error11
                    << "\n  tolerance:             " << tolerance);
    }
    Real cmsBondImpliedDirty2 = cmsBondImpliedValue2+cmsBond2->accruedAmount();
    Real error12 = std::fabs(cmsBondImpliedDirty2-cmsBondDirtyPrice2);
    if (error12>tolerance) {
        BOOST_ERROR("wrong dirty price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  market asset swap spread: " << cmsBondImpliedDirty2
                    << "\n  par asset swap spread: " << cmsBondDirtyPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error12
                    << "\n  tolerance:             " << tolerance);
    }

    // Zero Coupon bond (Isin: DE0004771662 IBRD 0 12/20/15)
    // maturity doesn't occur on a business day

    Date zeroCpnBondStartDate1 = Date(19,December,1985);
    Date zeroCpnBondMaturityDate1 = Date(20,December,2015);
    Date zeroCpnBondRedemption1 = bondCalendar.adjust(zeroCpnBondMaturityDate1,
                                                      Following);
    Leg zeroCpnBondLeg1 = Leg(1, boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, zeroCpnBondRedemption1)));
    boost::shared_ptr<Bond> zeroCpnBond1(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             zeroCpnBondMaturityDate1, zeroCpnBondStartDate1, zeroCpnBondLeg1));
    zeroCpnBond1->setPricingEngine(bondEngine);

    Real zeroCpnBondImpliedValue1 = zeroCpnBond1->cleanPrice();
    Date zeroCpnBondSettlementDate1= zeroCpnBond1->settlementDate();
    // standard market conventions:
    // bond's frequency + coumpounding and daycounter of the YieldCurve
    Real zeroCpnBondCleanPrice1 =
        zeroCpnBond1->cleanPriceFromZSpread(vars.spread,
                                            Actual365Fixed(),
                                            vars.compounding, Annual,
                                            zeroCpnBondSettlementDate1);
    Real zeroCpnBondDirtyPrice1 =
        zeroCpnBond1->dirtyPriceFromZSpread(vars.spread,
                                            Actual365Fixed(),
                                            vars.compounding, Annual,
                                            zeroCpnBondSettlementDate1);
    Real error13 = std::fabs(zeroCpnBondImpliedValue1-zeroCpnBondCleanPrice1);
    if (error13>tolerance) {
        BOOST_ERROR("wrong clean price for zero coupon bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  zero cpn implied value: " <<
                    zeroCpnBondImpliedValue1
                    << "\n  zero cpn price: " << zeroCpnBondCleanPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error13
                    << "\n  tolerance:             " << tolerance);
    }
    Real zeroCpnBondImpliedDirty1 = zeroCpnBondImpliedValue1+
                                    zeroCpnBond1->accruedAmount();
    Real error14 = std::fabs(zeroCpnBondImpliedDirty1-zeroCpnBondDirtyPrice1);
    if (error14>tolerance) {
        BOOST_ERROR("wrong dirty price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's implied dirty price: " <<
                    zeroCpnBondImpliedDirty1
                    << "\n  bond's full price: " << zeroCpnBondDirtyPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error14
                    << "\n  tolerance:             " << tolerance);
    }

    // Zero Coupon bond (Isin: IT0001200390 ISPIM 0 02/17/28)
    // maturity occurs on a business day

    Date zeroCpnBondStartDate2 = Date(17,February,1998);
    Date zeroCpnBondMaturityDate2 = Date(17,February,2028);
    Date zerocpbondRedemption2 = bondCalendar.adjust(zeroCpnBondMaturityDate2,
                                                      Following);
    Leg zeroCpnBondLeg2 = Leg(1, boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, zerocpbondRedemption2)));
    boost::shared_ptr<Bond> zeroCpnBond2(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             zeroCpnBondMaturityDate2, zeroCpnBondStartDate2, zeroCpnBondLeg2));
    zeroCpnBond2->setPricingEngine(bondEngine);

    Real zeroCpnBondImpliedValue2 = zeroCpnBond2->cleanPrice();
    Date zeroCpnBondSettlementDate2= zeroCpnBond2->settlementDate();
    // standard market conventions:
    // bond's frequency + coumpounding and daycounter of the YieldCurve
    Real zeroCpnBondCleanPrice2 =
        zeroCpnBond2->cleanPriceFromZSpread(vars.spread,
                                            Actual365Fixed(),
                                            vars.compounding, Annual,
                                            zeroCpnBondSettlementDate2);
    Real zeroCpnBondDirtyPrice2 =
        zeroCpnBond2->dirtyPriceFromZSpread(vars.spread,
                                            Actual365Fixed(),
                                            vars.compounding, Annual,
                                            zeroCpnBondSettlementDate2);
    Real error15 = std::fabs(zeroCpnBondImpliedValue2-zeroCpnBondCleanPrice2);
    if (error15>tolerance) {
        BOOST_ERROR("wrong clean price for zero coupon bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  zero cpn implied value: " <<
                    zeroCpnBondImpliedValue2
                    << "\n  zero cpn price: " << zeroCpnBondCleanPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error15
                    << "\n  tolerance:             " << tolerance);
    }
    Real zeroCpnBondImpliedDirty2 = zeroCpnBondImpliedValue2+
                                    zeroCpnBond2->accruedAmount();
    Real error16 = std::fabs(zeroCpnBondImpliedDirty2-zeroCpnBondDirtyPrice2);
    if (error16>tolerance) {
        BOOST_ERROR("wrong dirty price for zero coupon bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  bond's implied dirty price: " <<
                    zeroCpnBondImpliedDirty2
                    << "\n  bond's full price: " << zeroCpnBondDirtyPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error16
                    << "\n  tolerance:             " << tolerance);
    }
}


void AssetSwapTest::testSpecializedBondVsGenericBond() {

    BOOST_MESSAGE("Testing clean and dirty prices for specialized bond against"
                  " equivalent generic bond...");

    CommonVars vars;

    Calendar bondCalendar = TARGET();
    Natural settlementDays = 3;
    Natural fixingDays = 2;
    bool inArrears = false;

    // Fixed Underlying bond (Isin: DE0001135275 DBR 4 01/04/37)
    // maturity doesn't occur on a business day
    Date fixedBondStartDate1 = Date(4,January,2005);
    Date fixedBondMaturityDate1 = Date(4,January,2037);
    Schedule fixedBondSchedule1(fixedBondStartDate1,
                                fixedBondMaturityDate1,
                                Period(Annual), bondCalendar,
                                Unadjusted, Unadjusted,
                                DateGeneration::Backward, false);
    Leg fixedBondLeg1 = FixedRateLeg(fixedBondSchedule1,
                                     ActualActual(ActualActual::ISDA))
        .withNotionals(vars.faceAmount)
        .withCouponRates(0.04);
    Date fixedbondRedemption1 = bondCalendar.adjust(fixedBondMaturityDate1,
                                                    Following);
    fixedBondLeg1.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, fixedbondRedemption1)));
    // generic bond
    boost::shared_ptr<Bond> fixedBond1(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             fixedBondMaturityDate1, fixedBondStartDate1,
             fixedBondLeg1));
    boost::shared_ptr<PricingEngine> bondEngine(new
        DiscountingBondEngine(vars.termStructure));
    fixedBond1->setPricingEngine(bondEngine);

    // equivalent specialized fixed rate bond
    boost::shared_ptr<Bond> fixedSpecializedBond1(new
        FixedRateBond(settlementDays, vars.faceAmount, fixedBondSchedule1,
                      std::vector<Rate>(1, 0.04),
                      ActualActual(ActualActual::ISDA), Following,
                      100.0, Date(4,January,2005) ));
    fixedSpecializedBond1->setPricingEngine(bondEngine);

    Real fixedBondTheoValue1 = fixedBond1->cleanPrice();
    Real fixedSpecializedBondTheoValue1 = fixedSpecializedBond1->cleanPrice();
    Real tolerance = 1.0e-13;
    Real error1 = std::fabs(fixedBondTheoValue1-fixedSpecializedBondTheoValue1);
    if (error1>tolerance) {
        BOOST_ERROR("wrong clean price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  specialized fixed rate bond's theo clean price: "
                    << fixedBondTheoValue1
                    << "\n  generic equivalent bond's theo clean price: "
                    << fixedSpecializedBondTheoValue1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error1
                    << "\n  tolerance:             " << tolerance);
    }
    Real fixedBondTheoDirty1 = fixedBondTheoValue1+fixedBond1->accruedAmount();
    Real fixedSpecializedTheoDirty1 = fixedSpecializedBondTheoValue1+
                                  fixedSpecializedBond1->accruedAmount();
    Real error2 = std::fabs(fixedBondTheoDirty1-fixedSpecializedTheoDirty1);
    if (error2>tolerance) {
        BOOST_ERROR("wrong dirty price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  specialized fixed rate bond's theo dirty price: "
                    << fixedBondTheoDirty1
                    << "\n  generic equivalent bond's theo dirty price: "
                    << fixedSpecializedTheoDirty1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error2
                    << "\n  tolerance:             " << tolerance);
    }

    // Fixed Underlying bond (Isin: IT0006527060 IBRD 5 02/05/19)
    // maturity occurs on a business day
    Date fixedBondStartDate2 = Date(5,February,2005);
    Date fixedBondMaturityDate2 = Date(5,February,2019);
    Schedule fixedBondSchedule2(fixedBondStartDate2,
                                fixedBondMaturityDate2,
                                Period(Annual), bondCalendar,
                                Unadjusted, Unadjusted,
                                DateGeneration::Backward, false);
    Leg fixedBondLeg2 = FixedRateLeg(fixedBondSchedule2,
                                     Thirty360(Thirty360::BondBasis))
        .withNotionals(vars.faceAmount)
        .withCouponRates(0.05);
    Date fixedbondRedemption2 = bondCalendar.adjust(fixedBondMaturityDate2,
                                                    Following);
    fixedBondLeg2.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, fixedbondRedemption2)));

    // generic bond
    boost::shared_ptr<Bond> fixedBond2(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             fixedBondMaturityDate2, fixedBondStartDate2, fixedBondLeg2));
    fixedBond2->setPricingEngine(bondEngine);

    // equivalent specialized fixed rate bond
    boost::shared_ptr<Bond> fixedSpecializedBond2(new
         FixedRateBond(settlementDays, vars.faceAmount, fixedBondSchedule2,
                      std::vector<Rate>(1, 0.05),
                      Thirty360(Thirty360::BondBasis), Following,
                      100.0, Date(5,February,2005)));
    fixedSpecializedBond2->setPricingEngine(bondEngine);

    Real fixedBondTheoValue2 = fixedBond2->cleanPrice();
    Real fixedSpecializedBondTheoValue2 = fixedSpecializedBond2->cleanPrice();

    Real error3 = std::fabs(fixedBondTheoValue2-fixedSpecializedBondTheoValue2);
    if (error3>tolerance) {
        BOOST_ERROR("wrong clean price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  specialized fixed rate bond's theo clean price: "
                    << fixedBondTheoValue2
                    << "\n  generic equivalent bond's theo clean price: "
                    << fixedSpecializedBondTheoValue2
                    << "\n  error:                 " << error3
                    << "\n  tolerance:             " << tolerance);
    }
    Real fixedBondTheoDirty2 = fixedBondTheoValue2+
                               fixedBond2->accruedAmount();
    Real fixedSpecializedBondTheoDirty2 = fixedSpecializedBondTheoValue2+
                                      fixedSpecializedBond2->accruedAmount();

    Real error4 = std::fabs(fixedBondTheoDirty2-fixedSpecializedBondTheoDirty2);
    if (error4>tolerance) {
        BOOST_ERROR("wrong dirty price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  specialized fixed rate bond's dirty clean price: "
                    << fixedBondTheoDirty2
                    << "\n  generic equivalent bond's theo dirty price: "
                    << fixedSpecializedBondTheoDirty2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error4
                    << "\n  tolerance:             " << tolerance);
    }

    // FRN Underlying bond (Isin: IT0003543847 ISPIM 0 09/29/13)
    // maturity doesn't occur on a business day
    Date floatingBondStartDate1 = Date(29,September,2003);
    Date floatingBondMaturityDate1 = Date(29,September,2013);
    Schedule floatingBondSchedule1(floatingBondStartDate1,
                                   floatingBondMaturityDate1,
                                   Period(Semiannual), bondCalendar,
                                   Unadjusted, Unadjusted,
                                   DateGeneration::Backward, false);
    Leg floatingBondLeg1 = IborLeg(floatingBondSchedule1, vars.iborIndex)
        .withNotionals(vars.faceAmount)
        .withPaymentDayCounter(Actual360())
        .withFixingDays(fixingDays)
        .withSpreads(0.0056)
        .inArrears(inArrears);
    Date floatingbondRedemption1 =
        bondCalendar.adjust(floatingBondMaturityDate1, Following);
    floatingBondLeg1.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, floatingbondRedemption1)));
    // generic bond
    boost::shared_ptr<Bond> floatingBond1(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             floatingBondMaturityDate1, floatingBondStartDate1,
             floatingBondLeg1));
    floatingBond1->setPricingEngine(bondEngine);

    // equivalent specialized floater
    boost::shared_ptr<Bond> floatingSpecializedBond1(new
           FloatingRateBond(settlementDays, vars.faceAmount,
                            floatingBondSchedule1,
                            vars.iborIndex, Actual360(),
                            Following, fixingDays,
                            std::vector<Real>(1,1),
                            std::vector<Spread>(1,0.0056),
                            std::vector<Rate>(), std::vector<Rate>(),
                            inArrears,
                            100.0, Date(29,September,2003)));
    floatingSpecializedBond1->setPricingEngine(bondEngine);

    setCouponPricer(floatingBond1->cashflows(), vars.pricer);
    setCouponPricer(floatingSpecializedBond1->cashflows(), vars.pricer);
    vars.iborIndex->addFixing(Date(27,March,2007), 0.0402);
    Real floatingBondTheoValue1 = floatingBond1->cleanPrice();
    Real floatingSpecializedBondTheoValue1 =
        floatingSpecializedBond1->cleanPrice();

    Real error5 = std::fabs(floatingBondTheoValue1-
                            floatingSpecializedBondTheoValue1);
    if (error5>tolerance) {
        BOOST_ERROR("wrong clean price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic fixed rate bond's theo clean price: "
                    << floatingBondTheoValue1
                    << "\n  equivalent specialized bond's theo clean price: "
                    << floatingSpecializedBondTheoValue1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error5
                    << "\n  tolerance:             " << tolerance);
    }
    Real floatingBondTheoDirty1 = floatingBondTheoValue1+
                                  floatingBond1->accruedAmount();
    Real floatingSpecializedBondTheoDirty1 =
        floatingSpecializedBondTheoValue1+
        floatingSpecializedBond1->accruedAmount();
    Real error6 = std::fabs(floatingBondTheoDirty1-
                            floatingSpecializedBondTheoDirty1);
    if (error6>tolerance) {
        BOOST_ERROR("wrong dirty price for frn bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic frn bond's dirty clean price: "
                    << floatingBondTheoDirty1
                    << "\n  equivalent specialized bond's theo dirty price: "
                    << floatingSpecializedBondTheoDirty1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error6
                    << "\n  tolerance:             " << tolerance);
    }

    // FRN Underlying bond (Isin: XS0090566539 COE 0 09/24/18)
    // maturity occurs on a business day
    Date floatingBondStartDate2 = Date(24,September,2004);
    Date floatingBondMaturityDate2 = Date(24,September,2018);
    Schedule floatingBondSchedule2(floatingBondStartDate2,
                                   floatingBondMaturityDate2,
                                   Period(Semiannual), bondCalendar,
                                   ModifiedFollowing, ModifiedFollowing,
                                   DateGeneration::Backward, false);
    Leg floatingBondLeg2 = IborLeg(floatingBondSchedule2, vars.iborIndex)
        .withNotionals(vars.faceAmount)
        .withPaymentDayCounter(Actual360())
        .withPaymentAdjustment(ModifiedFollowing)
        .withFixingDays(fixingDays)
        .withSpreads(0.0025)
        .inArrears(inArrears);
    Date floatingbondRedemption2 =
        bondCalendar.adjust(floatingBondMaturityDate2, ModifiedFollowing);
    floatingBondLeg2.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, floatingbondRedemption2)));
    // generic bond
    boost::shared_ptr<Bond> floatingBond2(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             floatingBondMaturityDate2, floatingBondStartDate2,
             floatingBondLeg2));
    floatingBond2->setPricingEngine(bondEngine);

    // equivalent specialized floater
    boost::shared_ptr<Bond> floatingSpecializedBond2(new
        FloatingRateBond(settlementDays, vars.faceAmount,
                         floatingBondSchedule2,
                         vars.iborIndex, Actual360(),
                         ModifiedFollowing, fixingDays,
                         std::vector<Real>(1,1),
                         std::vector<Spread>(1,0.0025),
                         std::vector<Rate>(), std::vector<Rate>(),
                         inArrears,
                         100.0, Date(24,September,2004)));
    floatingSpecializedBond2->setPricingEngine(bondEngine);

    setCouponPricer(floatingBond2->cashflows(), vars.pricer);
    setCouponPricer(floatingSpecializedBond2->cashflows(), vars.pricer);

    vars.iborIndex->addFixing(Date(22,March,2007), 0.04013);

    Real floatingBondTheoValue2 = floatingBond2->cleanPrice();
    Real floatingSpecializedBondTheoValue2 =
        floatingSpecializedBond2->cleanPrice();

    Real error7 =
        std::fabs(floatingBondTheoValue2-floatingSpecializedBondTheoValue2);
    if (error7>tolerance) {
        BOOST_ERROR("wrong clean price for floater bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic floater bond's theo clean price: "
                    << floatingBondTheoValue2
                    << "\n  equivalent specialized bond's theo clean price: "
                    << floatingSpecializedBondTheoValue2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error7
                    << "\n  tolerance:             " << tolerance);
    }
    Real floatingBondTheoDirty2 = floatingBondTheoValue2+
                                  floatingBond2->accruedAmount();
    Real floatingSpecializedTheoDirty2 = floatingSpecializedBondTheoValue2+
                                     floatingSpecializedBond2->accruedAmount();

    Real error8 =
        std::fabs(floatingBondTheoDirty2-floatingSpecializedTheoDirty2);
    if (error8>tolerance) {
        BOOST_ERROR("wrong dirty price for floater bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic floater bond's theo dirty price: "
                    << floatingBondTheoDirty2
                    << "\n  equivalent specialized  bond's theo dirty price: "
                    << floatingSpecializedTheoDirty2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error8
                    << "\n  tolerance:             " << tolerance);
    }


    // CMS Underlying bond (Isin: XS0228052402 CRDIT 0 8/22/20)
    // maturity doesn't occur on a business day
    Date cmsBondStartDate1 = Date(22,August,2005);
    Date cmsBondMaturityDate1 = Date(22,August,2020);
    Schedule cmsBondSchedule1(cmsBondStartDate1,
                              cmsBondMaturityDate1,
                              Period(Annual), bondCalendar,
                              Unadjusted, Unadjusted,
                              DateGeneration::Backward, false);
    Leg cmsBondLeg1 = CmsLeg(cmsBondSchedule1, vars.swapIndex)
        .withNotionals(vars.faceAmount)
        .withPaymentDayCounter(Thirty360())
        .withFixingDays(fixingDays)
        .withCaps(0.055)
        .withFloors(0.025)
        .inArrears(inArrears);
    Date cmsbondRedemption1 = bondCalendar.adjust(cmsBondMaturityDate1,
                                                  Following);
    cmsBondLeg1.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, cmsbondRedemption1)));
    // generic cms bond
    boost::shared_ptr<Bond> cmsBond1(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             cmsBondMaturityDate1, cmsBondStartDate1, cmsBondLeg1));
    cmsBond1->setPricingEngine(bondEngine);

    // equivalent specialized cms bond
    boost::shared_ptr<Bond> cmsSpecializedBond1(new
        CmsRateBond(settlementDays, vars.faceAmount, cmsBondSchedule1,
                vars.swapIndex, Thirty360(),
                Following, fixingDays,
                std::vector<Real>(1,1.0), std::vector<Spread>(1,0.0),
                std::vector<Rate>(1,0.055), std::vector<Rate>(1,0.025),
                inArrears,
                100.0, Date(22,August,2005)));
    cmsSpecializedBond1->setPricingEngine(bondEngine);

    setCouponPricer(cmsBond1->cashflows(), vars.cmspricer);
    setCouponPricer(cmsSpecializedBond1->cashflows(), vars.cmspricer);
    vars.swapIndex->addFixing(Date(18,August,2006), 0.04158);
    Real cmsBondTheoValue1 = cmsBond1->cleanPrice();
    Real cmsSpecializedBondTheoValue1 = cmsSpecializedBond1->cleanPrice();
    Real error9 = std::fabs(cmsBondTheoValue1-cmsSpecializedBondTheoValue1);
    if (error9>tolerance) {
        BOOST_ERROR("wrong clean price for cms bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic cms bond's theo clean price: "
                    << cmsBondTheoValue1
                    <<  "\n  equivalent specialized bond's theo clean price: "
                    << cmsSpecializedBondTheoValue1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error9
                    << "\n  tolerance:             " << tolerance);
    }
    Real cmsBondTheoDirty1 = cmsBondTheoValue1+cmsBond1->accruedAmount();
    Real cmsSpecializedBondTheoDirty1 = cmsSpecializedBondTheoValue1+
                                    cmsSpecializedBond1->accruedAmount();
    Real error10 = std::fabs(cmsBondTheoDirty1-cmsSpecializedBondTheoDirty1);
    if (error10>tolerance) {
        BOOST_ERROR("wrong dirty price for cms bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n generic cms bond's theo dirty price: "
                    << cmsBondTheoDirty1
                    << "\n  specialized cms bond's theo dirty price: "
                    << cmsSpecializedBondTheoDirty1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error10
                    << "\n  tolerance:             " << tolerance);
    }

    // CMS Underlying bond (Isin: XS0218766664 ISPIM 0 5/6/15)
    // maturity occurs on a business day
    Date cmsBondStartDate2 = Date(06,May,2005);
    Date cmsBondMaturityDate2 = Date(06,May,2015);
    Schedule cmsBondSchedule2(cmsBondStartDate2,
                              cmsBondMaturityDate2,
                              Period(Annual), bondCalendar,
                              Unadjusted, Unadjusted,
                              DateGeneration::Backward, false);
    Leg cmsBondLeg2 = CmsLeg(cmsBondSchedule2, vars.swapIndex)
        .withNotionals(vars.faceAmount)
        .withPaymentDayCounter(Thirty360())
        .withFixingDays(fixingDays)
        .withGearings(0.84)
        .inArrears(inArrears);
    Date cmsbondRedemption2 = bondCalendar.adjust(cmsBondMaturityDate2,
                                                  Following);
    cmsBondLeg2.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, cmsbondRedemption2)));
    // generic bond
    boost::shared_ptr<Bond> cmsBond2(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             cmsBondMaturityDate2, cmsBondStartDate2, cmsBondLeg2));
    cmsBond2->setPricingEngine(bondEngine);

    // equivalent specialized cms bond
    boost::shared_ptr<Bond> cmsSpecializedBond2(new
        CmsRateBond(settlementDays, vars.faceAmount, cmsBondSchedule2,
                vars.swapIndex, Thirty360(),
                Following, fixingDays,
                std::vector<Real>(1,0.84), std::vector<Spread>(1,0.0),
                std::vector<Rate>(), std::vector<Rate>(),
                inArrears,
                100.0, Date(06,May,2005)));
    cmsSpecializedBond2->setPricingEngine(bondEngine);

    setCouponPricer(cmsBond2->cashflows(), vars.cmspricer);
    setCouponPricer(cmsSpecializedBond2->cashflows(), vars.cmspricer);
    vars.swapIndex->addFixing(Date(04,May,2006), 0.04217);
    Real cmsBondTheoValue2 = cmsBond2->cleanPrice();
    Real cmsSpecializedBondTheoValue2 = cmsSpecializedBond2->cleanPrice();

    Real error11 = std::fabs(cmsBondTheoValue2-cmsSpecializedBondTheoValue2);
    if (error11>tolerance) {
        BOOST_ERROR("wrong clean price for cms bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic cms bond's theo clean price: "
                    << cmsBondTheoValue2
                    << "\n  cms bond's theo clean price: "
                    << cmsSpecializedBondTheoValue2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error11
                    << "\n  tolerance:             " << tolerance);
    }
    Real cmsBondTheoDirty2 = cmsBondTheoValue2+cmsBond2->accruedAmount();
    Real cmsSpecializedBondTheoDirty2 =
        cmsSpecializedBondTheoValue2+cmsSpecializedBond2->accruedAmount();
    Real error12 = std::fabs(cmsBondTheoDirty2-cmsSpecializedBondTheoDirty2);
    if (error12>tolerance) {
        BOOST_ERROR("wrong dirty price for cms bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic cms bond's dirty price: "
                    << cmsBondTheoDirty2
                    << "\n  specialized cms bond's theo dirty price: "
                    << cmsSpecializedBondTheoDirty2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error12
                    << "\n  tolerance:             " << tolerance);
    }

    // Zero Coupon bond (Isin: DE0004771662 IBRD 0 12/20/15)
    // maturity doesn't occur on a business day
    Date zeroCpnBondStartDate1 = Date(19,December,1985);
    Date zeroCpnBondMaturityDate1 = Date(20,December,2015);
    Date zeroCpnBondRedemption1 = bondCalendar.adjust(zeroCpnBondMaturityDate1,
                                                      Following);
    Leg zeroCpnBondLeg1 = Leg(1, boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, zeroCpnBondRedemption1)));
    // generic bond
    boost::shared_ptr<Bond> zeroCpnBond1(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             zeroCpnBondMaturityDate1, zeroCpnBondStartDate1, zeroCpnBondLeg1));
    zeroCpnBond1->setPricingEngine(bondEngine);

    // specialized zerocpn bond
    boost::shared_ptr<Bond> zeroCpnSpecializedBond1(new
        ZeroCouponBond(settlementDays, bondCalendar, vars.faceAmount,
                  Date(20,December,2015),
                  Following,
                  100.0, Date(19,December,1985)));
    zeroCpnSpecializedBond1->setPricingEngine(bondEngine);

    Real zeroCpnBondTheoValue1 = zeroCpnBond1->cleanPrice();
    Real zeroCpnSpecializedBondTheoValue1 =
        zeroCpnSpecializedBond1->cleanPrice();

    Real error13 =
        std::fabs(zeroCpnBondTheoValue1-zeroCpnSpecializedBondTheoValue1);
    if (error13>tolerance) {
        BOOST_ERROR("wrong clean price for zero coupon bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic zero bond's clean price: "
                    << zeroCpnBondTheoValue1
                    << "\n  specialized zero bond's clean price: "
                    << zeroCpnSpecializedBondTheoValue1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error13
                    << "\n  tolerance:             " << tolerance);
    }
    Real zeroCpnBondTheoDirty1 = zeroCpnBondTheoValue1+
                                 zeroCpnBond1->accruedAmount();
    Real zeroCpnSpecializedBondTheoDirty1 =
        zeroCpnSpecializedBondTheoValue1+
        zeroCpnSpecializedBond1->accruedAmount();
    Real error14 =
        std::fabs(zeroCpnBondTheoDirty1-zeroCpnSpecializedBondTheoDirty1);
    if (error14>tolerance) {
        BOOST_ERROR("wrong dirty price for zero bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic zerocpn bond's dirty price: "
                    << zeroCpnBondTheoDirty1
                    << "\n  specialized zerocpn bond's clean price: "
                    << zeroCpnSpecializedBondTheoDirty1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error14
                    << "\n  tolerance:             " << tolerance);
    }

    // Zero Coupon bond (Isin: IT0001200390 ISPIM 0 02/17/28)
    // maturity occurs on a business day
    Date zeroCpnBondStartDate2 = Date(17,February,1998);
    Date zeroCpnBondMaturityDate2 = Date(17,February,2028);
    Date zerocpbondRedemption2 = bondCalendar.adjust(zeroCpnBondMaturityDate2,
                                                      Following);
    Leg zeroCpnBondLeg2 = Leg(1, boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, zerocpbondRedemption2)));
    // generic bond
    boost::shared_ptr<Bond> zeroCpnBond2(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             zeroCpnBondMaturityDate2, zeroCpnBondStartDate2, zeroCpnBondLeg2));
    zeroCpnBond2->setPricingEngine(bondEngine);

    // specialized zerocpn bond
    boost::shared_ptr<Bond> zeroCpnSpecializedBond2(new
        ZeroCouponBond(settlementDays, bondCalendar, vars.faceAmount,
                   Date(17,February,2028),
                   Following,
                   100.0, Date(17,February,1998)));
    zeroCpnSpecializedBond2->setPricingEngine(bondEngine);

    Real zeroCpnBondTheoValue2 = zeroCpnBond2->cleanPrice();
    Real zeroCpnSpecializedBondTheoValue2 =
        zeroCpnSpecializedBond2->cleanPrice();

    Real error15 =
        std::fabs(zeroCpnBondTheoValue2 -zeroCpnSpecializedBondTheoValue2);
    if (error15>tolerance) {
        BOOST_ERROR("wrong clean price for zero coupon bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic zerocpn bond's clean price: "
                    << zeroCpnBondTheoValue2
                    << "\n  specialized zerocpn bond's clean price: "
                    << zeroCpnSpecializedBondTheoValue2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error15
                    << "\n  tolerance:             " << tolerance);
    }
    Real zeroCpnBondTheoDirty2 = zeroCpnBondTheoValue2+
                                 zeroCpnBond2->accruedAmount();

    Real zeroCpnSpecializedBondTheoDirty2 =
        zeroCpnSpecializedBondTheoValue2+
        zeroCpnSpecializedBond2->accruedAmount();

    Real error16 =
        std::fabs(zeroCpnBondTheoDirty2-zeroCpnSpecializedBondTheoDirty2);
    if (error16>tolerance) {
        BOOST_ERROR("wrong dirty price for zero coupon bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic zerocpn bond's dirty price: "
                    << zeroCpnBondTheoDirty2
                    << "\n  specialized zerocpn bond's dirty price: "
                    << zeroCpnSpecializedBondTheoDirty2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error16
                    << "\n  tolerance:             " << tolerance);
    }
}


void AssetSwapTest::testSpecializedBondVsGenericBondUsingAsw() {

    BOOST_MESSAGE("Testing asset-swap prices and spreads for specialized bond"
                  " against equivalent generic bond...");

    CommonVars vars;

    Calendar bondCalendar = TARGET();
    Natural settlementDays = 3;
    Natural fixingDays = 2;
    bool payFixedRate = true;
    bool parAssetSwap = true;
    bool inArrears = false;

    // Fixed bond (Isin: DE0001135275 DBR 4 01/04/37)
    // maturity doesn't occur on a business day
    Date fixedBondStartDate1 = Date(4,January,2005);
    Date fixedBondMaturityDate1 = Date(4,January,2037);
    Schedule fixedBondSchedule1(fixedBondStartDate1,
                                fixedBondMaturityDate1,
                                Period(Annual), bondCalendar,
                                Unadjusted, Unadjusted,
                                DateGeneration::Backward, false);
    Leg fixedBondLeg1 = FixedRateLeg(fixedBondSchedule1,
                                     ActualActual(ActualActual::ISDA))
        .withNotionals(vars.faceAmount)
        .withCouponRates(0.04);
    Date fixedbondRedemption1 = bondCalendar.adjust(fixedBondMaturityDate1,
                                                    Following);
    fixedBondLeg1.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, fixedbondRedemption1)));
    // generic bond
    boost::shared_ptr<Bond> fixedBond1(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             fixedBondMaturityDate1, fixedBondStartDate1,
             fixedBondLeg1));
    boost::shared_ptr<PricingEngine> bondEngine(new
        DiscountingBondEngine(vars.termStructure));
    fixedBond1->setPricingEngine(bondEngine);

    // equivalent specialized fixed rate bond
    boost::shared_ptr<Bond> fixedSpecializedBond1(new
        FixedRateBond(settlementDays, vars.faceAmount, fixedBondSchedule1,
                      std::vector<Rate>(1, 0.04),
                      ActualActual(ActualActual::ISDA), Following,
                      100.0, Date(4,January,2005) ));
    fixedSpecializedBond1->setPricingEngine(bondEngine);

    Real fixedBondPrice1 = fixedBond1->cleanPrice();
    Real fixedSpecializedBondPrice1 = fixedSpecializedBond1->cleanPrice();
    AssetSwap fixedBondAssetSwap1(payFixedRate,
                                  fixedBond1, fixedBondPrice1,
                                  vars.iborIndex, vars.nonnullspread,
                                  vars.termStructure,
                                  Schedule(),
                                  vars.iborIndex->dayCounter(),
                                  parAssetSwap);
    AssetSwap fixedSpecializedBondAssetSwap1(payFixedRate,
                                             fixedSpecializedBond1,
                                             fixedSpecializedBondPrice1,
                                             vars.iborIndex,
                                             vars.nonnullspread,
                                             vars.termStructure,
                                             Schedule(),
                                             vars.iborIndex->dayCounter(),
                                             parAssetSwap);
    Real fixedBondAssetSwapPrice1 = fixedBondAssetSwap1.fairPrice();
    Real fixedSpecializedBondAssetSwapPrice1 =
        fixedSpecializedBondAssetSwap1.fairPrice();
    Real tolerance = 1.0e-13;
    Real error1 =
        std::fabs(fixedBondAssetSwapPrice1-fixedSpecializedBondAssetSwapPrice1);
    if (error1>tolerance) {
        BOOST_ERROR("wrong clean price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic  fixed rate bond's  clean price: "
                    << fixedBondAssetSwapPrice1
                    << "\n  equivalent specialized bond's clean price: "
                    << fixedSpecializedBondAssetSwapPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error1
                    << "\n  tolerance:             " << tolerance);
    }
    // market executable price as of 4th sept 2007
    Real fixedBondMktPrice1= 91.832;
    AssetSwap fixedBondASW1(payFixedRate,
                            fixedBond1, fixedBondMktPrice1,
                            vars.iborIndex, vars.spread,
                            vars.termStructure,
                            Schedule(),
                            vars.iborIndex->dayCounter(),
                            parAssetSwap);
    AssetSwap fixedSpecializedBondASW1(payFixedRate,
                                       fixedSpecializedBond1,
                                       fixedBondMktPrice1,
                                       vars.iborIndex, vars.spread,
                                       vars.termStructure,
                                       Schedule(),
                                       vars.iborIndex->dayCounter(),
                                       parAssetSwap);
    Real fixedBondASWSpread1 = fixedBondASW1.fairSpread();
    Real fixedSpecializedBondASWSpread1 = fixedSpecializedBondASW1.fairSpread();
    Real error2 = std::fabs(fixedBondASWSpread1-fixedSpecializedBondASWSpread1);
    if (error2>tolerance) {
        BOOST_ERROR("wrong asw spread  for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic  fixed rate bond's  asw spread: "
                    << fixedBondASWSpread1
                    << "\n  equivalent specialized bond's asw spread: "
                    << fixedSpecializedBondASWSpread1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error2
                    << "\n  tolerance:             " << tolerance);
    }

     //Fixed bond (Isin: IT0006527060 IBRD 5 02/05/19)
     //maturity occurs on a business day

    Date fixedBondStartDate2 = Date(5,February,2005);
    Date fixedBondMaturityDate2 = Date(5,February,2019);
    Schedule fixedBondSchedule2(fixedBondStartDate2,
                                fixedBondMaturityDate2,
                                Period(Annual), bondCalendar,
                                Unadjusted, Unadjusted,
                                DateGeneration::Backward, false);
    Leg fixedBondLeg2 = FixedRateLeg(fixedBondSchedule2,
                                     Thirty360(Thirty360::BondBasis))
        .withNotionals(vars.faceAmount)
        .withCouponRates(0.05);
    Date fixedbondRedemption2 = bondCalendar.adjust(fixedBondMaturityDate2,
                                                    Following);
    fixedBondLeg2.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, fixedbondRedemption2)));

    // generic bond
    boost::shared_ptr<Bond> fixedBond2(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             fixedBondMaturityDate2, fixedBondStartDate2, fixedBondLeg2));
    fixedBond2->setPricingEngine(bondEngine);

    // equivalent specialized fixed rate bond
    boost::shared_ptr<Bond> fixedSpecializedBond2(new
         FixedRateBond(settlementDays, vars.faceAmount, fixedBondSchedule2,
                      std::vector<Rate>(1, 0.05),
                      Thirty360(Thirty360::BondBasis), Following,
                      100.0, Date(5,February,2005)));
    fixedSpecializedBond2->setPricingEngine(bondEngine);

    Real fixedBondPrice2 = fixedBond2->cleanPrice();
    Real fixedSpecializedBondPrice2 = fixedSpecializedBond2->cleanPrice();
    AssetSwap fixedBondAssetSwap2(payFixedRate,
                                  fixedBond2, fixedBondPrice2,
                                  vars.iborIndex, vars.nonnullspread,
                                  vars.termStructure,
                                  Schedule(),
                                  vars.iborIndex->dayCounter(),
                                  parAssetSwap);
    AssetSwap fixedSpecializedBondAssetSwap2(payFixedRate,
                                             fixedSpecializedBond2,
                                             fixedSpecializedBondPrice2,
                                             vars.iborIndex,
                                             vars.nonnullspread,
                                             vars.termStructure,
                                             Schedule(),
                                             vars.iborIndex->dayCounter(),
                                             parAssetSwap);
    Real fixedBondAssetSwapPrice2 = fixedBondAssetSwap2.fairPrice();
    Real fixedSpecializedBondAssetSwapPrice2 =
        fixedSpecializedBondAssetSwap2.fairPrice();

    Real error3 =
        std::fabs(fixedBondAssetSwapPrice2-fixedSpecializedBondAssetSwapPrice2);
    if (error3>tolerance) {
        BOOST_ERROR("wrong clean price for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic  fixed rate bond's clean price: "
                    << fixedBondAssetSwapPrice2
                    << "\n  equivalent specialized  bond's clean price: "
                    << fixedSpecializedBondAssetSwapPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error3
                    << "\n  tolerance:             " << tolerance);
    }
    // market executable price as of 4th sept 2007
    Real fixedBondMktPrice2= 102.178;
    AssetSwap fixedBondASW2(payFixedRate,
                            fixedBond2, fixedBondMktPrice2,
                            vars.iborIndex, vars.spread,
                            vars.termStructure,
                            Schedule(),
                            vars.iborIndex->dayCounter(),
                            parAssetSwap);
    AssetSwap fixedSpecializedBondASW2(payFixedRate,
                                       fixedSpecializedBond2,
                                       fixedBondMktPrice2,
                                       vars.iborIndex, vars.spread,
                                       vars.termStructure,
                                       Schedule(),
                                       vars.iborIndex->dayCounter(),
                                       parAssetSwap);
    Real fixedBondASWSpread2 = fixedBondASW2.fairSpread();
    Real fixedSpecializedBondASWSpread2 = fixedSpecializedBondASW2.fairSpread();
    Real error4 = std::fabs(fixedBondASWSpread2-fixedSpecializedBondASWSpread2);
    if (error4>tolerance) {
        BOOST_ERROR("wrong asw spread for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic  fixed rate bond's  asw spread: "
                    << fixedBondASWSpread2
                    << "\n  equivalent specialized bond's asw spread: "
                    << fixedSpecializedBondASWSpread2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error4
                    << "\n  tolerance:             " << tolerance);
    }


    //FRN bond (Isin: IT0003543847 ISPIM 0 09/29/13)
    //maturity doesn't occur on a business day
    Date floatingBondStartDate1 = Date(29,September,2003);
    Date floatingBondMaturityDate1 = Date(29,September,2013);
    Schedule floatingBondSchedule1(floatingBondStartDate1,
                                   floatingBondMaturityDate1,
                                   Period(Semiannual), bondCalendar,
                                   Unadjusted, Unadjusted,
                                   DateGeneration::Backward, false);
    Leg floatingBondLeg1 = IborLeg(floatingBondSchedule1, vars.iborIndex)
        .withNotionals(vars.faceAmount)
        .withPaymentDayCounter(Actual360())
        .withFixingDays(fixingDays)
        .withSpreads(0.0056)
        .inArrears(inArrears);
    Date floatingbondRedemption1 =
        bondCalendar.adjust(floatingBondMaturityDate1, Following);
    floatingBondLeg1.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, floatingbondRedemption1)));
    // generic bond
    boost::shared_ptr<Bond> floatingBond1(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             floatingBondMaturityDate1, floatingBondStartDate1,
             floatingBondLeg1));
    floatingBond1->setPricingEngine(bondEngine);

    // equivalent specialized floater
    boost::shared_ptr<Bond> floatingSpecializedBond1(new
           FloatingRateBond(settlementDays, vars.faceAmount,
                            floatingBondSchedule1,
                            vars.iborIndex, Actual360(),
                            Following, fixingDays,
                            std::vector<Real>(1,1),
                            std::vector<Spread>(1,0.0056),
                            std::vector<Rate>(), std::vector<Rate>(),
                            inArrears,
                            100.0, Date(29,September,2003)));
    floatingSpecializedBond1->setPricingEngine(bondEngine);

    setCouponPricer(floatingBond1->cashflows(), vars.pricer);
    setCouponPricer(floatingSpecializedBond1->cashflows(), vars.pricer);
    vars.iborIndex->addFixing(Date(27,March,2007), 0.0402);
    Real floatingBondPrice1 = floatingBond1->cleanPrice();
    Real floatingSpecializedBondPrice1= floatingSpecializedBond1->cleanPrice();
    AssetSwap floatingBondAssetSwap1(payFixedRate,
                                     floatingBond1, floatingBondPrice1,
                                     vars.iborIndex, vars.nonnullspread,
                                     vars.termStructure,
                                     Schedule(),
                                     vars.iborIndex->dayCounter(),
                                     parAssetSwap);
    AssetSwap floatingSpecializedBondAssetSwap1(payFixedRate,
                                                floatingSpecializedBond1,
                                                floatingSpecializedBondPrice1,
                                                vars.iborIndex,
                                                vars.nonnullspread,
                                                vars.termStructure,
                                                Schedule(),
                                                vars.iborIndex->dayCounter(),
                                                parAssetSwap);
    Real floatingBondAssetSwapPrice1 = floatingBondAssetSwap1.fairPrice();
    Real floatingSpecializedBondAssetSwapPrice1 =
        floatingSpecializedBondAssetSwap1.fairPrice();

    Real error5 =
        std::fabs(floatingBondAssetSwapPrice1-floatingSpecializedBondAssetSwapPrice1);
    if (error5>tolerance) {
        BOOST_ERROR("wrong clean price for frnbond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic frn rate bond's clean price: "
                    << floatingBondAssetSwapPrice1
                    << "\n  equivalent specialized  bond's price: "
                    << floatingSpecializedBondAssetSwapPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error5
                    << "\n  tolerance:             " << tolerance);
    }
    // market executable price as of 4th sept 2007
    Real floatingBondMktPrice1= 101.33;
    AssetSwap floatingBondASW1(payFixedRate,
                               floatingBond1, floatingBondMktPrice1,
                               vars.iborIndex, vars.spread,
                               vars.termStructure,
                               Schedule(),
                               vars.iborIndex->dayCounter(),
                               parAssetSwap);
    AssetSwap floatingSpecializedBondASW1(payFixedRate,
                                          floatingSpecializedBond1,
                                          floatingBondMktPrice1,
                                          vars.iborIndex, vars.spread,
                                          vars.termStructure,
                                          Schedule(),
                                          vars.iborIndex->dayCounter(),
                                          parAssetSwap);
    Real floatingBondASWSpread1 = floatingBondASW1.fairSpread();
    Real floatingSpecializedBondASWSpread1 =
        floatingSpecializedBondASW1.fairSpread();
    Real error6 =
        std::fabs(floatingBondASWSpread1-floatingSpecializedBondASWSpread1);
    if (error6>tolerance) {
        BOOST_ERROR("wrong asw spread for fixed bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic  frn rate bond's  asw spread: "
                    << floatingBondASWSpread1
                    << "\n  equivalent specialized bond's asw spread: "
                    << floatingSpecializedBondASWSpread1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error6
                    << "\n  tolerance:             " << tolerance);
    }
    //FRN bond (Isin: XS0090566539 COE 0 09/24/18)
    //maturity occurs on a business day
    Date floatingBondStartDate2 = Date(24,September,2004);
    Date floatingBondMaturityDate2 = Date(24,September,2018);
    Schedule floatingBondSchedule2(floatingBondStartDate2,
                                   floatingBondMaturityDate2,
                                   Period(Semiannual), bondCalendar,
                                   ModifiedFollowing, ModifiedFollowing,
                                   DateGeneration::Backward, false);
    Leg floatingBondLeg2 = IborLeg(floatingBondSchedule2, vars.iborIndex)
        .withNotionals(vars.faceAmount)
        .withPaymentDayCounter(Actual360())
        .withPaymentAdjustment(ModifiedFollowing)
        .withFixingDays(fixingDays)
        .withSpreads(0.0025)
        .inArrears(inArrears);
    Date floatingbondRedemption2 =
        bondCalendar.adjust(floatingBondMaturityDate2,
                            ModifiedFollowing);
    floatingBondLeg2.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, floatingbondRedemption2)));
    // generic bond
    boost::shared_ptr<Bond> floatingBond2(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             floatingBondMaturityDate2, floatingBondStartDate2,
             floatingBondLeg2));
    floatingBond2->setPricingEngine(bondEngine);

    // equivalent specialized floater
    boost::shared_ptr<Bond> floatingSpecializedBond2(new
        FloatingRateBond(settlementDays, vars.faceAmount,
                         floatingBondSchedule2,
                         vars.iborIndex, Actual360(),
                         ModifiedFollowing, fixingDays,
                         std::vector<Real>(1,1),
                         std::vector<Spread>(1,0.0025),
                         std::vector<Rate>(), std::vector<Rate>(),
                         inArrears,
                         100.0, Date(24,September,2004)));
    floatingSpecializedBond2->setPricingEngine(bondEngine);

    setCouponPricer(floatingBond2->cashflows(), vars.pricer);
    setCouponPricer(floatingSpecializedBond2->cashflows(), vars.pricer);

    vars.iborIndex->addFixing(Date(22,March,2007), 0.04013);

    Real floatingBondPrice2 = floatingBond2->cleanPrice();
    Real floatingSpecializedBondPrice2= floatingSpecializedBond2->cleanPrice();
    AssetSwap floatingBondAssetSwap2(payFixedRate,
                                     floatingBond2, floatingBondPrice2,
                                     vars.iborIndex, vars.nonnullspread,
                                     vars.termStructure,
                                     Schedule(),
                                     vars.iborIndex->dayCounter(),
                                     parAssetSwap);
    AssetSwap floatingSpecializedBondAssetSwap2(payFixedRate,
                                                floatingSpecializedBond2,
                                                floatingSpecializedBondPrice2,
                                                vars.iborIndex,
                                                vars.nonnullspread,
                                                vars.termStructure,
                                                Schedule(),
                                                vars.iborIndex->dayCounter(),
                                                parAssetSwap);
    Real floatingBondAssetSwapPrice2 = floatingBondAssetSwap2.fairPrice();
    Real floatingSpecializedBondAssetSwapPrice2 =
        floatingSpecializedBondAssetSwap2.fairPrice();
    Real error7 =
        std::fabs(floatingBondAssetSwapPrice2-floatingSpecializedBondAssetSwapPrice2);
    if (error7>tolerance) {
        BOOST_ERROR("wrong clean price for frnbond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic frn rate bond's clean price: "
                    << floatingBondAssetSwapPrice2
                    << "\n  equivalent specialized frn  bond's price: "
                    << floatingSpecializedBondAssetSwapPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error7
                    << "\n  tolerance:             " << tolerance);
    }
    // market executable price as of 4th sept 2007
    Real floatingBondMktPrice2 = 101.26;
    AssetSwap floatingBondASW2(payFixedRate,
                               floatingBond2, floatingBondMktPrice2,
                               vars.iborIndex, vars.spread,
                               vars.termStructure,
                               Schedule(),
                               vars.iborIndex->dayCounter(),
                               parAssetSwap);
    AssetSwap floatingSpecializedBondASW2(payFixedRate,
                                          floatingSpecializedBond2,
                                          floatingBondMktPrice2,
                                          vars.iborIndex, vars.spread,
                                          vars.termStructure,
                                          Schedule(),
                                          vars.iborIndex->dayCounter(),
                                          parAssetSwap);
    Real floatingBondASWSpread2 = floatingBondASW2.fairSpread();
    Real floatingSpecializedBondASWSpread2 =
        floatingSpecializedBondASW2.fairSpread();
    Real error8 =
        std::fabs(floatingBondASWSpread2-floatingSpecializedBondASWSpread2);
    if (error8>tolerance) {
        BOOST_ERROR("wrong asw spread for frn bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic  frn rate bond's  asw spread: "
                    << floatingBondASWSpread2
                    << "\n  equivalent specialized bond's asw spread: "
                    << floatingSpecializedBondASWSpread2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error8
                    << "\n  tolerance:             " << tolerance);
    }

    // CMS bond (Isin: XS0228052402 CRDIT 0 8/22/20)
    // maturity doesn't occur on a business day
    Date cmsBondStartDate1 = Date(22,August,2005);
    Date cmsBondMaturityDate1 = Date(22,August,2020);
    Schedule cmsBondSchedule1(cmsBondStartDate1,
                              cmsBondMaturityDate1,
                              Period(Annual), bondCalendar,
                              Unadjusted, Unadjusted,
                              DateGeneration::Backward, false);
    Leg cmsBondLeg1 = CmsLeg(cmsBondSchedule1, vars.swapIndex)
        .withNotionals(vars.faceAmount)
        .withPaymentDayCounter(Thirty360())
        .withFixingDays(fixingDays)
        .withCaps(0.055)
        .withFloors(0.025)
        .inArrears(inArrears);
    Date cmsbondRedemption1 = bondCalendar.adjust(cmsBondMaturityDate1,
                                                  Following);
    cmsBondLeg1.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, cmsbondRedemption1)));
    // generic cms bond
    boost::shared_ptr<Bond> cmsBond1(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             cmsBondMaturityDate1, cmsBondStartDate1, cmsBondLeg1));
    cmsBond1->setPricingEngine(bondEngine);

    // equivalent specialized cms bond
    boost::shared_ptr<Bond> cmsSpecializedBond1(new
        CmsRateBond(settlementDays, vars.faceAmount, cmsBondSchedule1,
                vars.swapIndex, Thirty360(),
                Following, fixingDays,
                std::vector<Real>(1,1.0), std::vector<Spread>(1,0.0),
                std::vector<Rate>(1,0.055), std::vector<Rate>(1,0.025),
                inArrears,
                100.0, Date(22,August,2005)));
    cmsSpecializedBond1->setPricingEngine(bondEngine);


    setCouponPricer(cmsBond1->cashflows(), vars.cmspricer);
    setCouponPricer(cmsSpecializedBond1->cashflows(), vars.cmspricer);
    vars.swapIndex->addFixing(Date(18,August,2006), 0.04158);
    Real cmsBondPrice1 = cmsBond1->cleanPrice();
    Real cmsSpecializedBondPrice1 = cmsSpecializedBond1->cleanPrice();
    AssetSwap cmsBondAssetSwap1(payFixedRate,cmsBond1, cmsBondPrice1,
                                vars.iborIndex, vars.nonnullspread,
                                vars.termStructure,
                                Schedule(),vars.iborIndex->dayCounter(),
                                parAssetSwap);
    AssetSwap cmsSpecializedBondAssetSwap1(payFixedRate,cmsSpecializedBond1,
                                           cmsSpecializedBondPrice1,
                                           vars.iborIndex,
                                           vars.nonnullspread,
                                           vars.termStructure,
                                           Schedule(),
                                           vars.iborIndex->dayCounter(),
                                           parAssetSwap);
    Real cmsBondAssetSwapPrice1 = cmsBondAssetSwap1.fairPrice();
    Real cmsSpecializedBondAssetSwapPrice1 =
        cmsSpecializedBondAssetSwap1.fairPrice();
    Real error9 =
        std::fabs(cmsBondAssetSwapPrice1-cmsSpecializedBondAssetSwapPrice1);
    if (error9>tolerance) {
        BOOST_ERROR("wrong clean price for cmsbond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic bond's clean price: "
                    << cmsBondAssetSwapPrice1
                    << "\n  equivalent specialized cms rate bond's price: "
                    << cmsSpecializedBondAssetSwapPrice1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error9
                    << "\n  tolerance:             " << tolerance);
    }
    Real cmsBondMktPrice1 = 87.02;// market executable price as of 4th sept 2007
    AssetSwap cmsBondASW1(payFixedRate,
                          cmsBond1, cmsBondMktPrice1,
                          vars.iborIndex, vars.spread,
                          vars.termStructure,
                          Schedule(),
                          vars.iborIndex->dayCounter(),
                          parAssetSwap);
    AssetSwap cmsSpecializedBondASW1(payFixedRate,
                                     cmsSpecializedBond1,
                                     cmsBondMktPrice1,
                                     vars.iborIndex, vars.spread,
                                     vars.termStructure,
                                     Schedule(),
                                     vars.iborIndex->dayCounter(),
                                     parAssetSwap);
    Real cmsBondASWSpread1 = cmsBondASW1.fairSpread();
    Real cmsSpecializedBondASWSpread1 = cmsSpecializedBondASW1.fairSpread();
    Real error10 = std::fabs(cmsBondASWSpread1-cmsSpecializedBondASWSpread1);
    if (error10>tolerance) {
        BOOST_ERROR("wrong asw spread for cm bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic cms rate bond's  asw spread: "
                    << cmsBondASWSpread1
                    << "\n  equivalent specialized bond's asw spread: "
                    << cmsSpecializedBondASWSpread1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error10
                    << "\n  tolerance:             " << tolerance);
    }

      //CMS bond (Isin: XS0218766664 ISPIM 0 5/6/15)
      //maturity occurs on a business day
    Date cmsBondStartDate2 = Date(06,May,2005);
    Date cmsBondMaturityDate2 = Date(06,May,2015);
    Schedule cmsBondSchedule2(cmsBondStartDate2,
                              cmsBondMaturityDate2,
                              Period(Annual), bondCalendar,
                              Unadjusted, Unadjusted,
                              DateGeneration::Backward, false);
    Leg cmsBondLeg2 = CmsLeg(cmsBondSchedule2, vars.swapIndex)
        .withNotionals(vars.faceAmount)
        .withPaymentDayCounter(Thirty360())
        .withFixingDays(fixingDays)
        .withGearings(0.84)
        .inArrears(inArrears);
    Date cmsbondRedemption2 = bondCalendar.adjust(cmsBondMaturityDate2,
                                                  Following);
    cmsBondLeg2.push_back(boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, cmsbondRedemption2)));
    // generic bond
    boost::shared_ptr<Bond> cmsBond2(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             cmsBondMaturityDate2, cmsBondStartDate2, cmsBondLeg2));
    cmsBond2->setPricingEngine(bondEngine);

    // equivalent specialized cms bond
    boost::shared_ptr<Bond> cmsSpecializedBond2(new
        CmsRateBond(settlementDays, vars.faceAmount, cmsBondSchedule2,
                vars.swapIndex, Thirty360(),
                Following, fixingDays,
                std::vector<Real>(1,0.84), std::vector<Spread>(1,0.0),
                std::vector<Rate>(), std::vector<Rate>(),
                inArrears,
                100.0, Date(06,May,2005)));
    cmsSpecializedBond2->setPricingEngine(bondEngine);

    setCouponPricer(cmsBond2->cashflows(), vars.cmspricer);
    setCouponPricer(cmsSpecializedBond2->cashflows(), vars.cmspricer);
    vars.swapIndex->addFixing(Date(04,May,2006), 0.04217);
    Real cmsBondPrice2 = cmsBond2->cleanPrice();
    Real cmsSpecializedBondPrice2 = cmsSpecializedBond2->cleanPrice();
    AssetSwap cmsBondAssetSwap2(payFixedRate,cmsBond2, cmsBondPrice2,
                                vars.iborIndex, vars.nonnullspread,
                                vars.termStructure,
                                Schedule(),
                                vars.iborIndex->dayCounter(),
                                parAssetSwap);
    AssetSwap cmsSpecializedBondAssetSwap2(payFixedRate,cmsSpecializedBond2,
                                           cmsSpecializedBondPrice2,
                                           vars.iborIndex,
                                           vars.nonnullspread,
                                           vars.termStructure,
                                           Schedule(),
                                           vars.iborIndex->dayCounter(),
                                           parAssetSwap);
    Real cmsBondAssetSwapPrice2 = cmsBondAssetSwap2.fairPrice();
    Real cmsSpecializedBondAssetSwapPrice2 =
        cmsSpecializedBondAssetSwap2.fairPrice();
    Real error11 =
        std::fabs(cmsBondAssetSwapPrice2-cmsSpecializedBondAssetSwapPrice2);
    if (error11>tolerance) {
        BOOST_ERROR("wrong clean price for cmsbond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic  bond's clean price: "
                    << cmsBondAssetSwapPrice2
                    << "\n  equivalent specialized cms rate bond's price: "
                    << cmsSpecializedBondAssetSwapPrice2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error11
                    << "\n  tolerance:             " << tolerance);
    }
    Real cmsBondMktPrice2 = 94.35;// market executable price as of 4th sept 2007
    AssetSwap cmsBondASW2(payFixedRate,
                          cmsBond2, cmsBondMktPrice2,
                          vars.iborIndex, vars.spread,
                          vars.termStructure,
                          Schedule(),
                          vars.iborIndex->dayCounter(),
                          parAssetSwap);
    AssetSwap cmsSpecializedBondASW2(payFixedRate,
                                     cmsSpecializedBond2,
                                     cmsBondMktPrice2,
                                     vars.iborIndex, vars.spread,
                                     vars.termStructure,
                                     Schedule(),
                                     vars.iborIndex->dayCounter(),
                                     parAssetSwap);
    Real cmsBondASWSpread2 = cmsBondASW2.fairSpread();
    Real cmsSpecializedBondASWSpread2 = cmsSpecializedBondASW2.fairSpread();
    Real error12 = std::fabs(cmsBondASWSpread2-cmsSpecializedBondASWSpread2);
    if (error12>tolerance) {
        BOOST_ERROR("wrong asw spread for cm bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic cms rate bond's  asw spread: "
                    << cmsBondASWSpread2
                    << "\n  equivalent specialized bond's asw spread: "
                    << cmsSpecializedBondASWSpread2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error12
                    << "\n  tolerance:             " << tolerance);
    }


   //  Zero-Coupon bond (Isin: DE0004771662 IBRD 0 12/20/15)
   //  maturity doesn't occur on a business day
    Date zeroCpnBondStartDate1 = Date(19,December,1985);
    Date zeroCpnBondMaturityDate1 = Date(20,December,2015);
    Date zeroCpnBondRedemption1 = bondCalendar.adjust(zeroCpnBondMaturityDate1,
                                                      Following);
    Leg zeroCpnBondLeg1 = Leg(1, boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, zeroCpnBondRedemption1)));
    // generic bond
    boost::shared_ptr<Bond> zeroCpnBond1(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             zeroCpnBondMaturityDate1, zeroCpnBondStartDate1, zeroCpnBondLeg1));
    zeroCpnBond1->setPricingEngine(bondEngine);

    // specialized zerocpn bond
    boost::shared_ptr<Bond> zeroCpnSpecializedBond1(new
        ZeroCouponBond(settlementDays, bondCalendar, vars.faceAmount,
                  Date(20,December,2015),
                  Following,
                  100.0, Date(19,December,1985)));
    zeroCpnSpecializedBond1->setPricingEngine(bondEngine);

    Real zeroCpnBondPrice1 = zeroCpnBond1->cleanPrice();
    Real zeroCpnSpecializedBondPrice1 = zeroCpnSpecializedBond1->cleanPrice();
    AssetSwap zeroCpnBondAssetSwap1(payFixedRate,zeroCpnBond1,
                                    zeroCpnBondPrice1,
                                    vars.iborIndex, vars.nonnullspread,
                                    vars.termStructure,
                                    Schedule(),
                                    vars.iborIndex->dayCounter(),
                                    parAssetSwap);
    AssetSwap zeroCpnSpecializedBondAssetSwap1(payFixedRate,
                                               zeroCpnSpecializedBond1,
                                               zeroCpnSpecializedBondPrice1,
                                               vars.iborIndex,
                                               vars.nonnullspread,
                                               vars.termStructure,
                                               Schedule(),
                                               vars.iborIndex->dayCounter(),
                                               parAssetSwap);
    Real zeroCpnBondAssetSwapPrice1 = zeroCpnBondAssetSwap1.fairPrice();
    Real zeroCpnSpecializedBondAssetSwapPrice1 =
        zeroCpnSpecializedBondAssetSwap1.fairPrice();
    Real error13 =
        std::fabs(zeroCpnBondAssetSwapPrice1-zeroCpnSpecializedBondAssetSwapPrice1);
    if (error13>tolerance) {
        BOOST_ERROR("wrong clean price for zerocpn bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic zero cpn bond's clean price: "
                    << zeroCpnBondAssetSwapPrice1
                    << "\n  specialized equivalent bond's price: "
                    << zeroCpnSpecializedBondAssetSwapPrice1
                    << "\n  error:                 " << error13
                    << "\n  tolerance:             " << tolerance);
    }
    // market executable price as of 4th sept 2007
    Real zeroCpnBondMktPrice1 = 72.277;
    AssetSwap zeroCpnBondASW1(payFixedRate,
                              zeroCpnBond1,zeroCpnBondMktPrice1,
                              vars.iborIndex, vars.spread,
                              vars.termStructure,
                              Schedule(),
                              vars.iborIndex->dayCounter(),
                              parAssetSwap);
    AssetSwap zeroCpnSpecializedBondASW1(payFixedRate,
                                         zeroCpnSpecializedBond1,
                                         zeroCpnBondMktPrice1,
                                         vars.iborIndex, vars.spread,
                                         vars.termStructure,
                                         Schedule(),
                                         vars.iborIndex->dayCounter(),
                                         parAssetSwap);
    Real zeroCpnBondASWSpread1 = zeroCpnBondASW1.fairSpread();
    Real zeroCpnSpecializedBondASWSpread1 =
        zeroCpnSpecializedBondASW1.fairSpread();
    Real error14 =
        std::fabs(zeroCpnBondASWSpread1-zeroCpnSpecializedBondASWSpread1);
    if (error14>tolerance) {
        BOOST_ERROR("wrong asw spread for zeroCpn bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic zeroCpn bond's  asw spread: "
                    << zeroCpnBondASWSpread1
                    << "\n  equivalent specialized bond's asw spread: "
                    << zeroCpnSpecializedBondASWSpread1
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error14
                    << "\n  tolerance:             " << tolerance);
    }


   //  Zero Coupon bond (Isin: IT0001200390 ISPIM 0 02/17/28)
   //  maturity doesn't occur on a business day
    Date zeroCpnBondStartDate2 = Date(17,February,1998);
    Date zeroCpnBondMaturityDate2 = Date(17,February,2028);
    Date zerocpbondRedemption2 = bondCalendar.adjust(zeroCpnBondMaturityDate2,
                                                      Following);
    Leg zeroCpnBondLeg2 = Leg(1, boost::shared_ptr<CashFlow>(new
        SimpleCashFlow(100.0, zerocpbondRedemption2)));
    // generic bond
    boost::shared_ptr<Bond> zeroCpnBond2(new
        Bond(settlementDays, bondCalendar, vars.faceAmount,
             zeroCpnBondMaturityDate2, zeroCpnBondStartDate2, zeroCpnBondLeg2));
    zeroCpnBond2->setPricingEngine(bondEngine);

    // specialized zerocpn bond
    boost::shared_ptr<Bond> zeroCpnSpecializedBond2(new
        ZeroCouponBond(settlementDays, bondCalendar, vars.faceAmount,
                   Date(17,February,2028),
                   Following,
                   100.0, Date(17,February,1998)));
    zeroCpnSpecializedBond2->setPricingEngine(bondEngine);

    Real zeroCpnBondPrice2 = zeroCpnBond2->cleanPrice();
    Real zeroCpnSpecializedBondPrice2 = zeroCpnSpecializedBond2->cleanPrice();

    AssetSwap zeroCpnBondAssetSwap2(payFixedRate,zeroCpnBond2,
                                    zeroCpnBondPrice2,
                                    vars.iborIndex, vars.nonnullspread,
                                    vars.termStructure,
                                    Schedule(),
                                    vars.iborIndex->dayCounter(),
                                    parAssetSwap);
    AssetSwap zeroCpnSpecializedBondAssetSwap2(payFixedRate,
                                               zeroCpnSpecializedBond2,
                                               zeroCpnSpecializedBondPrice2,
                                               vars.iborIndex,
                                               vars.nonnullspread,
                                               vars.termStructure,
                                               Schedule(),
                                               vars.iborIndex->dayCounter(),
                                               parAssetSwap);
    Real zeroCpnBondAssetSwapPrice2 = zeroCpnBondAssetSwap2.fairPrice();
    Real zeroCpnSpecializedBondAssetSwapPrice2 =
                               zeroCpnSpecializedBondAssetSwap2.fairPrice();
    Real error15 = std::fabs(zeroCpnBondAssetSwapPrice2
                             -zeroCpnSpecializedBondAssetSwapPrice2);
    if (error8>tolerance) {
        BOOST_ERROR("wrong clean price for zerocpn bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic zero cpn bond's clean price: "
                    << zeroCpnBondAssetSwapPrice2
                    << "\n  equivalent specialized bond's price: "
                    << zeroCpnSpecializedBondAssetSwapPrice2
                    << "\n  error:                 " << error15
                    << "\n  tolerance:             " << tolerance);
    }
    // market executable price as of 4th sept 2007
    Real zeroCpnBondMktPrice2 = 72.277;
    AssetSwap zeroCpnBondASW2(payFixedRate,
                              zeroCpnBond2,zeroCpnBondMktPrice2,
                              vars.iborIndex, vars.spread,
                              vars.termStructure,
                              Schedule(),
                              vars.iborIndex->dayCounter(),
                              parAssetSwap);
    AssetSwap zeroCpnSpecializedBondASW2(payFixedRate,
                                         zeroCpnSpecializedBond2,
                                         zeroCpnBondMktPrice2,
                                         vars.iborIndex, vars.spread,
                                         vars.termStructure,
                                         Schedule(),
                                         vars.iborIndex->dayCounter(),
                                         parAssetSwap);
    Real zeroCpnBondASWSpread2 = zeroCpnBondASW2.fairSpread();
    Real zeroCpnSpecializedBondASWSpread2 =
        zeroCpnSpecializedBondASW2.fairSpread();
    Real error16 =
        std::fabs(zeroCpnBondASWSpread2-zeroCpnSpecializedBondASWSpread2);
    if (error16>tolerance) {
        BOOST_ERROR("wrong asw spread for zeroCpn bond:"
                    << QL_FIXED << std::setprecision(4)
                    << "\n  generic zeroCpn bond's  asw spread: "
                    << zeroCpnBondASWSpread2
                    << "\n  equivalent specialized bond's asw spread: "
                    << zeroCpnSpecializedBondASWSpread2
                    << QL_SCIENTIFIC << std::setprecision(2)
                    << "\n  error:                 " << error16
                    << "\n  tolerance:             " << tolerance);
    }
}


test_suite* AssetSwapTest::suite() {
    test_suite* suite = BOOST_TEST_SUITE("AssetSwap tests");
    suite->add(BOOST_TEST_CASE(&AssetSwapTest::testImpliedValue));
    suite->add(BOOST_TEST_CASE(&AssetSwapTest::testMarketASWSpread));
    suite->add(BOOST_TEST_CASE(&AssetSwapTest::testZSpread));
    suite->add(BOOST_TEST_CASE(&AssetSwapTest::testGenericBondImplied));
    suite->add(BOOST_TEST_CASE(&AssetSwapTest::testMASWWithGenericBond));
    suite->add(BOOST_TEST_CASE(&AssetSwapTest::testZSpreadWithGenericBond));
    suite->add(BOOST_TEST_CASE(
                           &AssetSwapTest::testSpecializedBondVsGenericBond));
    suite->add(BOOST_TEST_CASE(
                   &AssetSwapTest::testSpecializedBondVsGenericBondUsingAsw));

    return suite;
}