
/*
 Copyright (C) 2006 Eric Ehlers

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

#if defined(HAVE_CONFIG_H)     // Dynamically created by configure
    #include <qlo/config.hpp>
#endif
#include <qlo/quantovanillaoption.hpp>

#include <ql/instruments/quantovanillaoption.hpp>

namespace QuantLibAddin {

    QuantoVanillaOption::QuantoVanillaOption(
            const boost::shared_ptr<ObjectHandler::ValueObject>& properties,
            const boost::shared_ptr<QuantLib::StrikedTypePayoff> &payoff,
            const boost::shared_ptr < QuantLib::Exercise > &exercise,
            bool permanent) : OneAssetOption(properties, permanent) {
        libraryObject_ = boost::shared_ptr<QuantLib::Instrument>(new
            QuantLib::QuantoVanillaOption(payoff,
                                          exercise));
    }

}