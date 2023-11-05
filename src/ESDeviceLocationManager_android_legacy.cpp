//
//  ESDeviceLocationManager_android.cpp
//
//  Created by Steve Pucci 06 Jun 2011
//  Copyright Emerald Sequoia LLC 2011. All rights reserved.
//

#include "ESDeviceLocationManager.hpp"

#include "ESUtil.hpp"
#include "ESThread.hpp"
#include "ESErrorReporter.hpp"
#include "ESSystemTimeBase_android.hpp"
#include "ESLocation.hpp"

#include "../android/jni/ESJNIDefs.hpp"

static ESJNI_android_location_LocationManager androidLocationManager;

static bool location1IsStrictlyBetter(double         locationAccuracy1,
                                      ESTimeInterval locationTimestamp1,
                                      double         locationAccuracy2,
                                      ESTimeInterval locationTimestamp2) {
    if (locationTimestamp1 - locationTimestamp2 > 3600) {  // If loc1 is newer than loc2 by an hour or more use it
        return true;
    } else if (locationTimestamp2 - locationTimestamp1 > 3600) {  // If loc2 is newer than loc1 by an hour or more use it
        return false;
    } else {  // Otherwise pick the location with the best accuracy
        return locationAccuracy1 < locationAccuracy2;
    }
}

/*static*/ void
ESDeviceLocationManager::processLocation(JNIEnv                          *jniEnv,
                                         ESJNI_android_location_Location &location,
                                         bool                            notifyIfBetter) {
    // If this is better than what we had before, set _last* fields and, if notifyIfBetter, call ESLocation::newDeviceLocationAvailable()
    if (!location.isNull()) {
        float locationAccuracy = location.getAccuracy(jniEnv);
        ESTimeInterval locationTimestamp = ESTIME_FROM_ANDROID_TIME(location.getTime(jniEnv));
        if (!_lastLocationValid
            || location1IsStrictlyBetter(locationAccuracy, locationTimestamp, _lastLocationAccuracyMeters, _lastLocationTimestamp)) {
            // Accept it
            _lastLatitudeDegrees = location.getLatitude(jniEnv);
            _lastLongitudeDegrees = location.getLongitude(jniEnv);
            _lastLocationAccuracyMeters = locationAccuracy;
            _lastAltitudeMeters = location.getAltitude(jniEnv);
            // This should work but the interface is only available in Android O
            // _lastAltitudeAccuracyMeters = location.getVerticalAccuracy(jniEnv);
            SYNTAX ERROR;  // But we're not using this code anyway.
            _lastAltitudeAccuracyMeters = -1;
            _lastLocationTimestamp = locationTimestamp;
            _lastLocationValid = true;
            if (notifyIfBetter) {
                ESLocation::newDeviceLocationAvailable();
            }
        }
    }
}

/*! Do initialization of infrastructure and retrieve last location, if any, but don't start updating yet */
/*static*/ void 
ESDeviceLocationManager::init() {
    ESAssert(ESThread::inMainThread());
    ESAssert(androidLocationManager.isNull());  // Only call init once
    JNIEnv *jniEnv = ESUtil::jniEnv();
    ESJNI_android_location_Location::initAndRetain(jniEnv);
    ESJNI_android_location_LocationManager::initAndRetain(jniEnv);
    androidLocationManager =
        ESUtil::theAndroidActivity()
        .getSystemService(jniEnv, ESJNI_java_lang_String("location"))  // Context.LOCATION_SERVICE == "location"
        .castAs_android_location_LocationManager()
        .getRetainedCopy(jniEnv);
    ESJNI_android_location_Location gpsLocation =
        androidLocationManager.getLastKnownLocation(jniEnv, ESJNI_java_lang_String("gps"));  // LocationManager.GPS_PROVIDER == "gps"
    processLocation(jniEnv, gpsLocation, false/* !notifyIfBetter*/);
    ESJNI_android_location_Location networkLocation =
        androidLocationManager.getLastKnownLocation(jniEnv, ESJNI_java_lang_String("network"));  // LocationManager.NETWORK_PROVIDER = "network"
    processLocation(jniEnv, networkLocation, false/* !notifyIfBetter*/);
    if (_lastLocationValid) {
        ESLocation::newDeviceLocationAvailable();
    }
}

/*static*/ void 
ESDeviceLocationManager::startUpdatingToAccuracyInMeters(double meters, bool enhanceForNavigation) {  // Called by ESLocation when it needs it
    ESAssert(!androidLocationManager.isNull());  // Call init first
    JNIEnv *jniEnv = ESUtil::jniEnv();
#if 0  // For now, use the simple form of requestLocationUpdates
    if (enhanceForNavigation) {
        meters = 0;
    }
    int accuracy;
    if (enhanceForNavigation) {
        accuracy = 3; // ACCURACY_HIGH;
    } else if (meters >= 500) {
        accuracy = 2; // ACCURACY_MEDIUM;
    } else if (meters >= 100) {
        accuracy = 1; // ACCURACY_LOW;
    } else {
        accuracy = 3; // ACCURACY_HIGH;
    }
    // Set up a criteria, then say
    criteria.setHorizontalAccuracy(jniEnv, accuracy);
    // and use one of the forms of requestLocationUpdates that takes a criteria
#endif
    androidLocationManager.requestLocationUpdates(jniEnv,
                                                  ESJNI_java_lang_String("gps"),
                                                  0/*minTime*/,  // Note: Sending 0 for minTime but that's ok because we'll shut it off ourselves when we have a fix
                                                  0/*minDistanceMeters*/,   // Ditto here: OK to specify 0 because we'll shut it off ourselves
                                                  ESUtil::theAndroidActivity().castAs_android_location_LocationListener());
}

/*static*/ void 
ESDeviceLocationManager::stopUpdating() {
    ESAssert(!androidLocationManager.isNull());  // Call init first
    JNIEnv *jniEnv = ESUtil::jniEnv();
    androidLocationManager.removeUpdates(jniEnv, ESUtil::theAndroidActivity().castAs_android_location_LocationListener());
}

// Methods to be called by clients
/*static*/ ESDeviceLocationAuthorization 
ESDeviceLocationManager::authorizationStatus() {
    return ESDeviceLocationAuthorizationAllowed;
}

// Callbacks from LocationListener implementation, delegated from activity glue layer
/*static*/ void 
ESDeviceLocationManager::onLocationChanged(JNIEnv  *jniEnv,
                                           jobject activity,
                                           jobject location) {
    ESJNI_android_location_Location loc(location);
    processLocation(jniEnv, loc, true/*notifyIfBetter*/);
}

/*static*/ void 
ESDeviceLocationManager::onProviderDisabled(JNIEnv  *jniEnv,
                                            jobject activity,
                                            jstring provider) {

}

/*static*/ void 
ESDeviceLocationManager::onProviderEnabled(JNIEnv  *jniEnv,
                                           jobject activity,
                                           jstring provider) {

}

/*static*/ void 
ESDeviceLocationManager::onStatusChanged(JNIEnv  *jniEnv,
                                         jobject activity,
                                         jstring provider,
                                         jint    status,
                                         jobject extrasBundle) {

}

