//
//  ESLocationInfo_android.cc
//
//  Created by Steve Pucci 15 Jul 2017
//  Copyright Emerald Sequoia LLC 2017. All rights reserved.
//

#include <jni.h>

#include "ESJNI.hpp"
#include "ESJNIDefs.hpp"
#include "ESLocation.hpp"
#include "ESErrorReporter.hpp"

extern "C" {

/*
 * Class:     com_emeraldsequoia_eslocation_LocationInfo
 * Method:    getCurrentLocation
 * Signature: ()Lcom/emeraldsequoia/eslocation/LocationInfo;
 */
JNIEXPORT jobject JNICALL Java_com_emeraldsequoia_eslocation_LocationInfo_getCurrentLocation
(JNIEnv *jniEnv, jclass) {
    ESLocation *location = ESLocation::deviceLocation();
    if (location->valid()) {
        ESJNI_java_util_Date jDate = 
            ESJNI_java_util_Date::CreateJavaObject(jniEnv, UNIX_TIME_FROM_ESTIME(location->timestamp()));
        return ESJNI_com_emeraldsequoia_eslocation_LocationInfo::CreateJavaObject(jniEnv, 
                                                                                  location->latitudeDegrees(),
                                                                                  location->longitudeDegrees(),
                                                                                  location->altitudeMeters(),
                                                                                  location->altitudeAccuracyMeters(),
                                                                                  location->accuracyInMeters(),
                                                                                  jDate).toJObject();
    } else {
        return ESJNI_com_emeraldsequoia_eslocation_LocationInfo::CreateJavaObject(jniEnv, 
                                                                                  location->latitudeDegrees(),
                                                                                  location->longitudeDegrees(),
                                                                                  location->altitudeMeters(),
                                                                                  location->altitudeAccuracyMeters(),
                                                                                  location->accuracyInMeters(),
                                                                                  (jobject)0).toJObject();
    }
}

/*
 * Class:     com_emeraldsequoia_eslocation_LocationInfo
 * Method:    kmBetweenLatLongDegrees
 * Signature: (DDDD)D
 */
JNIEXPORT jdouble JNICALL Java_com_emeraldsequoia_eslocation_LocationInfo_kmBetweenLatLongDegrees
(JNIEnv *jniEnv, jclass, jdouble lat1Degrees, jdouble long1Degrees, jdouble lat2Degrees, jdouble long2Degrees) {
    ESErrorReporter::logError("kmBetween", "%.1f %.1f %.1f %.1f", lat1Degrees, long1Degrees, lat2Degrees, long2Degrees);
    return ESLocation::kmBetweenLatLong(lat1Degrees * M_PI / 180, long1Degrees * M_PI / 180,
                                        lat2Degrees * M_PI / 180, long2Degrees * M_PI / 180);
}

}  // extern "C"
