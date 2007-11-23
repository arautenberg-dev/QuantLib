
/*
 Copyright (C) 2004, 2005, 2006, 2007 Eric Ehlers

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

#include <Addins/Cpp/addincpp.hpp>
#include <oh/ohdefines.hpp>
#if defined BOOST_MSVC
#include <oh/auto_link.hpp>
#endif


using namespace QuantLibAddinCpp;

int main() {

    try {

        initializeAddin();

        ohSetLogFile("qlademo.log", 4L, ObjectHandler::Variant());
        ohSetConsole(1, 4L, ObjectHandler::Variant());
        ohLogMessage("Begin example program.", 4L, ObjectHandler::Variant());
        ohLogMessage(qlAddinVersion(ObjectHandler::Variant()), 4L, ObjectHandler::Variant());
        ohLogMessage(ohVersion(ObjectHandler::Variant()), 4L, ObjectHandler::Variant());

        std::string daycountConvention = "Actual/365 (Fixed)";
        std::string payoffType = "Vanilla";
        std::string optionType = "Put";
        std::string engineType = "AE";      // Analytic European
        std::string xmlFileName = "option_demo.xml";
        double dividendYield = 0.00;
        double riskFreeRate = 0.06;
        double volatility = 0.20;
        double underlying = 36;
        double strike = 40;
        long evaluationDate = 35930;        // 15 May 1998
        long settlementDate = 35932;        // 17 May 1998
        long exerciseDate = 36297;          // 17 May 1999

        qlSettingsSetEvaluationDate(evaluationDate, ObjectHandler::Variant());

        std::string idBlackConstantVol = qlBlackConstantVol(
            "my_blackconstantvol",
            settlementDate,
            volatility,
            daycountConvention,
            ObjectHandler::Variant(),
            ObjectHandler::Variant(),
            false);

        std::string idGeneralizedBlackScholesProcess = qlGeneralizedBlackScholesProcess(
            "my_blackscholes",
            idBlackConstantVol,
            underlying,
            daycountConvention,
            settlementDate,
            riskFreeRate,
            dividendYield,
            ObjectHandler::Variant(),
            ObjectHandler::Variant(),
            false);

        std::string idStrikedTypePayoff = qlStrikedTypePayoff(
            "my_payoff",
            payoffType,
            optionType,
            strike,
            strike,
            ObjectHandler::Variant(),
            ObjectHandler::Variant(),
            false);

        std::string idExercise = qlEuropeanExercise(
            "my_exercise",
            exerciseDate,
            ObjectHandler::Variant(),
            ObjectHandler::Variant(),
            false);

        std::string idPricingEngine = qlPricingEngine(
            "my_engine",
            engineType,
            ObjectHandler::Variant(),
            ObjectHandler::Variant(),
            false);

        std::string idVanillaOption = qlVanillaOption(
            "my_option",
            idGeneralizedBlackScholesProcess,
            idStrikedTypePayoff,
            idExercise,
            idPricingEngine,
            ObjectHandler::Variant(),
            ObjectHandler::Variant(),
            false);

        std::ostringstream s;
        s << "option NPV() = " << qlInstrumentNPV(idVanillaOption,ObjectHandler::Variant());
        ohLogMessage(s.str(), 4L, ObjectHandler::Variant());

        ohLogObject(idVanillaOption, ObjectHandler::Variant());

        std::vector<std::string> idList;
        idList.push_back(idBlackConstantVol);
        idList.push_back(idGeneralizedBlackScholesProcess);
        idList.push_back(idStrikedTypePayoff);
        idList.push_back(idExercise);
        idList.push_back(idPricingEngine);
        idList.push_back(idVanillaOption);
        ohObjectSave(idList, xmlFileName, ObjectHandler::Variant(), ObjectHandler::Variant());

        ohLogMessage("End example program.", 4L, ObjectHandler::Variant());

        return 0;
    } catch (const std::exception &e) {
        std::ostringstream s;
        s << "Error: " << e.what();
        ohLogMessage(s.str(), 1L, ObjectHandler::Variant());
        return 1;
    } catch (...) {
        ohLogMessage("unknown error", 1L, ObjectHandler::Variant());
        return 1;
    }

}
