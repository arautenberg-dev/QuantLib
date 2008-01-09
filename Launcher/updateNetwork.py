# Script to update the XLLs delivered by the Launcher
# from files on the network

import update

SOURCE_TARGET_LIST = (
    ( "X:/Apps/Appsscript/CabotoXL/Rev12939/xll", "Addins/01 Production", "QuantLibXLDynamic-vc80-mt-0_9_0.xll" ),
    ( "X:/Apps/Appsscript/CabotoXL/Rev12939/xll", "Addins/01 Production", "ObjectHandler-xll-vc80-mt-0_9_0.xll" ),
    ( "X:/Apps/Appsscript/CabotoXL/Rev12939/xll", "Addins/01 Production", "saohxll-vc80-mt-0_1_9.xll" ),

    ( "X:/Apps/Appsscript/CabotoXL/Rev12939/xll", "Addins/02 Pre-Production", "QuantLibXLDynamic-vc80-mt-0_9_0.xll" ),
    ( "X:/Apps/Appsscript/CabotoXL/Rev12939/xll", "Addins/02 Pre-Production", "ObjectHandler-xll-vc80-mt-0_9_0.xll" ),
    ( "X:/Apps/Appsscript/CabotoXL/Rev12939/xll", "Addins/02 Pre-Production", "saohxll-vc80-mt-0_1_9.xll" ),

    ( "X:/Apps/Appsscript/CabotoXL/R000900-branch/xll", "Addins/03 Testing", "QuantLibXLDynamic-vc80-mt-0_9_0.xll" ),
    ( "X:/Apps/Appsscript/CabotoXL/R000900-branch/xll", "Addins/03 Testing", "ObjectHandler-xll-vc80-mt-0_9_0.xll" ),
    ( "X:/Apps/Appsscript/CabotoXL/R000900-branch/xll", "Addins/03 Testing", "saohxll-vc80-mt-0_1_9.xll" ),
    ( "X:/Apps/Appsscript/CabotoXL/R000900-branch/xll", "Addins/03 Testing", "PDGLib_candidate.xll" ),
)

update.update(SOURCE_TARGET_LIST)