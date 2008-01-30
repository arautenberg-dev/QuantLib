/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2000, 2001, 2002, 2003 RiskMap srl
 Copyright (C) 2003, 2004, 2005, 2006, 2007 StatPro Italia srl
 Copyright (C) 2007, 2008 Ferdinando Ametrano
 Copyright (C) 2007 Roland Lichters

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

/*! \file ratehelpers.hpp
    \brief deposit, FRA, futures, and swap rate helpers
*/

#ifndef quantlib_ratehelpers_hpp
#define quantlib_ratehelpers_hpp

#include <ql/termstructures/bootstraphelper.hpp>
#include <ql/instruments/vanillaswap.hpp>
#include <ql/instruments/bmaswap.hpp>
#include <ql/time/calendar.hpp>
#include <ql/time/daycounter.hpp>

namespace QuantLib {

    class SwapIndex;
    class Quote;

    typedef BootstrapHelper<YieldTermStructure> RateHelper;

    //! Rate helper for bootstrapping over IborIndex futures prices
    class FuturesRateHelper : public RateHelper {
      public:
        FuturesRateHelper(const Handle<Quote>& price,
                          const Date& immDate,
                          Size nMonths,
                          const Calendar& calendar,
                          BusinessDayConvention convention,
                          bool endOfMonth,
                          const DayCounter& dayCounter,
                          const Handle<Quote>& convexityAdjustment = Handle<Quote>());
        FuturesRateHelper(Real price,
                          const Date& immDate,
                          Size nMonths,
                          const Calendar& calendar,
                          BusinessDayConvention convention,
                          bool endOfMonth,
                          const DayCounter& dayCounter,
                          Rate convexityAdjustment = 0.0);
        FuturesRateHelper(const Handle<Quote>& price,
                          const Date& immDate,
                          const boost::shared_ptr<IborIndex>& iborIndex,
                          const Handle<Quote>& convexityAdjustment = Handle<Quote>());
        FuturesRateHelper(Real price,
                          const Date& immDate,
                          const boost::shared_ptr<IborIndex>& iborIndex,
                          Rate convexityAdjustment = 0.0);
        //! \name RateHelper interface
        //@{
        Real impliedQuote() const;
        //@}
        //! \name FuturesRateHelper inspectors
        //@{
        Real convexityAdjustment() const;
        //@}
      private:
        Time yearFraction_;
        Handle<Quote> convAdj_;
    };

    //! Rate helper with date schedule relative to the global evaluation date
    /*! This class takes care of rebuilding the date schedule when the global
        evaluation date changes
    */
    class RelativeDateRateHelper : public RateHelper {
      public:
        RelativeDateRateHelper(const Handle<Quote>& quote);
        RelativeDateRateHelper(Real quote);
        //! \name Observer interface
        //@{
        void update();
        //@}
      protected:
        virtual void initializeDates() = 0;
        Date evaluationDate_;
    };


    //! Rate helper for bootstrapping over deposit rates
    class DepositRateHelper : public RelativeDateRateHelper {
      public:
        DepositRateHelper(const Handle<Quote>& rate,
                          const Period& tenor,
                          Natural fixingDays,
                          const Calendar& calendar,
                          BusinessDayConvention convention,
                          bool endOfMonth,
                          const DayCounter& dayCounter);
        DepositRateHelper(Rate rate,
                          const Period& tenor,
                          Natural fixingDays,
                          const Calendar& calendar,
                          BusinessDayConvention convention,
                          bool endOfMonth,
                          const DayCounter& dayCounter);
        DepositRateHelper(const Handle<Quote>& rate,
                          const boost::shared_ptr<IborIndex>& iborIndex);
        DepositRateHelper(Rate rate,
                          const boost::shared_ptr<IborIndex>& iborIndex);
        //! \name RateHelper interface
        //@{
        Real impliedQuote() const;
        void setTermStructure(YieldTermStructure*);
        //@}
      private:
        void initializeDates();
        Date fixingDate_;
        boost::shared_ptr<IborIndex> iborIndex_;
        RelinkableHandle<YieldTermStructure> termStructureHandle_;
    };


    //! Rate helper for bootstrapping over %FRA rates
    class FraRateHelper : public RelativeDateRateHelper {
      public:
        FraRateHelper(const Handle<Quote>& rate,
                      Natural monthsToStart,
                      Natural monthsToEnd,
                      Natural fixingDays,
                      const Calendar& calendar,
                      BusinessDayConvention convention,
                      bool endOfMonth,
                      const DayCounter& dayCounter);
        FraRateHelper(Rate rate,
                      Natural monthsToStart,
                      Natural monthsToEnd,
                      Natural fixingDays,
                      const Calendar& calendar,
                      BusinessDayConvention convention,
                      bool endOfMonth,
                      const DayCounter& dayCounter);
        FraRateHelper(const Handle<Quote>& rate,
                      Natural monthsToStart,
                      const boost::shared_ptr<IborIndex>& iborIndex);
        FraRateHelper(Rate rate,
                      Natural monthsToStart,
                      const boost::shared_ptr<IborIndex>& iborIndex);
        //! \name RateHelper interface
        //@{
        Real impliedQuote() const;
        void setTermStructure(YieldTermStructure*);
        //@}
      private:
        void initializeDates();
        Date fixingDate_;
        Natural monthsToStart_;
        boost::shared_ptr<IborIndex> iborIndex_;
        RelinkableHandle<YieldTermStructure> termStructureHandle_;
    };

    //! Rate helper for bootstrapping over swap rates
    /*! \todo use input SwapIndex to create the swap */
    class SwapRateHelper : public RelativeDateRateHelper {
      public:
        SwapRateHelper(const Handle<Quote>& rate,
                       const boost::shared_ptr<SwapIndex>& swapIndex,
                       const Handle<Quote>& spread = Handle<Quote>(),
                       const Period& fwdStart = 0*Days);
        SwapRateHelper(const Handle<Quote>& rate,
                       const Period& tenor,
                       const Calendar& calendar,
                       // fixed leg
                       Frequency fixedFrequency,
                       BusinessDayConvention fixedConvention,
                       const DayCounter& fixedDayCount,
                       // floating leg
                       const boost::shared_ptr<IborIndex>& iborIndex,
                       const Handle<Quote>& spread = Handle<Quote>(),
                       const Period& fwdStart = 0*Days);
        SwapRateHelper(Rate rate,
                       const Period& tenor,
                       const Calendar& calendar,
                       // fixed leg
                       Frequency fixedFrequency,
                       BusinessDayConvention fixedConvention,
                       const DayCounter& fixedDayCount,
                       // floating leg
                       const boost::shared_ptr<IborIndex>& iborIndex,
                       const Handle<Quote>& spread = Handle<Quote>(),
                       const Period& fwdStart = 0*Days);
        SwapRateHelper(Rate rate,
                       const boost::shared_ptr<SwapIndex>& swapIndex,
                       const Handle<Quote>& spread = Handle<Quote>(),
                       const Period& fwdStart = 0*Days);
        //! \name RateHelper interface
        //@{
        Real impliedQuote() const;
        void setTermStructure(YieldTermStructure*);
        //@}
        //! \name SwapRateHelper inspectors
        //@{
        Spread spread() const;
        boost::shared_ptr<VanillaSwap> swap() const;
        const Period& forwardStart() const;
        //@}
      protected:
        void initializeDates();
        Period tenor_;
        Calendar calendar_;
        BusinessDayConvention fixedConvention_;
        Frequency fixedFrequency_;
        DayCounter fixedDayCount_;
        boost::shared_ptr<IborIndex> iborIndex_;
        boost::shared_ptr<VanillaSwap> swap_;
        RelinkableHandle<YieldTermStructure> termStructureHandle_;
        Handle<Quote> spread_;
        Period fwdStart_;
    };

    //! Rate helper for bootstrapping over BMA swap rates
    class BMASwapRateHelper : public RelativeDateRateHelper {
      public:
        BMASwapRateHelper(const Handle<Quote>& liborFraction,
                          const Period& tenor, // swap maturity
                          Natural settlementDays,
                          const Calendar& calendar,
                          // bma leg
                          const Period& bmaPeriod,
                          BusinessDayConvention bmaConvention,
                          const DayCounter& bmaDayCount,
                          const boost::shared_ptr<BMAIndex>& bmaIndex,
                          // ibor leg
                          const boost::shared_ptr<IborIndex>& index);
        //! \name RateHelper interface
        //@{
        Real impliedQuote() const;
        void setTermStructure(YieldTermStructure*);
        //@}
    protected:
        void initializeDates();
        Period tenor_;
        Natural settlementDays_;
        Calendar calendar_;
        Period bmaPeriod_;
        BusinessDayConvention bmaConvention_;
        DayCounter bmaDayCount_;
        boost::shared_ptr<BMAIndex> bmaIndex_;
        boost::shared_ptr<IborIndex> iborIndex_;

        boost::shared_ptr<BMASwap> swap_;
        RelinkableHandle<YieldTermStructure> termStructureHandle_;
    };

}

#endif