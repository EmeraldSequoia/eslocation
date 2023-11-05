//
//  ESDeviceLocationManager_iOS.mm
//
//  Created by Steven Pucci 30 May 2011
//  Copyright Emerald Sequoia LLC 2011. All rights reserved.
//

#include "ESPlatform.h"
#include "ESDeviceLocationManager.hpp"
#include "ESLocation.hpp"
#define ESTRACE
#include "ESTrace.hpp"

#include "ESErrorReporter.hpp"
#include "ESThread.hpp"

#include <CoreLocation/CoreLocation.h>

static CLLocationManager *clLocationManager;
static bool wasAuthorizedWhenEnteringBackground;

class ESDeviceLocationSleepWakeObserver : public ESUtilSleepWakeObserver {
  public:
    virtual void            goingToSleep() {
    }
    virtual void            wakingUp() {
    }
    virtual void            enteringBackground() {
        ESDeviceLocationManager::enteringBackground();
    }
    virtual void            leavingBackground() {
        ESDeviceLocationManager::leavingBackground();
    }
};

@interface ESCLLocationManagerDelegate : NSObject <CLLocationManagerDelegate> {
}
@end

@implementation ESCLLocationManagerDelegate

- (void)locationManager:(CLLocationManager *)manager
     didUpdateLocations:(NSArray *)locations {
    ESDeviceLocationManager::newLocationAvailable([locations lastObject]);
}

- (void)locationManager:(CLLocationManager *)manager didFailWithError:(NSError *)error {
    if ([error code] == kCLErrorDenied) {
        tracePrintf("User said NO, dammit\n");
        ESDeviceLocationManager::userSaidNo();
    }
    // Otherwise do nothing
}

@end

/*! Do initialization of infrastructure and retrieve last location, if any, but don't start updating yet */
/*static*/ void 
ESDeviceLocationManager::init() {
    ESAssert(ESThread::inMainThread());
    ESAssert(!clLocationManager); // Only call init once
    clLocationManager = [[CLLocationManager alloc] init];
    if ([clLocationManager respondsToSelector:@selector(requestWhenInUseAuthorization)]) {
        [clLocationManager requestWhenInUseAuthorization];  // NOTE: Will not update in background!
    }
    clLocationManager.delegate = [[ESCLLocationManagerDelegate alloc] init];
    CLLocation *lastLocation = clLocationManager.location;
    if (lastLocation) {
        newLocationAvailable(lastLocation);
    }
    ESUtil::registerSleepWakeObserver(new ESDeviceLocationSleepWakeObserver);
}


/*static*/ void 
ESDeviceLocationManager::newLocationAvailable(CLLocation *lastLocation) {
    traceEnter("ESDeviceLocationManager::newLocationAvailable");
    CLLocationCoordinate2D coord = lastLocation.coordinate;
    _lastLatitudeDegrees = coord.latitude;
    _lastLongitudeDegrees = coord.longitude;
    _lastAltitudeMeters = lastLocation.altitude;
    _lastAltitudeAccuracyMeters = lastLocation.verticalAccuracy;
    _lastLocationAccuracyMeters = lastLocation.horizontalAccuracy;
    _lastLocationTimestamp = [lastLocation.timestamp timeIntervalSinceReferenceDate];
    _lastLocationValid = true;
    ESLocation::newDeviceLocationAvailable();
    traceExit("ESDeviceLocationManager::newLocationAvailable");
}

/*static*/ void 
ESDeviceLocationManager::startUpdatingToAccuracyInMeters(double meters, bool enhanceForNavigation) {  // Called by ESLocation when it needs it
    ESAssert(clLocationManager);  // Call init() first
    tracePrintf1("startUpdatingToAccuracyInMeters %.1f", meters);
    CLLocationAccuracy accuracy;
    if (enhanceForNavigation) {
#if ES_IOS
        NSString *osVersion = [[UIDevice currentDevice] systemVersion];
        NSArray *osVersionComponents = [osVersion componentsSeparatedByString:@"."];
        NSString *primaryOSVersionS = [osVersionComponents objectAtIndex:0];
        int primaryOSVersion = [primaryOSVersionS intValue];
        printf("enhanceForNavigation checking ios version %d >= 4\n", primaryOSVersion);
        bool navigationAccuracySupported = primaryOSVersion >= 4;
#elif ES_MACOS
        NSDictionary *versionPlist = [NSDictionary dictionaryWithContentsOfFile:@"/System/Library/CoreServices/SystemVersion.plist"];
        NSString *osVersion = [versionPlist objectForKey:@"ProductVersion"];
        NSLog (@"productVersion =========== %@", osVersion);
        NSArray *osVersionComponents = [osVersion componentsSeparatedByString:@"."];
        NSString *primaryOSVersionS = [osVersionComponents objectAtIndex:0];
        int primaryOSVersion = [primaryOSVersionS intValue];
        NSString *secondaryOSVersionS = [osVersionComponents objectAtIndex:0];
        int secondaryOSVersion = [secondaryOSVersionS intValue];
        bool navigationAccuracySupported = primaryOSVersion >= 10 && secondaryOSVersion >= 7;
#else
#error Unrecognized OS
#endif
        if (navigationAccuracySupported) {
            accuracy = kCLLocationAccuracyBestForNavigation;
        } else {
            accuracy = kCLLocationAccuracyBest;
        }
    } else if (meters >= 3000) {
        accuracy = kCLLocationAccuracyThreeKilometers;
    } else if (meters >= 1000) {
        accuracy = kCLLocationAccuracyKilometer;
    } else if (meters >= 100) {
        accuracy = kCLLocationAccuracyHundredMeters;
    } else if (meters >= 10) {
        accuracy = kCLLocationAccuracyNearestTenMeters;
    } else {
        accuracy = kCLLocationAccuracyBest;
    }
    clLocationManager.desiredAccuracy = accuracy;
    [clLocationManager startUpdatingLocation];
}

/*static*/ void 
ESDeviceLocationManager::stopUpdating() {
    ESAssert(clLocationManager);  // Call init() first
    [clLocationManager stopUpdatingLocation];
}


/*static*/ ESDeviceLocationAuthorization 
ESDeviceLocationManager::authorizationStatus() {
    ESAssert(clLocationManager);  // Call init() first
    CLAuthorizationStatus clStatus = [clLocationManager authorizationStatus];
    switch(clStatus) {
      default:
      case kCLAuthorizationStatusNotDetermined:
        return ESDeviceLocationAuthorizationUnknown;
      case kCLAuthorizationStatusRestricted:
        return ESDeviceLocationAuthorizationRestricted;
      case kCLAuthorizationStatusDenied:
        return ESDeviceLocationAuthorizationDenied;
      case kCLAuthorizationStatusAuthorizedAlways:
        return ESDeviceLocationAuthorizationAllowed;
    }
}

/*static*/ void 
ESDeviceLocationManager::leavingBackground() {
    ESDeviceLocationAuthorization newStatus = authorizationStatus();
    tracePrintf1("device location manager leaving background, status = %d\n", newStatus);
    if (newStatus == ESDeviceLocationAuthorizationAllowed) {
        if (!wasAuthorizedWhenEnteringBackground) {
            tracePrintf("Newly authorized location services: Checking for device location observers\n");
            ESLocation::recalculateDeviceLocationUpdateParameters();
        }
    }
}

/*static*/ void 
ESDeviceLocationManager::enteringBackground() {
    tracePrintf1("device location manager entering background, status = %d\n", authorizationStatus());
    wasAuthorizedWhenEnteringBackground = (authorizationStatus() == ESDeviceLocationAuthorizationAllowed);
}

