//
//  ESDeviceLocationManager.hpp
//
//  Created by Steven Pucci 30 May 2011
//  Copyright Emerald Sequoia LLC 2011. All rights reserved.
//

#ifndef _ESDEVICELOCATIONMANAGER_HPP_
#define _ESDEVICELOCATIONMANAGER_HPP_

#include "ESPlatform.h"
#include "ESTime.hpp"

// Opaque types for platform-specific methods
ES_OPAQUE_OBJC(CLLocation);
class ESJNI_android_location_Location;

#if ES_ANDROID
#include <jni.h>
#endif

enum ESDeviceLocationAuthorization {
    ESDeviceLocationAuthorizationUnknown,
    ESDeviceLocationAuthorizationRestricted,
    ESDeviceLocationAuthorizationDenied,
    ESDeviceLocationAuthorizationAllowed
};

/*! This class, platform dependent, is responsible for getting location updates from the device.
 *  It always returns data for the device, regardless of overrides set elsewhere. */
class ESDeviceLocationManager {
  public:
    // Methods to be called by ESLocationManager
    static void             init();  /*! Do initialization of infrastructure and retrieve last location, if any, but don't start updating yet */
    static double           lastLatitudeDegrees() { return _lastLatitudeDegrees; }
    static double           lastLongitudeDegrees() { return _lastLongitudeDegrees; }
    static double           lastAltitudeMeters() { return _lastAltitudeMeters; }
    static double           lastAltitudeAccuracyMeters() { return _lastAltitudeAccuracyMeters; }
    static double           lastLocationAccuracyMeters() { return _lastLocationAccuracyMeters; }
    static bool             lastLocationValid() { return _lastLocationValid; }
    static ESTimeInterval   lastLocationTimestamp() { return _lastLocationTimestamp; }
    static void             startUpdatingToAccuracyInMeters(double meters, bool enhanceForNavigation);  // Called by ESLocation when it needs it
    static void             stopUpdating();

    // Methods to be called by clients
    static ESDeviceLocationAuthorization authorizationStatus();

    // Methods to be called by implementation internals
    static void             userSaidNo();
#if ES_COCOA
    static void             newLocationAvailable(CLLocation *lastLocation);
#elif ES_ANDROID
    static void             processLocation(jdouble latitudeDegrees, jdouble longitudeDegrees, 
                                            jdouble altitudeMeters, jdouble altitudeAccuracyMeters, 
                                            jfloat accuracyMeters, jlong unixTimeMS);

    // The rest of these are from the legacy system (non-Wear only)
    // static void             processLocation(JNIEnv                          *jniEnv,
    //                                         ESJNI_android_location_Location &location,
    //                                         bool                            notifyIfBetter = true);
    // static void             onLocationChanged(JNIEnv  *jniEnv,
    //                                           jobject activity,
    //                                           jobject location);
    // static void             onProviderDisabled(JNIEnv  *jniEnv,
    //                                            jobject activity,
    //                                            jstring provider);
    // static void             onProviderEnabled(JNIEnv  *jniEnv,
    //                                           jobject activity,
    //                                           jstring provider);
    // static void             onStatusChanged(JNIEnv  *jniEnv,
    //                                         jobject activity,
    //                                         jstring provider,
    //                                         jint    status,
    //                                         jobject extrasBundle);
#endif

  private:
    static void             enteringBackground();
    static void             leavingBackground();

    static double           _lastLatitudeDegrees;
    static double           _lastLongitudeDegrees;
    static double           _lastAltitudeMeters;
    static double           _lastAltitudeAccuracyMeters;
    static double           _lastLocationAccuracyMeters;
    static bool             _lastLocationValid;
    static ESTimeInterval   _lastLocationTimestamp;

friend class ESDeviceLocationSleepWakeObserver;
};

#endif  // _ESDEVICELOCATIONMANAGER_HPP_
