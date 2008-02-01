/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2003, 2004 Ferdinando Ametrano
 Copyright (C) 2000, 2001, 2002, 2003 RiskMap srl

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

#include <ql/pricingengines/asian/mc_discr_geom_av_price.hpp>

namespace QuantLib {

    GeometricAPOPathPricer::GeometricAPOPathPricer(
                                         Option::Type type,
                                         Real strike, DiscountFactor discount,
                                         Real runningProduct, Size pastFixings)
    : payoff_(type, strike), discount_(discount),
      runningProduct_(runningProduct), pastFixings_(pastFixings) {
        QL_REQUIRE(strike>=0.0, "negative strike given");
    }

}