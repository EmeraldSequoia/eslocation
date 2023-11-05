//
//  ESLocation.cpp
//
//  Created by Steven Pucci 30 May 2011
//  Copyright Emerald Sequoia LLC 2011. All rights reserved.
//

#include "ESPlatform.h"
#include "ESLocation.hpp"
#include "ESDeviceLocationManager.hpp"
#include "ESErrorReporter.hpp"
#include "ESTimer.hpp"
#include "ESTime.hpp"
#include "ESThread.hpp"
#include "ESUserPrefs.hpp"
#include "ESUserString.hpp"
#include "ESUtil.hpp"
#undef ESTRACE
#include "ESTrace.hpp"

/*static*/ ESLocation * ESLocation::_appDefault;
/*static*/ ESLocation * ESLocation::_deviceLocation;

/*static*/ const char * const ESLocation::locationPrefixForDeviceOnly = "Location-Device";

static std::list<ESLocation *> *deviceLocations;
static double minimumDeviceAccuracyNeeded = 1E99;
static ESTimeInterval minimumDeviceUpdateInterval = 1E99;
static bool enhanceForNavigation = false;

static bool deviceLocationInProgress = false;
static double activeDeviceAccuracy = 1E99;

static ESTimer *deviceLocationTimer = NULL;

ESLocation::ESLocation(const char *prefsName)
:   _observers(NULL),
    _prefsName(prefsName)    
{  // Initializes from the info in the UserPrefs data associated with prefsName
    ESAssert(ESThread::inMainThread());  // Location observation calls always return in main thread; this isn't quite an appropriate check but it might catch something
    bool isUserLoc = ESUserPrefs::boolPref(_prefsName + "-isUser");
    _isDeviceLocation = false;  // unconditionally so setToDevice sees proper flag
    _latitudeDegrees = ESUserPrefs::doublePref(_prefsName + "-latitudeDegrees");
    _longitudeDegrees = ESUserPrefs::doublePref(_prefsName + "-longitudeDegrees");
    _altitudeMeters = ESUserPrefs::doublePref(_prefsName + "-altitudeMeters");
    _altitudeAccuracyMeters = ESUserPrefs::doublePref(_prefsName + "-altitudeAccuracyMeters");
    _accuracyInMeters = ESUserPrefs::doublePref(_prefsName + "-accuracyMeters");
    _valid = ESUserPrefs::boolPref(_prefsName + "-valid");
    _timestamp = ESUserPrefs::doublePref(_prefsName + "-timestamp");
    if (!isUserLoc) {
        setToDevice(true/*skipZeroOverride*/);
    }
    // if (_valid) {
    //     ESErrorReporter::logInfo("ESLocation ctor", "retrieved location from UserPrefs: %.3f, %.3f", _latitudeDegrees, _longitudeDegrees);
    // } else {
    //     ESErrorReporter::logInfo("ESLocation ctor", "valid location not stored in ESUserPrefs");
    // }
}

ESLocation::ESLocation(double latitudeInDegrees, double longitudeInDegrees)
:   _observers(NULL),
    _prefsName("")    
{
    _isDeviceLocation = false;
    _latitudeDegrees = latitudeInDegrees;
    _longitudeDegrees = longitudeInDegrees;
    _altitudeMeters = 0;
    _altitudeAccuracyMeters = -1;
    _accuracyInMeters = 0;
    _valid = true;
    _timestamp = ESTime::currentTime();
}

ESLocation::~ESLocation() {
    ESAssert(ESThread::inMainThread());  // Location observation calls always return in main thread; this isn't quite an appropriate check but it might catch something
    setNotDevice(true/*skipWritingPrefs*/);
    if (_observers) {
        delete _observers;
    }
}

void
ESLocation::recordStateInPrefs() {
    if (_prefsName.empty()) {
        return;
    }
    ESAssert(ESThread::inMainThread());  // Location observation calls always return in main thread; this isn't quite an appropriate check but it might catch something
    ESAssert(_prefsName != locationPrefixForDeviceOnly || _isDeviceLocation);
    ESUserPrefs::setPref(_prefsName + "-isUser", !_isDeviceLocation);
    ESUserPrefs::setPref(_prefsName + "-latitudeDegrees", _latitudeDegrees);
    ESUserPrefs::setPref(_prefsName + "-longitudeDegrees", _longitudeDegrees);
    ESUserPrefs::setPref(_prefsName + "-altitudeMeters", _altitudeMeters);
    ESUserPrefs::setPref(_prefsName + "-altitudeAccuracyMeters", _altitudeAccuracyMeters);
    ESUserPrefs::setPref(_prefsName + "-accuracyMeters", _accuracyInMeters);
    ESUserPrefs::setPref(_prefsName + "-valid", _valid);
    ESUserPrefs::setPref(_prefsName + "-timestamp", _timestamp);
}

ESLocState
ESLocation::indicatorState() {
    // The following commented-out block was from iOS EC and can't be implemented here, since we don't support
    // user-requested location updates (I think).
    // if (deviceLocationInProgress && userRequested) {
    //     return ESSystemTimeBase::currentSystemTime() - _timestamp < minimumDeviceUpdateInterval ? ESLocWorkingGood : ESLocWorkingUncertain;
    // }
    if (!_isDeviceLocation) {
	return ESLocManual;
//    } else if (canceled) {
//	return ESLocCanceled;
    } else if (_valid && ESSystemTimeBase::currentSystemTime() - _timestamp < minimumDeviceUpdateInterval) {
	return deviceLocationInProgress ? ESLocWorkingGood : ESLocGood;
    } else {
	return deviceLocationInProgress ? ESLocWorkingUncertain : ESLocUncertain;
    }
}

bool
ESLocation::locationInProgress() {
    return _isDeviceLocation && deviceLocationInProgress;
}

void 
ESLocation::setToDevice(bool skipZeroOverride) {
    ESAssert(ESThread::inMainThread());  // Location observation calls always return in main thread; this isn't quite an appropriate check but it might catch something
    if (!_isDeviceLocation) {
        _isDeviceLocation = true;  // Do this early so asserts don't trigger on mismatch with name.
        // Add to list of device locations
        if (!deviceLocations) {
            deviceLocations = new std::list<ESLocation *>;
            ESDeviceLocationManager::init();
        }
        if (ESDeviceLocationManager::lastLocationValid()) {
            _latitudeDegrees = ESDeviceLocationManager::lastLatitudeDegrees();
            _longitudeDegrees = ESDeviceLocationManager::lastLongitudeDegrees();
            _altitudeMeters = ESDeviceLocationManager::lastAltitudeMeters();
            _altitudeAccuracyMeters = ESDeviceLocationManager::lastAltitudeAccuracyMeters();
            _valid = true;
            _timestamp = ESDeviceLocationManager::lastLocationTimestamp();
            newLocationAvailable();
        } else if (!skipZeroOverride) {
            _latitudeDegrees = 0;
            _longitudeDegrees = 0;
            _altitudeMeters = 0;
            _altitudeAccuracyMeters = -1;
            _valid = false;
        }
        deviceLocations->push_back(this);
        recalculateDeviceLocationUpdateParameters();
        recordStateInPrefs();
    }
}

void
ESLocation::setNotDevice(bool skipWritingPrefs) {
    ESAssert(ESThread::inMainThread());  // Location observation calls always return in main thread; this isn't quite an appropriate check but it might catch something
    // Remove from list of device locations
    if (_isDeviceLocation) {
        ESAssert(deviceLocations);  // How did we get here without creating the list
        deviceLocations->remove(this);
        _isDeviceLocation = false;
        recalculateDeviceLocationUpdateParameters();
        if (!skipWritingPrefs) {
            recordStateInPrefs();
        }
    }
}

void 
ESLocation::setToUserLocation(double latitudeDegrees,
                              double longitudeDegrees,
                              double accuracyInMeters) {
    ESAssert(ESThread::inMainThread());  // Location observation calls always return in main thread; this isn't quite an appropriate check but it might catch something
    ESAssert(this != _deviceLocation);  // Don't try to override device location -- it's reserved to always point at the device
    ESAssert(_prefsName != locationPrefixForDeviceOnly);
    setNotDevice();
    _latitudeDegrees = latitudeDegrees;
    _longitudeDegrees = longitudeDegrees;
    _altitudeMeters = 0;
    _altitudeAccuracyMeters = -1;
    _accuracyInMeters = accuracyInMeters;
    _valid = true;
    _timestamp = ESTime::currentContinuousTime();
    newLocationAvailable();
    recordStateInPrefs();
}

void 
ESLocation::setToUserLocationAtLastLocation() {
    setToUserLocation(_latitudeDegrees, _longitudeDegrees, _accuracyInMeters);
}

void 
ESLocation::addObserver(ESLocationObserver *observer) {  // Tell me when *this* location changes
    ESAssert(ESThread::inMainThread());  // Location observation calls always return in main thread; this isn't quite an appropriate check but it might catch something
    if (!_observers) {
        _observers = new std::list<ESLocationObserver *>;
    }
    _observers->push_back(observer);
    if (_isDeviceLocation) {
        recalculateDeviceLocationUpdateParameters();
    }
}

void 
ESLocation::removeObserver(ESLocationObserver *observer) {
    ESAssert(ESThread::inMainThread());  // Location observation calls always return in main thread; this isn't quite an appropriate check but it might catch something
    ESAssert(_observers);
    _observers->remove(observer);
}


void 
ESLocation::newLocationAvailable() {
    ESAssert(ESThread::inMainThread());  // Location observation calls always return in main thread; this isn't quite an appropriate check but it might catch something
    if (_observers) {
        std::list<ESLocationObserver *>::iterator end = _observers->end();
        std::list<ESLocationObserver *>::iterator iter = _observers->begin();
        while (iter != end) {
            ESLocationObserver *observer = *iter;
            observer->newLocationAvailable(this);
            iter++;
        }
    }
    recordStateInPrefs();
}

class ESDeviceLocationTimerFireObserver : public ESTimerObserver {
  public:
    virtual void            notify(ESTimer *timer) {
        ESLocation::recalculateDeviceLocationUpdateParameters();
    }
};
static ESDeviceLocationTimerFireObserver *timerFireObserver = NULL;

static void
reinstallLocationTimer() {
    ESAssert(ESThread::inMainThread());
    if (deviceLocationTimer) {
        deviceLocationTimer->release();
        deviceLocationTimer = NULL;
    }
    double lastTimestamp = ESDeviceLocationManager::lastLocationTimestamp();
    ESTimeInterval nextUpdateInterval = minimumDeviceUpdateInterval - (ESSystemTimeBase::currentSystemTime() - lastTimestamp);
    if (nextUpdateInterval < 1E50) {
        if (!timerFireObserver) {
            timerFireObserver = new ESDeviceLocationTimerFireObserver;
        }
        deviceLocationTimer = new ESIntervalTimer(timerFireObserver,
                                                  nextUpdateInterval);
    }
}


/*static*/ void
ESLocation::stopUpdating(bool userSaidNo) {
    ESDeviceLocationManager::stopUpdating();
    deviceLocationInProgress = false;
    if (!userSaidNo) {
        reinstallLocationTimer();
    }
}

/*static*/ void 
ESLocation::newDeviceLocationAvailable() {
    ESAssert(ESThread::inMainThread());  // Location observation calls always return in main thread; this isn't quite an appropriate check but it might catch something
    traceEnter("newDeviceLocationAvailable");
    double lastAccuracyMeters = ESDeviceLocationManager::lastLocationAccuracyMeters();
    double lastLatitudeDegrees = ESDeviceLocationManager::lastLatitudeDegrees();
    double lastLongitudeDegrees = ESDeviceLocationManager::lastLongitudeDegrees();
    double lastAltitudeMeters = ESDeviceLocationManager::lastAltitudeMeters();
    double lastAltitudeAccuracyMeters = ESDeviceLocationManager::lastAltitudeAccuracyMeters();
    double lastTimestamp = ESDeviceLocationManager::lastLocationTimestamp();
    if (deviceLocations) {
        std::list<ESLocation *>::iterator end = deviceLocations->end();
        std::list<ESLocation *>::iterator iter = deviceLocations->begin();
        while (iter != end) {
            ESLocation *location = *iter;
            ESAssert(location->_isDeviceLocation);
            location->_latitudeDegrees = lastLatitudeDegrees;
            location->_longitudeDegrees = lastLongitudeDegrees;
            location->_altitudeMeters = lastAltitudeMeters;
            location->_altitudeAccuracyMeters = lastAltitudeAccuracyMeters;
            location->_timestamp = lastTimestamp;
            location->_accuracyInMeters = lastAccuracyMeters;
            location->_valid = true;
            location->newLocationAvailable();
            iter++;
        }
        if (lastAccuracyMeters <= minimumDeviceAccuracyNeeded &&
            (ESSystemTimeBase::currentSystemTime() - lastTimestamp <= minimumDeviceUpdateInterval)) {
            stopUpdating(false/* !userSaidNo*/);
            tracePrintf("Turning off location updating because requested accuracy reached");
        }
    } else {
        // No point in updating if nobody cares...
        minimumDeviceAccuracyNeeded = 1E99;
        minimumDeviceUpdateInterval = 1E99;
        stopUpdating(false/* !userSaidNo*/);
        tracePrintf("Turning off location updating because no observers");
    }
    traceExit("newDeviceLocationAvailable");
}

/*static*/ void 
ESLocation::initDeviceLocation() {
    ESAssert(ESThread::inMainThread());  // Location observation calls always return in main thread; this isn't quite an appropriate check but it might catch something
    ESAssert(!_deviceLocation);
    if (!_deviceLocation) {
        _deviceLocation = new ESLocation(locationPrefixForDeviceOnly);
    }
}

/*static*/ void 
ESLocation::initAppDefault() {
    ESAssert(ESThread::inMainThread());  // Location observation calls always return in main thread; this isn't quite an appropriate check but it might catch something
    ESAssert(!_appDefault);
    _appDefault = new ESLocation("Location-AppDefault");
}

/*static*/ void 
ESLocation::recalculateDeviceLocationUpdateParameters() {
    ESAssert(ESThread::inMainThread());  // Location observation calls always return in main thread; this isn't quite an appropriate check but it might catch something
    // Walk through all observers of all device locations collecting most restrictive parameters
    minimumDeviceAccuracyNeeded = 1E99;
    minimumDeviceUpdateInterval = 1E99;
    enhanceForNavigation = false;
    if (deviceLocations) {
        std::list<ESLocation *>::iterator locEnd = deviceLocations->end();
        std::list<ESLocation *>::iterator locIter = deviceLocations->begin();
        while (locIter != locEnd) {
            ESLocation *location = *locIter;
            if (location->_observers) {
                std::list<ESLocationObserver *>::iterator end = location->_observers->end();
                std::list<ESLocationObserver *>::iterator iter = location->_observers->begin();
                while (iter != end) {
                    ESLocationObserver *observer = *iter;
                    if (observer->_accuracyDesiredInMeters < minimumDeviceAccuracyNeeded) {
                        minimumDeviceAccuracyNeeded = observer->_accuracyDesiredInMeters;
                    }
                    if (observer->_updateInterval < minimumDeviceUpdateInterval) {
                        minimumDeviceUpdateInterval = observer->_updateInterval;
                    }
                    if (observer->_enhanceForNavigation) {
                        enhanceForNavigation = true;
                    }
                    iter++;
                }
            }
            locIter++;
        }
    }
    bool lastValid = ESDeviceLocationManager::lastLocationValid();
    double lastAccuracy = ESDeviceLocationManager::lastLocationAccuracyMeters();
    ESTimeInterval intervalSinceLastLocationFix = ESSystemTimeBase::currentSystemTime() - ESDeviceLocationManager::lastLocationTimestamp();
    bool moreNeeded = deviceLocations && (!lastValid || minimumDeviceAccuracyNeeded < lastAccuracy || minimumDeviceUpdateInterval < intervalSinceLastLocationFix || enhanceForNavigation);
    if (deviceLocationInProgress) {
        if (moreNeeded) {
            if (minimumDeviceAccuracyNeeded < activeDeviceAccuracy || enhanceForNavigation) {
                ESDeviceLocationManager::stopUpdating();
                ESDeviceLocationManager::startUpdatingToAccuracyInMeters(minimumDeviceAccuracyNeeded, enhanceForNavigation);
                activeDeviceAccuracy = minimumDeviceAccuracyNeeded;
            }
        } else {
            stopUpdating(false/* !userSaidNo*/);
        }
    } else {
        if (moreNeeded) {
            ESDeviceLocationManager::startUpdatingToAccuracyInMeters(minimumDeviceAccuracyNeeded, enhanceForNavigation);
            deviceLocationInProgress = true;
            activeDeviceAccuracy = minimumDeviceAccuracyNeeded;
        } else {
            reinstallLocationTimer();
        }
    }
}

// Haversine
/*static*/ double 
ESLocation::kmBetweenLatLong(double latitude1Radians,
                             double longitude1Radians,
                             double latitude2Radians,
                             double longitude2Radians) {
    double earthRadius = 6371; // km
    double deltaLat = latitude2Radians - latitude1Radians;
    double deltaLong = longitude2Radians - longitude1Radians;

    double sinDLatOver2 = sin(deltaLat/2);
    double sinDLongOver2 = sin(deltaLong/2);
    double a = sinDLatOver2*sinDLatOver2 + sinDLongOver2*sinDLongOver2 * cos(latitude1Radians)*cos(latitude2Radians);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    return earthRadius * c;
}

ESLocationObserver::ESLocationObserver(double         accuracyDesiredInMeters,
                                       ESTimeInterval updateInterval,
                                       bool           enhanceForNavigation)
:   _accuracyDesiredInMeters(accuracyDesiredInMeters),
    _updateInterval(updateInterval),
    _enhanceForNavigation(enhanceForNavigation)
{
    ESAssert(ESThread::inMainThread());  // Location observation calls always return in main thread; this isn't quite an appropriate check but it might catch something
    // The accuracy desired and update interval only apply to device locations
    // But they vary from client to client so they are properly a property of observers not locations
}

/*virtual*/ 
ESLocationObserver::~ESLocationObserver() {
    ESAssert(ESThread::inMainThread());  // Location observation calls always return in main thread; this isn't quite an appropriate check but it might catch something
}
