#include "ESDeviceLocationManager.hpp"

#include "ESUtil.hpp"
#include "ESThread.hpp"
#include "ESErrorReporter.hpp"
#include "ESSystemTimeBase.hpp"
#include "ESLocation.hpp"

#include "ESJNIDefs.hpp"
#include "com_emeraldsequoia_eslocation_ESLocationListenerImpl.h"

static bool initialized = false;
static ESJNI_com_emeraldsequoia_eslocation_ESLocationListenerImpl listenerImpl;

/*! Do initialization of infrastructure and retrieve last location, if any, but don't start updating yet */
/*static*/ void 
ESDeviceLocationManager::init() {
    // TODO(spucci): Consider switching to a model where we create our own listener here:
    // JNIEnv *jniEnv = ESUtil::jniEnv();
    // ESAssert(ESThread::inMainThread());
    // ESAssert(listenerImpl.isNull());  // Only call init once

    // listenerImpl = ESJNI_com_emeraldsequoia_eslocation_ESLocationListenerImpl::CreateJavaObject(jniEnv).getRetainedCopy(jniEnv);

    // Go get the GoogleApiClient from... where initAndroid is going to put it
    // Call this *once we're connected??*
    // LocationServices.FusedLocationApi.requestLocationUpdates(mGoogleApiClient, mLocationRequest, this);

}

static bool location1IsBetterOrSame(double         locationAccuracy1,
                                    ESTimeInterval locationTimestamp1,
                                    double         locationAccuracy2,
                                    ESTimeInterval locationTimestamp2) {
    if (locationTimestamp1 - locationTimestamp2 > 1800) {  // If loc1 is newer than loc2 by 30 minutes or more use it
        return true;
    } else if (locationTimestamp2 - locationTimestamp1 > 1800) {  // If loc2 is newer than loc1 by 30 minutes or more use it
        return false;
    } else {  // Otherwise pick the location with the best accuracy.  A tie means use #1, the most recent fix.
        return locationAccuracy1 <= locationAccuracy2;
    }
}

/*static*/ void 
ESDeviceLocationManager::processLocation(jdouble latitudeDegrees, jdouble longitudeDegrees, 
                                         jdouble altitudeMeters, jdouble altitudeAccuracyMeters, 
                                         jfloat accuracyMeters, jlong unixTimeMS) {
    // ESErrorReporter::logInfo("processLocation", "lat %.6f, lon %.6f acc %.1f", latitudeDegrees, longitudeDegrees, (double)accuracyMeters);
    ESTimeInterval locationTimestamp = ESTIME_FROM_UNIX_TIME(unixTimeMS);
    if (!_lastLocationValid
        || location1IsBetterOrSame(accuracyMeters, locationTimestamp, _lastLocationAccuracyMeters, _lastLocationTimestamp)) {
        // Accept it
        _lastLatitudeDegrees = latitudeDegrees;
        _lastLongitudeDegrees = longitudeDegrees;
        _lastAltitudeMeters = altitudeMeters;
        _lastAltitudeAccuracyMeters = altitudeAccuracyMeters;
        _lastLocationAccuracyMeters = accuracyMeters;
        _lastLocationTimestamp = locationTimestamp;
        _lastLocationValid = true;
        ESLocation::newDeviceLocationAvailable();
    }
}



/*static*/ void 
ESDeviceLocationManager::startUpdatingToAccuracyInMeters(double meters, bool enhanceForNavigation) {  // Called by ESLocation when it needs it

}

/*static*/ void 
ESDeviceLocationManager::stopUpdating() {

}

JNIEXPORT void JNICALL Java_com_emeraldsequoia_eslocation_ESLocationListenerImpl_locationChanged
    (JNIEnv *, jobject, jobject) {

}
