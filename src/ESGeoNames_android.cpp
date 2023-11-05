//
//  ESGeoNames_android.cpp
//
//  Created by Steve Pucci 11 Jun 2017
//  Copyright Emerald Sequoia LLC 2017. All rights reserved.
//

#include "ESPlatform.h"
#include "ESErrorReporter.hpp"
#include "ESGeoNames.hpp"
#include "ESJNI.hpp"
#include "ESJNIDefs.hpp"

#include <map>

#include "com_emeraldsequoia_eslocation_GeoNames.h"

std::map<int, ESGeoNames*> geoNamesBySerialNumber;

static int lastUsedSerialNumber = 0;

static ESGeoNames *findGeoNames(int serialNumber) {
    std::map<int, ESGeoNames *>::iterator it = geoNamesBySerialNumber.find(serialNumber);
    if (it == geoNamesBySerialNumber.end()) {
        return NULL;
    }
    return it->second;
}

static ESGeoNames *findAndRemoveGeoNames(int serialNumber) {
    std::map<int, ESGeoNames *>::iterator it = geoNamesBySerialNumber.find(serialNumber);
    if (it == geoNamesBySerialNumber.end()) {
        return NULL;
    }
    ESGeoNames *geoNames = it->second;
    geoNamesBySerialNumber.erase(it);
    return geoNames;
}

static void addGeoNames(int serialNumber, ESGeoNames *theGeoName) {
    ESAssert(!findGeoNames(serialNumber));
    geoNamesBySerialNumber.insert(std::map<int, ESGeoNames *>::value_type(serialNumber, theGeoName));;
}

extern "C" {

/*
 * Class:     com_emeraldsequoia_eslocation_GeoNames
 * Method:    searchForCityNameFragment
 * Signature: (ILjava/lang/String;Z)V
 */
JNIEXPORT void JNICALL Java_com_emeraldsequoia_eslocation_GeoNames_searchForCityNameFragment
(JNIEnv *jniEnv, jobject javaObject, jint jSerialNumber, jstring jstr, jboolean jbool) {
    ESGeoNames *geoNames = findGeoNames((int)jSerialNumber);
    if (geoNames) {
        geoNames->searchForCityNameFragment(ESJNI_java_lang_String(jstr).toESString(jniEnv).c_str(),
                                            ESJBool(jbool).toBool());
    } else {
        ESErrorReporter::logError("ESGeoNames_searchForCityNameFragment", "Didn't find geoNames with serialNumber %d", (int)jSerialNumber);
    }
}

/*
 * Class:     com_emeraldsequoia_eslocation_GeoNames
 * Method:    findClosestCityToLatitudeDegrees
 * Signature: (IDD)V
 */
JNIEXPORT void JNICALL Java_com_emeraldsequoia_eslocation_GeoNames_findClosestCityToLatitudeDegrees
(JNIEnv *jniEnv, jobject javaObject, jint jSerialNumber, jdouble latitudeDegrees, jdouble longitudeDegrees) {
    ESGeoNames *geoNames = findGeoNames((int)jSerialNumber);
    if (geoNames) {
        geoNames->findClosestCityToLatitudeDegrees((float)latitudeDegrees, (float)longitudeDegrees);
    } else {
        ESErrorReporter::logError("ESGeoNames_findClosestCityToLatitudeDegrees", "Didn't find geoNames with serialNumber %d", (int)jSerialNumber);
    }
}

/*
 * Class:     com_emeraldsequoia_eslocation_GeoNames
 * Method:    findBestMatchCityToLatitudeDegrees
 * Signature: (IDD)V
 */
JNIEXPORT void JNICALL Java_com_emeraldsequoia_eslocation_GeoNames_findBestMatchCityToLatitudeDegrees
(JNIEnv *jniEnv, jobject javaObject, jint jSerialNumber, jdouble latitudeDegrees, jdouble longitudeDegrees) {
    ESGeoNames *geoNames = findGeoNames((int)jSerialNumber);
    if (geoNames) {
        geoNames->findBestMatchCityToLatitudeDegrees((float)latitudeDegrees, (float)longitudeDegrees);
    } else {
        ESErrorReporter::logError("ESGeoNames_findBestMatchCityToLatitudeDegrees", "Didn't find geoNames with serialNumber %d", (int)jSerialNumber);
    }
}

/*
 * Class:     com_emeraldsequoia_eslocation_GeoNames
 * Method:    numMatches
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_emeraldsequoia_eslocation_GeoNames_numMatches
(JNIEnv *jniEnv, jobject javaObject, jint jSerialNumber) {
    ESGeoNames *geoNames = findGeoNames((int)jSerialNumber);
    if (geoNames) {
        return geoNames->numMatches();
    } else {
        ESErrorReporter::logError("ESGeoNames_numMatches", "Didn't find geoNames with serialNumber %d", (int)jSerialNumber);
        return 0;
    }
}

/*
 * Class:     com_emeraldsequoia_eslocation_GeoNames
 * Method:    numMatchesAtLevel
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_emeraldsequoia_eslocation_GeoNames_numMatchesAtLevel
(JNIEnv *jniEnv, jobject javaObject, jint jSerialNumber, jint level) {
    ESGeoNames *geoNames = findGeoNames((int)jSerialNumber);
    if (geoNames) {
        return geoNames->numMatchesAtLevel((int)level);
    } else {
        ESErrorReporter::logError("ESGeoNames_numMatchesAtLevel", "Didn't find geoNames with serialNumber %d", (int)jSerialNumber);
        return 0;
    }
}

/*
 * Class:     com_emeraldsequoia_eslocation_GeoNames
 * Method:    topCityNameAtIndex
 * Signature: (II)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_emeraldsequoia_eslocation_GeoNames_topCityNameAtIndex
(JNIEnv *jniEnv, jobject javaObject, jint jSerialNumber, jint index) {
    ESGeoNames *geoNames = findGeoNames((int)jSerialNumber);
    if (geoNames) {
        return ESJNI_java_lang_String(geoNames->topCityNameAtIndex((int)index)).toJString(jniEnv);
    } else {
        ESErrorReporter::logError("ESGeoNames_topCityNameAtIndex", "Didn't find geoNames with serialNumber %d", (int)jSerialNumber);
        return ESJNI_java_lang_String("").toJString(jniEnv);
    }
}

/*
 * Class:     com_emeraldsequoia_eslocation_GeoNames
 * Method:    selectNthTopCity
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_com_emeraldsequoia_eslocation_GeoNames_selectNthTopCity
(JNIEnv *jniEnv, jobject javaObject, jint jSerialNumber, jint index) {
    ESGeoNames *geoNames = findGeoNames((int)jSerialNumber);
    if (geoNames) {
        geoNames->selectNthTopCity((int)index);
    } else {
        ESErrorReporter::logError("ESGeoNames_selectNthTopCity", "Didn't find geoNames with serialNumber %d", (int)jSerialNumber);
    }
}

/*
 * Class:     com_emeraldsequoia_eslocation_GeoNames
 * Method:    selectedCityName
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_emeraldsequoia_eslocation_GeoNames_selectedCityName
(JNIEnv *jniEnv, jobject javaObject, jint jSerialNumber) {
    ESGeoNames *geoNames = findGeoNames((int)jSerialNumber);
    if (geoNames) {
        return ESJNI_java_lang_String(geoNames->selectedCityName()).toJString(jniEnv);
    } else {
        ESErrorReporter::logError("ESGeoNames_selectedCityName", "Didn't find geoNames with serialNumber %d", (int)jSerialNumber);
        return ESJNI_java_lang_String("").toJString(jniEnv);
    }
}

/*
 * Class:     com_emeraldsequoia_eslocation_GeoNames
 * Method:    selectedCityRegionName
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_emeraldsequoia_eslocation_GeoNames_selectedCityRegionName
(JNIEnv *jniEnv, jobject javaObject, jint jSerialNumber) {
    ESGeoNames *geoNames = findGeoNames((int)jSerialNumber);
    if (geoNames) {
        return ESJNI_java_lang_String(geoNames->selectedCityRegionName()).toJString(jniEnv);
    } else {
        ESErrorReporter::logError("ESGeoNames_selectedCityRegionName", "Didn't find geoNames with serialNumber %d", (int)jSerialNumber);
        return ESJNI_java_lang_String("").toJString(jniEnv);
    }
}

/*
 * Class:     com_emeraldsequoia_eslocation_GeoNames
 * Method:    selectedCityTZName
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_emeraldsequoia_eslocation_GeoNames_selectedCityTZName
(JNIEnv *jniEnv, jobject javaObject, jint jSerialNumber) {
    ESGeoNames *geoNames = findGeoNames((int)jSerialNumber);
    if (geoNames) {
        return ESJNI_java_lang_String(geoNames->selectedCityTZName()).toJString(jniEnv);
    } else {
        ESErrorReporter::logError("ESGeoNames_selectedCityTZName", "Didn't find geoNames with serialNumber %d", (int)jSerialNumber);
        return ESJNI_java_lang_String("").toJString(jniEnv);
    }
}

/*
 * Class:     com_emeraldsequoia_eslocation_GeoNames
 * Method:    selectedCityLatitudeDegrees
 * Signature: (I)F
 */
JNIEXPORT jfloat JNICALL Java_com_emeraldsequoia_eslocation_GeoNames_selectedCityLatitudeDegrees
    (JNIEnv *jniEnv, jobject javaObject, jint jSerialNumber) {
    ESGeoNames *geoNames = findGeoNames((int)jSerialNumber);
    if (geoNames) {
        return (jfloat)geoNames->selectedCityLatitude();
    } else {
        ESErrorReporter::logError("ESGeoNames_selectedCityLatitudeDegrees", "Didn't find geoNames with serialNumber %d", (int)jSerialNumber);
        return 0;
    }
}

/*
 * Class:     com_emeraldsequoia_eslocation_GeoNames
 * Method:    selectedCityLongitudeDegrees
 * Signature: (i)F
 */
JNIEXPORT jfloat JNICALL Java_com_emeraldsequoia_eslocation_GeoNames_selectedCityLongitudeDegrees
    (JNIEnv *jniEnv, jobject javaObject, jint jSerialNumber) {
    ESGeoNames *geoNames = findGeoNames((int)jSerialNumber);
    if (geoNames) {
        return (jfloat)geoNames->selectedCityLongitude();
    } else {
        ESErrorReporter::logError("ESGeoNames_selectedCityLongitudeDegrees", "Didn't find geoNames with serialNumber %d", (int)jSerialNumber);
        return 0;
    }
}

/*
 * Class:     com_emeraldsequoia_eslocation_GeoNames
 * Method:    selectedCityPopulation
 * Signature: (I)J
 */
JNIEXPORT jlong JNICALL Java_com_emeraldsequoia_eslocation_GeoNames_selectedCityPopulation
    (JNIEnv *jniEnv, jobject javaObject, jint jSerialNumber) {
    ESGeoNames *geoNames = findGeoNames((int)jSerialNumber);
    if (geoNames) {
        return (jlong)geoNames->selectedCityPopulation();
    } else {
        ESErrorReporter::logError("ESGeoNames_selectedCityPopulation", "Didn't find geoNames with serialNumber %d", (int)jSerialNumber);
        return 0;
    }
}

/*
 * Class:     com_emeraldsequoia_eslocation_GeoNames
 * Method:    selectedCityCountryCode
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_emeraldsequoia_eslocation_GeoNames_selectedCityCountryCode
    (JNIEnv *jniEnv, jobject javaObject, jint jSerialNumber) {
    ESGeoNames *geoNames = findGeoNames((int)jSerialNumber);
    if (geoNames) {
        return ESJNI_java_lang_String(geoNames->selectedCityCountryCode()).toJString(jniEnv);
    } else {
        ESErrorReporter::logError("ESGeoNames_selectedCityCountryCode", "Didn't find geoNames with serialNumber %d", (int)jSerialNumber);
        return ESJNI_java_lang_String("").toJString(jniEnv);
    }
}

/*
 * Class:     com_emeraldsequoia_eslocation_GeoNames
 * Method:    selectedCityValidForSlotAtOffsetHour
 * Signature: (II)Z
 */
JNIEXPORT jboolean JNICALL Java_com_emeraldsequoia_eslocation_GeoNames_selectedCityValidForSlotAtOffsetHour
    (JNIEnv *jniEnv, jobject javaObject, jint jSerialNumber, jint offsetHour) {
    ESGeoNames *geoNames = findGeoNames((int)jSerialNumber);
    if (geoNames) {
        return ESJBool(geoNames->selectedCityValidForSlotAtOffsetHour((int)offsetHour)).toJBool();
    } else {
        ESErrorReporter::logError("ESGeoNames_selectedCityValidForSlotAtOffsetHour", "Didn't find geoNames with serialNumber %d", (int)jSerialNumber);
        return 0;
    }
}

/*
 * Class:     com_emeraldsequoia_eslocation_GeoNames
 * Method:    selectedCityInclusionClassForSlotAtOffsetHour
 * Signature: (II)Lcom/emeraldsequoia/eslocation/GeoNames/SlotInclusionClass;
 */
JNIEXPORT jobject JNICALL Java_com_emeraldsequoia_eslocation_GeoNames_selectedCityInclusionClassForSlotAtOffsetHour
    (JNIEnv *jniEnv, jobject javaObject, jint jSerialNumber, jint offsetHour) {
    ESGeoNames *geoNames = findGeoNames((int)jSerialNumber);
    if (geoNames) {
        ESSlotInclusionClass slotInclusionClass = geoNames->selectedCityInclusionClassForSlotAtOffsetHour(offsetHour);
        switch (slotInclusionClass) {
          case notIncluded:
            return ESJNI_com_emeraldsequoia_eslocation_GeoNames_SlotInclusionClass::notIncludedField(jniEnv).toJObject();
          case normalHasDST:
            return ESJNI_com_emeraldsequoia_eslocation_GeoNames_SlotInclusionClass::normalHasDSTField(jniEnv).toJObject();
          case normalNoDSTLeft:
            return ESJNI_com_emeraldsequoia_eslocation_GeoNames_SlotInclusionClass::normalNoDSTLeftField(jniEnv).toJObject();
          case normalNoDSTRight:
            return ESJNI_com_emeraldsequoia_eslocation_GeoNames_SlotInclusionClass::normalNoDSTRightField(jniEnv).toJObject();
          case halfHasDSTLeft:
            return ESJNI_com_emeraldsequoia_eslocation_GeoNames_SlotInclusionClass::halfHasDSTLeftField(jniEnv).toJObject();
          case halfHasDSTRight:
            return ESJNI_com_emeraldsequoia_eslocation_GeoNames_SlotInclusionClass::halfHasDSTRightField(jniEnv).toJObject();
          case halfNoDST:
            return ESJNI_com_emeraldsequoia_eslocation_GeoNames_SlotInclusionClass::halfNoDSTField(jniEnv).toJObject();
          case oddHasDST:
            return ESJNI_com_emeraldsequoia_eslocation_GeoNames_SlotInclusionClass::oddHasDSTField(jniEnv).toJObject();
          case oddNoDST:
            return ESJNI_com_emeraldsequoia_eslocation_GeoNames_SlotInclusionClass::oddNoDSTField(jniEnv).toJObject();
          default:
            ESErrorReporter::logError("ESGeoNames_selectedCityInclusionClassForSlotAtOffsetHour", "returned unexpected %d", (int)slotInclusionClass);
            return ESJNI_com_emeraldsequoia_eslocation_GeoNames_SlotInclusionClass::notIncludedField(jniEnv).toJObject();
        }
    } else {
        ESErrorReporter::logError("ESGeoNames_selectedCityInclusionClassForSlotAtOffsetHour", "Didn't find geoNames with serialNumber %d", (int)jSerialNumber);
        return ESJNI_com_emeraldsequoia_eslocation_GeoNames_SlotInclusionClass::notIncludedField(jniEnv).toJObject();
    }
}

/*
 * Class:     com_emeraldsequoia_eslocation_GeoNames
 * Method:    findBestCityForTZName
 * Signature: (ILjava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_com_emeraldsequoia_eslocation_GeoNames_findBestCityForTZName
(JNIEnv *jniEnv, jobject javaObject, jint jSerialNumber, jstring jTZName) {
    ESGeoNames *geoNames = findGeoNames((int)jSerialNumber);
    if (geoNames) {
        ESJNI_java_lang_String tzName(jTZName);
        if (geoNames->findBestCityForTZName(tzName.toESString(jniEnv))) {
            return JNI_TRUE;
        } else {
            return JNI_FALSE;
        }
    } else {
        ESErrorReporter::logError("ESGeoNames_findBestCityForTZName", "Didn't find geoNames with serialNumber %d", (int)jSerialNumber);
        return -1;
    }
}

/*
 * Class:     com_emeraldsequoia_eslocation_GeoNames
 * Method:    onCreate
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_emeraldsequoia_eslocation_GeoNames_onCreate
(JNIEnv *jniEnv, jobject javaObject) {
    ESGeoNames *geoNames = new ESGeoNames();
    int serialNumber = ++lastUsedSerialNumber;
    addGeoNames(serialNumber, geoNames);
    return serialNumber;
}

/*
 * Class:     com_emeraldsequoia_eslocation_GeoNames
 * Method:    onDestroy
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_emeraldsequoia_eslocation_GeoNames_onDestroy
(JNIEnv *jniEnv, jobject javaObject, jint jSerialNumber) {
    ESGeoNames *geoNames = findAndRemoveGeoNames((int)jSerialNumber);
    if (geoNames) {
        delete geoNames;
    } else {
        ESErrorReporter::logError("ESGeoNames_onDestroy", "Didn't find geoNames with serialNumber %d", (int)jSerialNumber);
    }
}

}  // extern "C"
