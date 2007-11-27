
##########################################################################
# code for Excel
##########################################################################

code11 = '''\
        std::string %(name)sCpp = ObjectHandler::operToScalar<std::string>(
            *%(name)s, "%(name)s", %(defaultValue)s%(errorValue)s);\n'''

code12 = '''\
        %(nativeType)s %(name)sCpp = ObjectHandler::operToScalar<%(nativeType)s>(
            *%(name)s, "%(name)s", %(defaultValue)s%(errorValue)s);\n'''

code12b = '''\
        ObjectHandler::Variant %(name)sCpp = ObjectHandler::operToScalar<ObjectHandler::Variant>(
            *%(name)s, "%(name)s");\n'''

code13 = '''\
        std::vector<%(nativeType)s> %(name)sCpp =
            ObjectHandler::operToVector<%(nativeType)s>(*%(name)s, "%(name)s");\n'''

code14 = '''\
        std::vector<ObjectHandler::Variant> %(name)sCpp =
            ObjectHandler::operToVector<ObjectHandler::Variant>(*%(name)s, "%(name)s");\n'''

code15 = '''\
        std::vector<std::string> %(name)sCpp =
            ObjectHandler::operToVector<std::string>(*%(name)s, "%(name)s");\n'''

code18 = '''\
        std::vector<std::vector<%(type)s> > %(name)sCpp =
            ObjectHandler::operToMatrix<%(type)s>(*%(name)s, "%(name)s");\n'''

code19 = '''\
        std::vector<std::vector<long> > %(name)sCpp =
            ObjectHandler::fpToMatrix<long>(*%(name)s);\n'''

code20 = '''\
        std::vector<std::vector<double> > %(name)sCpp =
            ObjectHandler::fpToMatrix<double>(*%(name)s);\n'''

code21a = '''\
        std::vector<std::vector<std::string> > %(name)sCpp =
            ObjectHandler::operToMatrix<std::string>(*%(name)s, "%(name)s");\n'''

code21b = '''\
        std::vector<std::vector<ObjectHandler::Variant> > %(name)sCpp =
            ObjectHandler::operToMatrix<ObjectHandler::Variant>(*%(name)s, "%(name)s");\n'''

code22 = '''\
        %(type)s %(name)sLib;
        QuantLibAddin::cppToLibrary(%(name)s, %(name)sLib);\n'''

code23 = '''\
        %(type)s %(name)sLib;
        QuantLibAddin::cppToLibrary(*%(name)s, %(name)sLib);\n'''

code24 = '''\
        %(type)s %(name)sLib = ObjectHandler::operToScalar<%(type)s>(
            *%(name)s, "%(name)s");\n'''

code25 = '''\
        %(type)s %(name)sLib = ObjectHandler::operToScalar<%(type)s>(
            %(name)s, "%(name)s", %(defaultValue)s%(errorValue)s);\n'''

code26 = '''\
        %(type)s %(name)sLib = ObjectHandler::operToScalar<%(type)s>(
            *%(name)s, "%(name)s", %(defaultValue)s%(errorValue)s);\n'''

code28 = '''\
        %(type)s %(name)sLib =
            QuantLibXL::operToQlArray(*%(name)s, "%(name)s");\n'''

code29 = '''\
        %(type)s %(name)sLib =
            QuantLibXL::operToQlMatrix(*%(name)s);\n'''

code31 = '''\
        std::vector<%(type)s> %(name)sLib =
            ObjectHandler::operToVector<%(type)s>(*%(name)s, "%(name)s");\n'''

code33 = '''\
        %(type)s %(name)sEnum =
            ObjectHandler::Create<%(type)s>()(%(name)sCpp);\n'''

code34 = '''\
        %(type)s %(name)sEnum =
            ObjectHandler::Create<%(type)s>()(%(name)s);\n'''

code35 = '''\
        std::vector<%(type)s> %(name)sEnum =
            ObjectHandler::vectorStringToEnum<%(type)s>(%(name)sCpp, "%(name)s");\n'''

code36 = '''\
        OH_GET_OBJECT(%(name)sObj, %(name)s, %(type)s)\n'''

code36b = '''\
        OH_GET_OBJECT(%(name)sTemp, %(name)s, ObjectHandler::Object)
        boost::shared_ptr<%(type)s> %(name)sObj =
            %(namespaceObjects)s::CoerceQuoteObject<%(type)s>()(
                %(name)sTemp);\n'''

code36c = '''\
        OH_GET_OBJECT(%(name)sTemp, %(name)s, ObjectHandler::Object)
        boost::shared_ptr<%(type)s> %(name)sObj =
            %(namespaceObjects)s::CoerceTermStructureObject<%(type)s>()(
                %(name)sTemp);\n'''

code37 = '''\
        std::vector<boost::shared_ptr<%(namespaceObjects)s::%(classname)s> > %(name)sObj =
            ObjectHandler::getObjectVector<%(namespaceObjects)s::%(classname)s>(%(name)sCpp);\n'''

code37b = '''\
        std::vector<boost::shared_ptr<%(namespaceLibrary)s::%(classname)s> > %(name)sLibObj =
            ObjectHandler::getLibraryObjectVector<%(namespaceObjects)s::%(classname)s, %(namespaceLibrary)s::%(classname)s>(%(name)sCpp);\n'''

code37c = '''\
        std::vector<boost::shared_ptr<%(namespaceLibrary)s::%(classname)s> > %(name)sLibObj =
            ObjectHandler::getLibraryObjectVector<%(namespaceObjects)s::%(classname)s, %(namespaceLibrary)s::%(classname)s>(%(name)s);\n'''

code38 = '''\
        OH_GET_REFERENCE(%(name)sLibObj, %(name)s,
            %(namespaceObjects)s::%(classname)s, %(namespaceLibrary)s::%(classname)s)\n'''

code39 = '''\
        OH_GET_REFERENCE_DEFAULT(%(name)sLibObj, %(name)sCpp,
            %(namespaceObjects)s::%(classname)s, %(namespaceLibrary)s::%(classname)s)\n'''

code42 = '''\
        std::vector<boost::shared_ptr<%(namespaceLibrary)s::%(classname)s> > %(name)sLibObj =
            ObjectHandler::operToObjectVector<%(namespaceLibrary)s::%(classname)s, %(namespaceObjects)s::%(classname)s>(
            *%(name)s, "%(name)s");\n'''

code43 = '''\
        std::vector<QuantLib::Handle<QuantLib::Quote> > %(name)sLibObj =
            ObjectHandler::operToVector<QuantLib::Handle<QuantLib::Quote> >(*%(name)s, "%(name)s");\n'''

code44a = '''\
        OH_GET_OBJECT_DEFAULT(%(name)sCoerce, %(name)sCpp, ObjectHandler::Object)
        %(namespaceLibrary)s::Handle<%(namespaceLibrary)s::%(classname)s> %(name)sLibObj =
            %(namespaceObjects)s::CoerceHandle<
                %(namespaceObjects)s::%(classname)s,
                %(namespaceLibrary)s::%(classname)s>()(
                    %(name)sCoerce, %(namespaceLibrary)s::Handle<%(namespaceLibrary)s::%(classname)s>());\n'''

code44b = '''\
        OH_GET_OBJECT(%(name)sCoerce, %(name)s, ObjectHandler::Object)
        %(namespaceLibrary)s::Handle<%(namespaceLibrary)s::%(classname)s> %(name)sLibObj =
            %(namespaceObjects)s::CoerceHandle<
                %(namespaceObjects)s::%(classname)s,
                %(namespaceLibrary)s::%(classname)s>()(
                    %(name)sCoerce);\n'''

code45a = '''\
        QuantLib::Handle<QuantLib::Quote> %(name)sLibObj = 
            ObjectHandler::operToScalar<QuantLib::Handle<QuantLib::Quote> >(
                *%(name)s, "%(name)s");\n'''

code45b = '''\
        OH_GET_OBJECT(%(name)sTemp, %(name)s, ObjectHandler::Object)
        boost::shared_ptr<%(namespaceLibrary)s::%(classname)s> %(name)sLibObj =
            %(namespaceObjects)s::CoerceLibrarySame<
                %(namespaceObjects)s::%(classname)s,
                %(namespaceLibrary)s::%(classname)s>()(
                    %(name)sTemp);\n'''

code45c = '''\
        OH_GET_OBJECT(%(name)sTemp, %(name)s, ObjectHandler::Object)
        boost::shared_ptr<%(namespaceLibrary)s::%(classname)s> %(name)sLibObj =
            %(namespaceObjects)s::CoerceQuote<
                %(namespaceObjects)s::%(classname)s,
                %(namespaceLibrary)s::%(classname)s>()(
                    %(name)sTemp);\n'''

code46 = '''\
        OH_GET_OBJECT(%(name)sTemp, %(name)s, ObjectHandler::Object)
        boost::shared_ptr<%(namespaceLibrary)s::%(classname)s> %(name)sLibObj =
            %(namespaceObjects)s::CoerceTermStructure<
                %(namespaceObjects)s::%(classname)s,
                %(namespaceLibrary)s::%(classname)s>()(
                    %(name)sTemp);\n'''

code47 = '''\
        boost::shared_ptr<QuantLib::Quote> %(name)sLibObj =
            ObjectHandler::operToScalar<boost::shared_ptr<QuantLib::Quote> >(
                *%(name)s, "%(name)s");\n'''

code48 = '''\
        OH_GET_UNDERLYING(%(name)sLibObj, %(name)s,
            %(namespaceObjects)s::%(classname)s, %(namespaceLibrary)s::%(classname)s)\n'''

code49 = '''\
        OH_GET_UNDERLYING_NONCONST(%(name)sLibObj, %(name)s,
            %(namespaceObjects)s::%(classname)s, %(namespaceLibrary)s::%(classname)s)\n'''

code50 = '''\
        std::vector<std::vector<QuantLib::Handle<QuantLib::Quote> > > %(name)sLibObj =
            ObjectHandler::operToMatrix<QuantLib::Handle<QuantLib::Quote> >(*%(name)s, "%(name)s");\n'''

code51 = '''\
std::vector<std::vector<std::string> > returnValue = '''

code52 = '''\
std::vector<std::vector<%(type)s> > returnValue = '''

code53 = '''\
std::vector<std::vector<boost::any> > returnValue = '''

code54 = '''\
            %(name)sLib.begin(),
            %(name)sLib.end()'''

code55 = '''\
            %(name)sCpp.begin(),
            %(name)sCpp.end()'''

code56a = '''\
        std::string str = %(namespaceObjects)s::libraryToScalar(returnValue);
        static char ret[XL_MAX_STR_LEN];
        ObjectHandler::stringToChar(str, ret);
        return ret;'''
 
code56b = '''\
        static %(nativeType)s returnValueXL;
        returnValueXL = %(namespaceObjects)s::libraryToScalar(returnValue);
        return &returnValueXL;'''

code57 = '''\
        std::ostringstream os;
        os << returnValue;
        static char ret[XL_MAX_STR_LEN];
        ObjectHandler::stringToChar(os.str(), ret);
        return ret;'''

code58 = '''\
        static char ret[XL_MAX_STR_LEN];
        ObjectHandler::stringToChar(returnValue, ret);
        return ret;'''

code59 = '''\
        static OPER xRet;
        ObjectHandler::%(tensorRank)sToOper(returnValue, xRet);
        return &xRet;'''

code60 = '''\
        static OPER xRet;
        ObjectHandler::scalarToOper2(returnValue, xRet);
        return &xRet;'''

code60b = '''\
        static OPER xRet;
        ObjectHandler::matrixToOper(returnValue, xRet);
        return &xRet;'''

code61 = '''\
        return &returnValue;'''

code62 = '''\
        std::vector<%(nativeType)s> returnValVec = %(namespaceObjects)s::libraryToVector(returnValue);
        static OPER xRet;
        ObjectHandler::vectorToOper(returnValVec, xRet);
        return &xRet;'''

code63 = '''\
        std::vector<std::string> returnValVec = %(namespaceObjects)s::libraryToVector(returnValue);
        static OPER xRet;
        ObjectHandler::vectorToOper(returnValVec, xRet);
        return &xRet;'''

code63b = '''\
        std::vector<%(nativeType)s> returnValVec =
            %(namespaceObjects)s::convertVector<%(type)s, %(nativeType)s>(returnValue);
        static OPER xRet;
        ObjectHandler::vectorToOper(returnValVec, xRet);
        return &xRet;'''

code64 = '''\
        static OPER xRet;
        ObjectHandler::%(tensorRank)sToOper(returnValue, xRet);
        return &xRet;'''

code65 = '''\
        std::string %(name)sStrip = ObjectHandler::ObjectXL::getStub(%(name)s);'''

code65b = '''\
        std::string %(name)sStrip = ObjectHandler::ObjectXL::getStub(%(name)sCpp);'''

##########################################################################
# code for C++
##########################################################################

code200 = '''\
        %(type)s %(name)sLib = ObjectHandler::ohVariantToScalar<%(type)s>(
            %(name)s, "%(name)s");\n'''

code201 = '''\
        %(type)s %(name)sLib = ObjectHandler::ohVariantToScalar<%(type)s>(
            %(name)s, "%(name)s", %(defaultValue)s%(errorValue)s);\n'''

code203 = '''\
        /*%(type)s %(name)sLib =
            QuantLibXL::operToQlArray(%(name)s, "%(name)s");*/\n'''

code204 = '''\
        std::vector<QuantLib::Date> %(name)sLib = 
            ObjectHandler::ohVariantToVector<QuantLib::Date>(%(name)s, "%(name)s");\n'''

code205 = '''\
        std::vector<%(type)s> %(name)sLib =
            ObjectHandler::ohVariantToVector<%(type)s>(%(name)s, "%(name)s");\n'''

code206 = '''\
        %(type)s %(name)sLib =
            QuantLibAddin::vvToQlMatrix(%(name)s);\n'''

code207 = '''\
        std::string %(name)sCpp = ObjectHandler::ohVariantToScalar<std::string>(
            %(name)s, "%(name)s", %(defaultValue)s%(errorValue)s);\n'''

code208 = '''\
        %(nativeType)s %(name)sCpp = ObjectHandler::ohVariantToScalar<%(nativeType)s>(
            %(name)s, "%(name)s", %(defaultValue)s%(errorValue)s);\n'''

code209 = '''\
        std::vector<%(nativeType)s> returnValueLib = %(namespaceObjects)s::libraryToVector(returnValue);
        return returnValueLib;'''

code209b = '''\
        std::vector<%(nativeType)s> returnValueLib =
            %(namespaceObjects)s::convertVector<%(type)s, %(nativeType)s>(returnValue);
        return returnValueLib;'''

code210 = '''\
        %(nativeType)s returnValueLib = %(namespaceObjects)s::libraryToScalar(returnValue);
        return returnValueLib;'''

code211 = '''\
        std::ostringstream os;
        os << returnValue;
        return os.str();'''

code213 = '''\
        %(type)s %(name)sLib = ObjectHandler::ohVariantToScalar<%(type)s>(
            %(name)s, "%(name)s", %(defaultValue)s%(errorValue)s);\n'''

code214 = '''\
        QuantLib::Handle<QuantLib::Quote> %(name)sLibObj = 
            ObjectHandler::ohVariantToScalar<QuantLib::Handle<QuantLib::Quote> >(
                %(name)s, "%(name)s");\n'''

code215 = '''\
        std::vector<std::vector<QuantLib::Handle<QuantLib::Quote> > > %(name)sLibObj =
            ObjectHandler::ohVariantToMatrix<QuantLib::Handle<QuantLib::Quote> >(%(name)s, "%(name)s");\n'''

code218 = '''\
        std::vector<QuantLib::Handle<QuantLib::Quote> > %(name)sLibObj =
            ObjectHandler::ohVariantToVector<QuantLib::Handle<QuantLib::Quote> >(%(name)s, "%(name)s");\n'''

##########################################################################
# code for Calc
##########################################################################

code70 = '''\
        std::string %(name)sCpp = ouStringToStlString(%(name)s);\n'''

code71 = '''\
        std::string %(name)sCpp;
        calcToScalar(%(name)sCpp, %(name)s);\n'''

code71b = '''\
        boost::any %(name)sCpp;
        calcToScalar(%(name)sCpp, %(name)s);\n'''

code72 = '''\
        %(nativeType)s %(name)sCpp;
        calcToScalar(%(name)sCpp, %(name)s);\n'''

code73 = '''\
        std::vector<std::string> %(name)sCpp;
        calcToVector(%(name)sCpp, %(name)s);\n'''

code73b = '''\
        std::vector<boost::any> %(name)sCpp;
        calcToVector(%(name)sCpp, %(name)s);\n'''

code74 = '''\
        std::vector<%(nativeType)s> %(name)sCpp;
        calcToVector(%(name)sCpp, %(name)s);\n'''

code75 = '''\
        std::vector<std::vector<%(nativeType)s> > %(name)sCpp;
        calcToMatrix(%(name)sCpp, %(name)s);\n'''

code76 = '''\
        std::vector<%(type)s> %(name)sLib;
        calcToVector(%(name)sLib, %(name)s);\n'''

code77 = '''\
        %(type)s %(name)sLib;
        calcToVector(%(name)sLib, %(name)s);\n'''

code78 = '''\
        std::vector<std::string> %(name)sLib;
        calcToVector(%(name)sLib, %(name)s);\n'''

code79 = '''\
        std::vector<%(type)s> %(name)sLib;
        calcToVector(%(name)sLib, %(name)s);\n'''

code80 = '''\
        OH_GET_REFERENCE(%(name)sLibObj, %(name)sCpp,
            %(namespaceObjects)s::%(classname)s, %(namespaceLibrary)s::%(classname)s)\n'''

code81 = '''\
        OH_GET_REFERENCE(%(name)sLibObj, %(name)sCpp,
            %(namespaceObjects)s::%(classname)s, %(namespaceLibrary)s::%(classname)s)\n'''

code82 = '''\
        %(type)s %(name)sEnum =
            ObjectHandler::Create<%(type)s>()(%(name)sCpp);\n'''

code83 = '''\
        sal_Int32 returnValueCalc;
        scalarToCalc(returnValueCalc, returnValue);
        return returnValueCalc;\n'''

code84 = '''\
        STRING returnValueCalc;
        scalarToCalc(returnValueCalc, returnValue);
        return returnValueCalc;\n'''

code85 = '''\
        ANY returnValueCalc;
        scalarToCalc(returnValueCalc, returnValue);
        return returnValueCalc;\n'''

code86 = '''\
        %(nativeType)s returnValueCalc;
        scalarToCalc(returnValueCalc, returnValue);
        return returnValueCalc;\n'''

code87 = '''\
        %(nativeType)s returnValueCalc;
        scalarToCalc(returnValueCalc, returnValue);
        return returnValueCalc;\n'''

code88 = '''\
        SEQSEQ(sal_Int32) returnValueCalc;
        %(tensorRank)sToCalc(returnValueCalc, returnValue);
        return returnValueCalc;\n'''

code89 = '''\
        SEQSEQ(STRING) returnValueCalc;
        %(tensorRank)sToCalc(returnValueCalc, returnValue);
        return returnValueCalc;\n'''

code90 = '''\
        SEQSEQ(ANY) returnValueCalc;
        %(tensorRank)sToCalc(returnValueCalc, returnValue);
        return returnValueCalc;\n'''

code91 = '''\
        SEQSEQ(%(nativeType)s) returnValueCalc;
        %(tensorRank)sToCalc(returnValueCalc, returnValue);
        return returnValueCalc;\n'''

code92 = '''\
        SEQSEQ(%(nativeType)s) returnValueCalc;
        %(tensorRank)sToCalc(returnValueCalc, returnValue);
        return returnValueCalc;\n'''

code93 = '''\
        OH_GET_OBJECT(%(name)sCoerce, %(name)sTemp, ObjectHandler::Object)
        %(namespaceLibrary)s::Handle<%(namespaceLibrary)s::%(classname)s> %(name)sLibObj =
            %(namespaceObjects)s::CoerceHandle<
                %(namespaceObjects)s::%(classname)s,
                %(namespaceLibrary)s::%(classname)s>()(
                    %(name)sCoerce);\n'''

code94 = '''\
        %(type)s %(name)sLib = calcToQlMatrix(%(name)s);\n'''

code95 = '''\
        OH_GET_OBJECT(%(name)sObj, %(name)sCpp, %(namespaceObjects)s::%(object)s)\n'''

code96 = '''\
        %(type)s %(name)sLib;
        calcToScalar(%(name)sLib, %(name)s);\n'''

##########################################################################
# code for ValueObjects
##########################################################################

code66 = '''\
        else if(strcmp(nameUpper.c_str(), "%(nameUpper)s")==0)
            return %(name)s_;\n'''

code67a = '''\
        else if(strcmp(nameUpper.c_str(), "%(nameUpper)s")==0)
            %(name)s_ = boost::any_cast<ObjectHandler::Variant>(value);\n'''

code67b = '''\
        else if(strcmp(nameUpper.c_str(), "%(nameUpper)s")==0)
            %(name)s_ = boost::any_cast<std::string>(value);\n'''

code67c = '''\
        else if(strcmp(nameUpper.c_str(), "%(nameUpper)s")==0)
            %(name)s_ = boost::any_cast<%(nativeType)s>(value);\n'''

code67d = '''\
        else if(strcmp(nameUpper.c_str(), "%(nameUpper)s")==0)
            %(name)s_ = boost::any_cast<std::vector<ObjectHandler::Variant> >(value);\n'''

code67e = '''\
        else if(strcmp(nameUpper.c_str(), "%(nameUpper)s")==0)
            %(name)s_ = boost::any_cast<std::vector<std::string> >(value);\n'''

code67f = '''\
        else if(strcmp(nameUpper.c_str(), "%(nameUpper)s")==0)
            %(name)s_ = boost::any_cast<std::vector<%(nativeType)s> >(value);\n'''

code67g = '''\
        else if(strcmp(nameUpper.c_str(), "%(nameUpper)s")==0)
            %(name)s_ = boost::any_cast<std::vector<std::vector<ObjectHandler::Variant> > >(value);\n'''

code67h = '''\
        else if(strcmp(nameUpper.c_str(), "%(nameUpper)s")==0)
            %(name)s_ = boost::any_cast<std::vector<std::vector<std::string> > >(value);\n'''

code67i = '''\
        else if(strcmp(nameUpper.c_str(), "%(nameUpper)s")==0)
            %(name)s_ = boost::any_cast<std::vector<std::vector<%(nativeType)s> > >(value);\n'''

##########################################################################
# code for Serialization
##########################################################################

code110 = '''\
        ObjectHandler::Variant %(name)s =
            boost::any_cast<ObjectHandler::Variant>(valueObject->getProperty("%(name)s"));\n'''

code111 = '''\
        std::string %(name)s =
            boost::any_cast<std::string>(valueObject->getProperty("%(name)s"));\n'''

code112 = '''\
        %(nativeType)s %(name)s =
            boost::any_cast<%(nativeType)s>(valueObject->getProperty("%(name)s"));\n'''

code113 = '''\
        std::vector<%(nativeType)s> %(name)s =
            boost::any_cast<std::vector<%(nativeType)s> >(valueObject->getProperty("%(name)s"));\n'''

code114 = '''\
        std::vector<ObjectHandler::Variant> %(name)s =
            boost::any_cast<std::vector<ObjectHandler::Variant> >(valueObject->getProperty("%(name)s"));\n'''

code115 = '''\
        std::vector<std::string> %(name)s =
            boost::any_cast<std::vector<std::string> >(valueObject->getProperty("%(name)s"));\n'''

code116 = '''\
        std::vector<std::vector<std::string> > %(name)s =
            boost::any_cast<std::vector<std::vector<std::string> > >(valueObject->getProperty("%(name)s"));\n'''

code117 = '''\
        std::vector<std::vector<%(nativeType)s> > %(name)s =
            boost::any_cast<std::vector<std::vector<%(nativeType)s> > >(valueObject->getProperty("%(name)s"));\n'''

code118 = '''\
        std::vector<std::vector<ObjectHandler::Variant> > %(name)s =
            boost::any_cast<std::vector<std::vector<ObjectHandler::Variant> > >(valueObject->getProperty("%(name)s"));\n'''

code121 = '''\
        %(type)s %(name)sLib = ObjectHandler::ohVariantToScalar<%(type)s>(
            %(name)s, "%(name)s");\n'''

code122 = '''\
        %(type)s %(name)sLib = ObjectHandler::ohVariantToScalar<%(type)s>(
            %(name)s, "%(name)s", %(defaultValue)s);\n'''

code123 = '''\
        std::string %(name)sCpp = ObjectHandler::ohVariantToScalar<std::string>(
            %(name)s, "%(name)s", %(defaultValue)s);\n'''

code124 = '''\
        %(nativeType)s %(name)sCpp = ObjectHandler::ohVariantToScalar<%(nativeType)s>(
            %(name)s, "%(name)s", %(defaultValue)s);\n'''

code131 = '''\
        std::vector<%(type)s> %(name)sLib =
            ObjectHandler::ohVariantToVector<%(type)s>(%(name)s, "%(name)s");\n'''

code132 = '''\
        std::vector<%(type)s> %(name)sLib = 
            QuantLibAddin::convertVector<%(nativeType)s, %(type)s>(%(name)s);\n'''

code133 = '''\
        std::vector<%(type)s> %(name)sLib = 
            QuantLibAddin::convertVector<%(nativeType)s, %(type)s>(%(name)sCpp);\n'''

code135 = '''\
        QuantLib::Matrix %(name)sLib =
            QuantLibAddin::vvToQlMatrix(%(name)s);\n'''

code140 = '''\
        QuantLib::Handle<QuantLib::Quote> %(name)sLibObj = 
            ObjectHandler::ohVariantToScalar<QuantLib::Handle<QuantLib::Quote> >(
                %(name)s, "%(name)s");\n'''

code150 = '''\
        std::vector<QuantLib::Handle<QuantLib::Quote> > %(name)sLibObj =
            ObjectHandler::ohVariantToVector<QuantLib::Handle<QuantLib::Quote> >(%(name)s, "%(name)s");\n'''

code151 = '''\
        std::vector<std::vector<QuantLib::Handle<QuantLib::Quote> > > %(name)sLibObj =
            ObjectHandler::ohVariantToMatrix<QuantLib::Handle<QuantLib::Quote> >(%(name)s, "%(name)s");\n'''

code152 = '''\
        boost::shared_ptr<QuantLib::Quote> %(name)sLibObj =
            ObjectHandler::ohVariantToScalar<boost::shared_ptr<QuantLib::Quote> >(
                %(name)s, "%(name)s");\n'''

code153 = '''\
        std::vector<boost::shared_ptr<%(namespaceLibrary)s::%(classname)s> > %(name)sLibObj =
            ObjectHandler::ohVariantToObjectVector<%(namespaceLibrary)s::%(classname)s, %(namespaceObjects)s::%(classname)s>(
            %(name)s, "%(name)s");\n'''

code159 = '''\
        std::vector<boost::shared_ptr<ObjectHandler::Object> > %(name)sObj =
            ObjectHandler::getObjectVector<ObjectHandler::Object>(%(name)s);\n'''

code160 = '''\
        std::vector<boost::shared_ptr<%(namespaceObjects)s::%(classname)s> > %(name)sObj =
            ObjectHandler::getObjectVector<%(namespaceObjects)s::%(classname)s>(%(name)s);\n'''

code161 = '''\
        OH_GET_REFERENCE_DEFAULT(%(name)sLibObj, %(name)s,
            %(namespaceObjects)s::%(classname)s, %(namespaceLibrary)s::%(classname)s)\n'''

code162 = '''\
        std::vector<%(type)s> %(name)sEnum =
            ObjectHandler::vectorStringToEnum<%(type)s>(%(name)s, "%(name)s");\n'''

code163 = '''\
        OH_GET_OBJECT_DEFAULT(%(name)sCoerce, %(name)s, ObjectHandler::Object)
        %(namespaceLibrary)s::Handle<%(namespaceLibrary)s::%(classname)s> %(name)sLibObj =
            %(namespaceObjects)s::CoerceHandle<
                %(namespaceObjects)s::%(classname)s,
                %(namespaceLibrary)s::%(classname)s>()(
                    %(name)sCoerce, %(namespaceLibrary)s::Handle<%(namespaceLibrary)s::%(classname)s>());\n'''

##########################################################################
# code for C
##########################################################################

code101 = '''\
        %(type)s %(name)sLib;
        cToLib(%(name)sLib, %(name)s);\n'''

code102 = '''\
        OH_GET_REFERENCE(%(name)sLibObj, %(name)s,
            %(namespaceObjects)s::%(classname)s, %(namespaceLibrary)s::%(classname)s)\n'''

code103 = '''\
        %(type)s %(name)sEnum =
            ObjectHandler::Create<%(type)s>()(%(name)s);\n'''

##########################################################################
# code common to mutiple addins
##########################################################################

wrap1 = '''
        // convert input datatypes to C++ datatypes

%s'''

wrap2 = '''
        // convert input datatypes to QuantLib datatypes

%s'''

wrap3 = '''
        // convert input datatypes to QuantLib enumerated datatypes

%s'''

wrap4 = '''
        // convert input datatypes to Object references

%s'''

wrap5 = '''
        // convert object IDs into library objects

%s'''

wrap6 = '''
        // perform data conversion for return value of function

%s'''