/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2006 Ferdinando Ametrano
 Copyright (C) 2007 Eric Ehlers

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

#ifndef qla_capletvolstructure_hpp
#define qla_capletvolstructure_hpp

#include <qlo/termstructures.hpp>
#include <ql/termstructures/yieldtermstructure.hpp>
#include <ql/termstructures/volatility/optionlet/optionletstripper.hpp>
#include <ql/termstructures/volatility/optionlet/strippedoptionletadapter.hpp>

namespace QuantLib {
    class OptionletVolatilityStructure;
    class Period;
    class SmileSection;
    class IborIndex;
    class CapFloorTermVolSurface;
    class CapFloorTermVolCurve;
    class OptionletStripper1;
    class OptionletStripper2;
    class StrippedOptionletAdapter;
    class StrippedOptionlet;
}

namespace QuantLibAddin {
    
    OH_OBJ_CLASS(OptionletVolatilityStructure, TermStructure);

    class ConstantOptionletVolatility : public OptionletVolatilityStructure {
      public:
        ConstantOptionletVolatility(
                        const boost::shared_ptr<ObjectHandler::ValueObject>&,
                        QuantLib::Natural settlementDays,
                        const QuantLib::Handle<QuantLib::Quote>& volatility,
                        const QuantLib::DayCounter& dayCounter,
                        const QuantLib::Calendar& cal,
                        QuantLib::BusinessDayConvention bdc,
                        bool permanent);
    };
      
    class SpreadedOptionletVolatility : public OptionletVolatilityStructure {
      public:
        SpreadedOptionletVolatility(
            const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
            const QuantLib::Handle<QuantLib::OptionletVolatilityStructure>&,
            const QuantLib::Handle<QuantLib::Quote>&,
            bool permanent);

    };

    class StrippedOptionletAdapter : public OptionletVolatilityStructure {
      public:
        StrippedOptionletAdapter(
                    const boost::shared_ptr<ObjectHandler::ValueObject>&,
                    const boost::shared_ptr<QuantLib::StrippedOptionletBase>&,
                    bool permanent);
    };

    OH_OBJ_CLASS(CapFloorTermVolatilityStructure, TermStructure);

    class CapFloorTermVolCurve : public CapFloorTermVolatilityStructure {
      public:
        CapFloorTermVolCurve(
                      const boost::shared_ptr<ObjectHandler::ValueObject>&,
                      QuantLib::Natural settlementDays,
                      const QuantLib::Calendar& calendar,
                      const std::vector<QuantLib::Period>& optionTenors,
                      const std::vector<QuantLib::Handle<QuantLib::Quote> >&,
                      const QuantLib::DayCounter& dayCounter,
                      bool permanent);
    };

    class CapFloorTermVolSurface : public CapFloorTermVolatilityStructure {
      public:
        CapFloorTermVolSurface(
          const boost::shared_ptr<ObjectHandler::ValueObject>&,
          QuantLib::Natural settlementDays,
          const QuantLib::Calendar& calendar,
          const std::vector<QuantLib::Period>& optionTenors,
          const std::vector<QuantLib::Rate>& strikes,
          const std::vector<std::vector<QuantLib::Handle<QuantLib::Quote> > >&,
          const QuantLib::DayCounter& dc,
          bool permanent);
    };

        
    class StrippedOptionletBase : public ObjectHandler::LibraryObject<QuantLib::StrippedOptionletBase> {
      public:
        StrippedOptionletBase(
                        const boost::shared_ptr<ObjectHandler::ValueObject>&,
                        bool permanent);
    };
        
    class StrippedOptionlet : public StrippedOptionletBase {
      public:
        StrippedOptionlet(
        const boost::shared_ptr<ObjectHandler::ValueObject>&,
        const QuantLib::Date& referenceDate,
        const QuantLib::Calendar& calendar,
        QuantLib::Natural settlementDays,
        QuantLib::BusinessDayConvention businessDayConvention,
        const boost::shared_ptr<QuantLib::IborIndex>& index,
        const std::vector<QuantLib::Period>& optionletTenors,
        const std::vector<QuantLib::Rate>& strikes,
        const std::vector<std::vector<QuantLib::Handle<QuantLib::Quote> > >&,
        const QuantLib::DayCounter& dc,
        bool permanent);
    };

    OH_OBJ_CLASS(OptionletStripper, StrippedOptionletBase);

    class OptionletStripper1 : public OptionletStripper {
      public:
        OptionletStripper1(
                    const boost::shared_ptr<ObjectHandler::ValueObject>&,
                    const boost::shared_ptr<QuantLib::CapFloorTermVolSurface>&,
                    const boost::shared_ptr<QuantLib::IborIndex>& index,
                    QuantLib::Rate switchStrike,
                    QuantLib::Real accuracy,
                    bool permanent);
    };

    class OptionletStripper2 : public OptionletStripper {
      public:
        OptionletStripper2(
                    const boost::shared_ptr<ObjectHandler::ValueObject>&,
                    const boost::shared_ptr<QuantLib::OptionletStripper1>&,
                    const QuantLib::Handle<QuantLib::CapFloorTermVolCurve>&,
                    bool permanent);
    };

}

#endif