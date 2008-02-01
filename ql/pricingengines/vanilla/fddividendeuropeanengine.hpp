/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2005 StatPro Italia srl

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

/*! \file fddividendeuropeanengine.hpp
    \brief finite-differences engine for European option with dividends
*/

#ifndef quantlib_fd_dividend_european_engine_hpp
#define quantlib_fd_dividend_european_engine_hpp

#include <ql/instruments/dividendvanillaoption.hpp>
#include <ql/pricingengines/vanilla/fddividendengine.hpp>

namespace QuantLib {

    //! Finite-differences pricing engine for dividend European options
    /*! \ingroup vanillaengines

        \test
        - the correctness of the returned greeks is tested by
          reproducing numerical derivatives.
        - the invariance of the results upon addition of null
          dividends is tested.
    */
    typedef FDEngineAdapter<FDDividendEngine,
                            DividendVanillaOption::engine>
    FDDividendEuropeanEngine;

    typedef FDEngineAdapter<FDDividendEngineMerton73,
                            DividendVanillaOption::engine>
    FDDividendEuropeanEngineMerton73;

    typedef FDEngineAdapter<FDDividendEngineShiftScale,
                            DividendVanillaOption::engine>
    FDDividendEuropeanEngineShiftScale;
}


#endif