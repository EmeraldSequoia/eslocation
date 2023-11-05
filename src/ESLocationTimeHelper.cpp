//
//  ESLocationTimeHelper.cpp
//
//  Created by Steven Pucci 28 Jul 2011
//  Copyright Emerald Sequoia LLC 2011. All rights reserved.
//

#include "ESLocationTimeHelper.hpp"

#include "ESThread.hpp"
#include "ESUserPrefs.hpp"
#include "ESGeoNames.hpp"
#include "ESTime.hpp"
#include "ESErrorReporter.hpp"
#include "ESUtil.hpp"
#undef ESTRACE
#include "ESTrace.hpp"

class ESLocationTimeHelperThread : public ESChildThread {
  public:
                            ESLocationTimeHelperThread()
    :   ESChildThread("LocTimeHelper", ESChildThreadExitsOnlyByParentRequest)
    {
    }

    /*virtual*/ void        *main();
};

/*virtual*/ void *
ESLocationTimeHelperThread::main() {
    while (true) {
        fd_set readers;
        FD_ZERO(&readers);
        int highestThreadFD = ESThread::setBitsForSelect(&readers);
        select(highestThreadFD + 1, &readers, NULL/*writers*/, NULL, NULL);
        ESThread::processInterThreadMessages(&readers);
    }
    // Exit only happens through pthread_exit()
    return NULL;
}

ESLocationTimeHelper::ESLocationTimeHelper()
:   ESLocationObserver(1000000/*accuracyDesiredInMeters*/, 3600/*updateInterval*/),
    _thread(new ESLocationTimeHelperThread),
    _firstNotificationDone(false)
{
    traceEnter("ESLocationTimeHelper ctor");
    bool priorLocValid = ESUserPrefs::boolPref("ESLocationTimeHelperValid");
    if (priorLocValid) {
        ESTime::setDeviceCountryCode(ESUserPrefs::stringPref("ESLocationTimeHelperCountryCode").c_str());
        _lastNotifiedLatitudeDegrees = ESUserPrefs::doublePref("ESLocationTimeHelperLatitudeDegrees");
        _lastNotifiedLongitudeDegrees = ESUserPrefs::doublePref("ESLocationTimeHelperLongitudeDegrees");
        _firstNotificationDone = true;
    }
    doHelp();
    ESLocation::deviceLocation()->addObserver(this);
    _thread->start();
    traceExit("ESLocationTimeHelper ctor");
}

ESLocationTimeHelper::~ESLocationTimeHelper()
{
    ESLocation::deviceLocation()->removeObserver(this);
    _thread->requestExit();
}
 
void
ESLocationTimeHelper::notifyTime(ESGeoNames *geoNames) {
    ESAssert(ESThread::inMainThread());
    std::string cc = geoNames->selectedCityCountryCode();
    tracePrintf2("Selected city is %s, country code %s\n", geoNames->selectedCityName().c_str(), cc.c_str());
    delete geoNames;
    geoNames = NULL;
    ESUserPrefs::setPref("ESLocationTimeHelperLatitudeDegrees", _lastNotifiedLatitudeDegrees);
    ESUserPrefs::setPref("ESLocationTimeHelperLongitudeDegrees", _lastNotifiedLongitudeDegrees);
    ESUserPrefs::setPref("ESLocationTimeHelperCountryCode", cc);
    ESUserPrefs::setPref("ESLocationTimeHelperValid", true);
    ESUtil::noteTimeAtPhase(ESUtil::stringWithFormat("ESLocationTimeHelper::notifyTime with country code '%s'", cc.c_str()));
    ESTime::setDeviceCountryCode(cc.c_str());
}

static void notifyTimeGlue(void *obj,
                           void *param) {
    ESAssert(ESThread::inMainThread());
    ESLocationTimeHelper *timeHelper = (ESLocationTimeHelper*)obj;
    ESGeoNames *geoNames = (ESGeoNames *)param;
    timeHelper->notifyTime(geoNames);
}

void
ESLocationTimeHelper::doHelpInThread(ESGeoNames *geoNames) {
    ESLocation *devLoc = ESLocation::deviceLocation();
    ESUtil::noteTimeAtPhase(ESUtil::stringWithFormat("ESLocationTimeHelper::doHelpInThread calling findClosestCity to %.2f, %.2f", devLoc->latitudeDegrees(), devLoc->longitudeDegrees()));
    geoNames->findClosestCityToLatitudeDegrees(devLoc->latitudeDegrees(), devLoc->longitudeDegrees());
    ESUtil::noteTimeAtPhase("ESLocationTimeHelper::doHelpInThread done calling findClosestCity");
    ESThread::callInMainThread(notifyTimeGlue, this, geoNames);
}

static void doHelpGlue(void *obj,
                       void *param) {
    ((ESLocationTimeHelper *)obj)->doHelpInThread((ESGeoNames *)param);
}

#define MIN_KM_THRESH 100  // We can move this much in lat and long without changing countries
#define KM_FOR_DLAT(DLAT) (110 * DLAT)
#define KM_FOR_DLONG_AT_LAT(DLONG, LAT) (110 * DLONG * cos(LAT*M_PI/180))

inline bool
ESLocationTimeHelper::countryCheckNeeded() {
    ESAssert(ESThread::inMainThread());
    ESLocation *devLoc = ESLocation::deviceLocation();
    if (!devLoc->valid()) {
        return false;
    }
    if (devLoc->latitudeDegrees() == 0.0 &&
        devLoc->longitudeDegrees() == 0.0) {   // If you're exactly at 0,0 and don't move an inch, we'll fail to update :-)
        return false;
    }
    if (_firstNotificationDone) {
        double devLat = devLoc->latitudeDegrees();
        double devLong = devLoc->longitudeDegrees();
        //printf("This dev loc %.2f, %.2f     last notify %.2f, %.2f\n",
        //       devLat, devLong, _lastNotifiedLatitudeDegrees, _lastNotifiedLongitudeDegrees);
        if (KM_FOR_DLAT(fabs(devLat - _lastNotifiedLatitudeDegrees)) < MIN_KM_THRESH &&
            KM_FOR_DLONG_AT_LAT(fabs(devLong - _lastNotifiedLongitudeDegrees), _lastNotifiedLatitudeDegrees) < MIN_KM_THRESH) {
            // ESUtil::noteTimeAtPhase("ESLocationTimeHelper::countryCheckNeeded, close enough to last time, no need for check");
            return false;
        }
        ESUtil::noteTimeAtPhase("ESLocationTimeHelper::countryCheckNeeded, NOT close enough to last time, need to check");
    } else {
        printf("First time, dev loc is %.2f, %.2f\n", devLoc->latitudeDegrees(), devLoc->longitudeDegrees());
        ESUtil::noteTimeAtPhase("ESLocationTimeHelper::countryCheckNeeded, first notification always passes");
        _firstNotificationDone = true;
    }
    _lastNotifiedLatitudeDegrees = devLoc->latitudeDegrees();
    _lastNotifiedLongitudeDegrees = devLoc->longitudeDegrees();
    return true;
}

void 
ESLocationTimeHelper::doHelp() {
    ESAssert(ESThread::inMainThread());  // for ESGeoNames::getSharedGeoNames
    if (countryCheckNeeded()) {
        _thread->callInThread(doHelpGlue, this, new ESGeoNames);
    }
}

/*virtual*/ void 
ESLocationTimeHelper::newLocationAvailable(ESLocation *location) {
    doHelp();
}
