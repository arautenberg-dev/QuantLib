
/*
 Copyright (C) 2005, 2006 Eric Ehlers
 Copyright (C) 2006 Ferdinando Ametrano
 Copyright (C) 2005 Aurelien Chanudet
 Copyright (C) 2005 Plamen Neykov
 Copyright (C) 2006 Katiuscia Manzoni

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

#if defined(HAVE_CONFIG_H)
    #include <qlo/config.hpp>
#endif

#include <qlo/vanillaswap.hpp>
#include <ql/instruments/makevanillaswap.hpp>
#include <ql/indexes/swapindex.hpp>
#include <ql/termstructures/yield/ratehelpers.hpp>

namespace QuantLibAddin {

    VanillaSwap::VanillaSwap(
            const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
            QuantLib::VanillaSwap::Type type,
            QuantLib::Real nominal,
            const boost::shared_ptr<QuantLib::Schedule>& fixedSchedule,
            QuantLib::Rate fixRate,
            const QuantLib::DayCounter& fixDayCounter,
            const boost::shared_ptr<QuantLib::Schedule>& floatSchedule,
            const boost::shared_ptr<QuantLib::IborIndex>& index,
            QuantLib::Spread spread,
            const QuantLib::DayCounter& floatDayCounter,
            bool permanent)
    : Swap(properties, permanent)
    {
        libraryObject_ = boost::shared_ptr<QuantLib::Instrument>(new
            QuantLib::VanillaSwap(type,
                                  nominal,
                                  *fixedSchedule,
                                  fixRate,
                                  fixDayCounter,
                                  *floatSchedule,
                                  index,
                                  spread,
                                  floatDayCounter));
    }

    VanillaSwap::VanillaSwap(
            const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
            const QuantLib::Period& fwdStart,
            const QuantLib::Period& swapTenor, 
            QuantLib::Rate fixedRate,
            const QuantLib::DayCounter& fixDayCounter,
            const boost::shared_ptr<QuantLib::IborIndex>& index,
            QuantLib::Spread floatingLegSpread,
            bool permanent)
    : Swap(properties, permanent)
    {
        libraryObject_ =
            QuantLib::MakeVanillaSwap(swapTenor, index, fixedRate, fwdStart)
                .withFloatingLegSpread(floatingLegSpread)
                .withFixedLegDayCount(fixDayCounter)
                .operator boost::shared_ptr<QuantLib::VanillaSwap>();
    }

    VanillaSwap::VanillaSwap(
        const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
        const boost::shared_ptr<QuantLib::SwapIndex>& swapIndex,
        const QuantLib::Date& fixingDate,
        bool permanent)
    : Swap(properties, permanent)
    {
        libraryObject_ = swapIndex->underlyingSwap(fixingDate);
    }

    VanillaSwap::VanillaSwap(
        const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
        const boost::shared_ptr<QuantLib::SwapRateHelper>& swapRH,
        bool permanent)
    : Swap(properties, permanent)
    {
        libraryObject_ = swapRH->swap();
    }


    std::vector<std::vector<boost::any> > VanillaSwap::fixedLegAnalysis() {
        return Swap::legAnalysis(0);
    }

    std::vector<std::vector<boost::any> > VanillaSwap::floatingLegAnalysis() {
        return Swap::legAnalysis(1);
    }

}