//
//  ESLocation.hpp
//
//  Created by Steven Pucci 30 May 2011
//  Copyright Emerald Sequoia LLC 2011. All rights reserved.
//

#ifndef _ESLOCATION_HPP_
#define _ESLOCATION_HPP_

#include "ESTime.hpp"

#include <math.h>

#include <string>
#include <list>

// Forward decl
class ESLocation;

/*! This class is used by clients to know when their location has been changed. */
class ESLocationObserver {
  public:
                            ESLocationObserver(double         accuracyDesiredInMeters,
                                               ESTimeInterval updateInterval,
                                               bool           enhanceForNavigation = false);
    virtual                 ~ESLocationObserver();

    // You must redefine this method, which will always be called in the main thread
    virtual void            newLocationAvailable(ESLocation *location) = 0;

  private:
    double                  _accuracyDesiredInMeters;  // Don't make these protected; clients shouldn't be able to change them
    ESTimeInterval          _updateInterval;           //  because we need to take action when they do (perhaps reset device update params)
    bool                    _enhanceForNavigation;

friend class ESLocation;
};

typedef enum ESLocState {
    ESLocGood = 0,
    ESLocWorkingGood = 1,
    ESLocWorkingUncertain = 2,
    ESLocUncertain = 3,
    ESLocCanceled = 4,
    ESLocManual = 5
} ESLocState;

/*! A location is either a user-specified location or a system location. */
class ESLocation {
  public:
                            ESLocation(const char *prefsName);  // Initializes from the info in the UserPrefs data associated with prefsName
                            ESLocation(double latitudeInDegrees, double longitudeInDegrees);
                            ~ESLocation();

    static ESLocation       *appDefault();      // once constructed, never goes away, even through user=>device=>user changes
    static ESLocation       *deviceLocation();  // access to device location without (explicitly) constructing an object
    
    bool                    isNorthernHemisphere() { return _latitudeDegrees >= 0; }  // equator is north
    double                  latitudeDegrees() { return _latitudeDegrees; }
    double                  longitudeDegrees() { return _longitudeDegrees; }
    double                  latitudeRadians() { return _latitudeDegrees * M_PI / 180; }
    double                  longitudeRadians() { return _longitudeDegrees * M_PI / 180; }
    double                  altitudeMeters() { return _altitudeMeters; }
    double                  altitudeAccuracyMeters() { return _altitudeAccuracyMeters; }
    double                  accuracyInMeters() { return _accuracyInMeters; }
    bool                    valid() { return _valid; }
    ESTimeInterval          timestamp() { return _timestamp; }
    bool                    isDeviceLocation() { return _isDeviceLocation; }
    bool                    deviceLocationServicesAvailable();
    bool                    locationInProgress();

    ESLocState              indicatorState();

    void                    setToDevice() { setToDevice(false); }
    void                    setToUserLocation(double latitudeDegrees,
                                              double longitudeDegrees,
                                              double accuracyInMeters);
    void                    setToUserLocationAtLastLocation();

    void                    addObserver(ESLocationObserver *observer);  // Tell me when *this* location changes
    void                    removeObserver(ESLocationObserver *observer);

    static double           kmBetweenLatLong(double latitude1Radians,
                                             double longitude1Radians,
                                             double latitude2Radians,
                                             double longitude2Radians);
                                             
    static const char       * const locationPrefixForDeviceOnly;

  protected:
    void                    setNotDevice(bool skipWritingPrefs = false);
    void                    newLocationAvailable();
    void                    recordStateInPrefs();

    static void             newDeviceLocationAvailable();
    static void             initDeviceLocation();
    static void             initAppDefault();
    static void             recalculateDeviceLocationUpdateParameters();

    bool                    _isDeviceLocation;
    double                  _latitudeDegrees;
    double                  _longitudeDegrees;
    double                  _altitudeMeters;
    double                  _altitudeAccuracyMeters;
    double                  _accuracyInMeters;
    bool                    _valid;
    ESTimeInterval          _timestamp;

  private:
    void                    setToDevice(bool skipZeroOverride);
    static void             stopUpdating(bool userSaidNo);

    std::string             _prefsName;
    std::list<ESLocationObserver *> *_observers;

    static ESLocation       *_appDefault;
    static ESLocation       *_deviceLocation;

friend class ESDeviceLocationManager;
friend class ESDeviceLocationTimerFireObserver;
};

inline /*static*/ ESLocation *
ESLocation::appDefault() {
    if (!_appDefault) {
        initAppDefault();
    }
    return _appDefault;
}

inline /*static*/ ESLocation *
ESLocation::deviceLocation() {
    if (!_deviceLocation) {
        initDeviceLocation();
    }
    return _deviceLocation;
}


#endif  // _ESLOCATION_HPP_
