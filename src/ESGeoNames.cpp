//
//  ECGeoNames.m
//  Emerald Chronometer
//
//  Created by Steve Pucci 9/2009.
//  Copyright Emerald Sequoia LLC 2009. All rights reserved.
//

//#include "Constants.h"
//#include "ECGlobals.h"
#undef ESTRACE
#include "ESTrace.hpp"
//#include "ChronometerAppDelegate.h"
#include "ESErrorReporter.hpp"
#include "ESGeoNames.hpp"
#include "ESLocation.hpp"
#include "ESThread.hpp"
//#include "ECWatchTime.h"
#include "ESTime.hpp"
#include "ESFileArray.hpp"
#include "ESFile.hpp"
#include "ESLock.hpp"

#include <sys/stat.h>  // For fstat
#include <fcntl.h>  // For open
#include <unistd.h>  // for lseek, read, close
#include <stdlib.h>  // For malloc, free
#include <algorithm>

static ESLock *modifyLock;
static ESGeoNamesData *sharedData;
static int sharedDataRefCount = 0;

static void
getAndRetainSharedDataObject() {
    if (!modifyLock) {
        ESAssert(ESThread::inMainThread());
        modifyLock = new ESLock;
    }
    modifyLock->lock();
    if (!sharedData) {
        ESAssert(sharedDataRefCount == 0);
        sharedData = new ESGeoNamesData;
        sharedDataRefCount = 1;
    }
    modifyLock->unlock();
}

static void
releaseSharedDataObject() {
    ESAssert(modifyLock);  // should have been created with getSharedDataObject() and never deleted
    modifyLock->lock();
    if (--sharedDataRefCount == 0) {
        sharedData->clearStorage();
        // sharedData object is never deleted
    }
    modifyLock->unlock();
}

template<class ElementType>
static void checkFreeFileArray(ESFileArray<ElementType> **arr) {
    if (*arr) {
        delete *arr;
	*arr = NULL;
    }
}

static void checkFreeMallocArray(void **arr) {
    if (*arr) {
	free(*arr);
	*arr = NULL;
    }
}

static void checkFreeFileStringArray(ESFileStringArray **arr) {
    if (*arr) {
        delete *arr;
	*arr = NULL;
    }
}

ESGeoNames::ESGeoNames()
:   _selectedCityIndex(-1),
    _sortedSearchIndices(NULL),
    _numMatchingCities(0)
{
    _numMatchingAtLevel[0] = 0;
    _numMatchingAtLevel[1] = 0;
    _numMatchingAtLevel[2] = 0;
    getAndRetainSharedDataObject();
}

ESGeoNames::~ESGeoNames() {
    releaseSharedDataObject();
    checkFreeMallocArray((void**)&_sortedSearchIndices);
}

// Ideally we'd add the country/city index here, but it's only 16 bits and padding would waste 16 bits.
// If we ever need another 16 bits anyway, we could put all 32 here.
struct ESCityData {
    ESUINT32 population;
    float    latitude;
    float    longitude;
};

struct ESRegionDesc {
    short ccIndex;
    short a1Index;
    short a2Index;
};

ESGeoNamesData::ESGeoNamesData()
:   _numCities(-1),
    _cityNames(NULL),
    _nameIndices(NULL),
    _cityData(NULL),
    _ccNames(NULL),
    _ccCodes(NULL),
    _a1Names(NULL),
    _a2Names(NULL),
    _a1Codes(NULL),
    _tzIndices(NULL),
    _tzNames(NULL),
    _tzCache(NULL),
    _cityRegions(NULL),
    _regionDescs(NULL),
    _numRegionDescs(0)
{
}

ESGeoNamesData::~ESGeoNamesData() {
    ESAssert(false);  // never delete, just call clearStorage
    clearStorage();
}

void
ESGeoNamesData::clearStorage() {
    checkFreeFileArray<char>(&_cityNames);
    checkFreeFileArray<short>(&_ccCodes);
    checkFreeFileArray<int>(&_nameIndices);
    checkFreeFileArray<ESCityData>(&_cityData);
    checkFreeFileArray<short>(&_cityRegions);
    checkFreeFileArray<ESRegionDesc>(&_regionDescs);
    checkFreeFileArray<ESTZData>(&_tzCache);
    checkFreeFileStringArray(&_ccNames);
    checkFreeFileStringArray(&_a1Names);
    checkFreeFileStringArray(&_a2Names);
    checkFreeFileStringArray(&_a1Codes);
    checkFreeFileArray<short>(&_tzIndices);
    checkFreeFileStringArray(&_tzNames);
    _numCities = -1;
    _numRegionDescs = -1;
}

void
ESGeoNamesData::qualifyNumCities(int numCitiesRead) {
    if (_numCities < 0) {
	_numCities = numCitiesRead;
    } else {
	if (numCitiesRead != _numCities) {
            ESErrorReporter::logError("ESGeoNames", "City file mismatch: %d != %d", numCitiesRead, _numCities);
	    exit(1);
	}
    }
}

void
ESGeoNamesData::readCityData() {
    traceEnter("ESGeoNamesData::readCityData");
    _cityData = new ESFileArray<ESCityData>("/eslocation/loc-data.dat", ESFilePathTypeRelativeToResourceDir);
    size_t bytesRead = _cityData->bytesRead();
    ESAssert(bytesRead != 0);
    ESErrorReporter::logInfo("GeoNames", "%d bytes read of %s, first byte is 0x%016lx (%ld)", bytesRead, "loc-data.dat", _cityData->array()[0].population,  _cityData->array()[0].population);
    qualifyNumCities((int)(bytesRead / sizeof(ESCityData)));
    traceExit("ESGeoNamesData::readCityData");
}

void
ESGeoNamesData::readCityNames() {
    traceEnter("ESGeoNamesData::readCityNames");
    _cityNames = new ESFileArray<char>("/eslocation/loc-names.dat", ESFilePathTypeRelativeToResourceDir);
    traceExit("ESGeoNamesData::readCityNames");
}

void
ESGeoNamesData::readNameIndices() {
    _nameIndices = new ESFileArray<int>("/eslocation/loc-index.dat", ESFilePathTypeRelativeToResourceDir);
    size_t bytesRead = _nameIndices->bytesRead();
    qualifyNumCities((int)(bytesRead / sizeof(int)));
}

void
ESGeoNamesData::readRegions() {
    _cityRegions = new ESFileArray<short>("/eslocation/loc-region.dat", ESFilePathTypeRelativeToResourceDir);
    size_t bytesRead = _cityRegions->bytesRead();
    qualifyNumCities((int)(bytesRead / sizeof(short)));
}

void
ESGeoNamesData::readRegionDescs() {
    traceEnter("readRegionDescs");
    _regionDescs = new ESFileArray<ESRegionDesc>("/eslocation/loc-regiondesc.dat", ESFilePathTypeRelativeToResourceDir);
    size_t bytesRead = _regionDescs->bytesRead();
    _numRegionDescs = (int)(bytesRead / sizeof(ESRegionDesc));
    traceExit("readRegionDescs");
}

void
ESGeoNamesData::setupTimezoneRangeTable() {
    traceEnter("setupTimezoneRangeTable");
    ESAssert(_tzCache);	    // malloc-ed but empty
    ESAssert(_tzNames);
    ESAssert(_tzIndices);
    const char **ptr = _tzNames->strings();
    const char **end = ptr + _tzNames->numStrings();
    for (int i = 0; ptr < end; ptr++, i++) {
        const char *tzName = *ptr;
	ESAssert(tzName == _tzNames->stringAtIndex(i));
	ESTimeZone *estz = ESCalendar_initTimeZoneFromOlsonID(tzName);
#ifndef NDEBUG
	if (!estz) {
	    printf("Couldn't construct ESTimeZone with name %s\n", tzName);
	}
	ESAssert(estz);
#endif
        ESTZData *tzCacheEntry = &_tzCache->writableArray()[i];

	ESTimeInterval now = ESTime::currentTime();
        int currentOffset = (int)rint(ESCalendar_tzOffsetForTimeInterval(estz, now));
	ESAssert((currentOffset % 60) == 0);
        tzCacheEntry->currentOffset = currentOffset / 60;
        tzCacheEntry->nextTransition = ESCalendar_nextDSTChangeAfterTimeInterval(estz, now);
	if (tzCacheEntry->nextTransition) {
	    ESAssert(tzCacheEntry->nextTransition > now);
	    int postTransitionOffset = (int)rint(ESCalendar_tzOffsetForTimeInterval(estz, tzCacheEntry->nextTransition + 7200));
	    ESAssert((postTransitionOffset % 60) == 0);
	    ESAssert(currentOffset - postTransitionOffset <= 3600);  // no DST transition greater than an hour
	    tzCacheEntry->stdOffset = 
                (currentOffset < postTransitionOffset ? currentOffset : postTransitionOffset) / 60;
	    tzCacheEntry->dstOffset = 
                (currentOffset < postTransitionOffset ? postTransitionOffset : currentOffset) / 60;
	} else {
	    tzCacheEntry->stdOffset
                = tzCacheEntry->dstOffset 
                = tzCacheEntry->currentOffset;
	}
//	printf("%03d\t%+04d\t%+04d\t%s\n",i, _tzCache->array()[i].stdOffset, _tzCache->array()[i].dstOffset, tzName);
	ESCalendar_releaseTimeZone(estz);
    }
    traceExit ("setupTimezoneRangeTable");
}

#ifndef NDEBUG
void
ESGeoNamesData::testTZNames() {
    ESAssert(_tzNames);
    ESAssert(_tzIndices);
    ESErrorReporter::logInfo("ESGeoNamesData::testTZNames", "begin");
    const char **ptr = _tzNames->strings();
    const char **end = ptr + _tzNames->numStrings();
    for (int i = 0; ptr < end; ptr++, i++) {
	ESTimeZone *estz = ESCalendar_initTimeZoneFromOlsonID(*ptr);  // Will assert if time zone name not found
        ESCalendar_releaseTimeZone(estz);
        int selectedCityIndex = this->findBestCityForTZName(*ptr);
        ESAssert(selectedCityIndex >= 0);
        ESErrorReporter::logInfo("ESGeoNamesData::testTZNames", "Time zone '%s' should use city '%s'", 
                                 *ptr, cityNameForSelectedIndex(selectedCityIndex).c_str());
    }
    ESErrorReporter::logInfo("ESGeoNamesData::testTZNames", "end %d", _tzNames->numStrings());
}
#endif

void
ESGeoNamesData::readTZ() {
    traceEnter("readTZ");
    _tzIndices = new ESFileArray<short>("/eslocation/loc-tz.dat", ESFilePathTypeRelativeToResourceDir);
    size_t bytesRead = _tzIndices->bytesRead();
    qualifyNumCities((int)(bytesRead / sizeof(short)));
    ESAssert(!_tzNames);
    ESAssert(_numCities > 0);
    _tzNames = new ESFileStringArray("/eslocation/loc-tzNames.dat", ESFilePathTypeRelativeToResourceDir, _numCities);
    _tzNamesChecksum = ESFile::readSingleUnsignedFromFile("/eslocation/loc-tzNames.sum", ESFilePathTypeRelativeToResourceDir);
    //printf("_tzNames checksum is %u (0x%08x)\n", _tzNamesChecksum, _tzNamesChecksum);
    if (_tzCache) {
	ESAssert(false);
	return;
    }
#ifndef NDEBUG
    ESErrorReporter::logInfo("ESGeoNames", "need to generate tz table");
#endif
    _tzCache = new ESFileArray<ESTZData>("ThisFileShouldNeverExist.dat", ESFilePathTypeRelativeToAppSupportDir, false /* don't try reading */);
    _tzCache->setupForWriteWithNumElements(_tzNames->numStrings());
#ifndef NDEBUG
    size_t cacheSize = sizeof(ESTZData) * _tzNames->numStrings();
    ESAssert(cacheSize = _tzCache->bytesRead());
#endif
    setupTimezoneRangeTable();
    ESErrorReporter::logInfo("ESGeoNames", "done generating");
    // Don't write to path; I don't trust versioning, especially on Android.
    // _tzCache->writeToPath(fn.c_str(), ESFilePathTypeRelativeToAppSupportDir);
    traceExit("readTZ");
}

#ifndef NDEBUG
ESFileStringArray *
ESGeoNamesData::tzNames() {
    ensureTZ();
    ESAssert(_tzNames);
    return _tzNames;
}
#endif

void
ESGeoNamesData::readCCNames() {
    ESAssert(!_ccNames);
    traceEnter("readCCNames");
    _ccNames = new ESFileStringArray("/eslocation/loc-cc.dat", ESFilePathTypeRelativeToResourceDir, 238);
    traceExit("readCCNames");
}

void
ESGeoNamesData::readCCCodes() {
    ESAssert(!_ccCodes);
    traceEnter("readCCCodes");
    _ccCodes = new ESFileArray<short>("/eslocation/loc-ccCodes.dat", ESFilePathTypeRelativeToResourceDir);
    ESAssert(_ccCodes->array());
    traceExit("readCCCodes");
}

void
ESGeoNamesData::readA1Names() {
    ESAssert(!_a1Names);
    _a1Names = new ESFileStringArray("/eslocation/loc-a1.dat", ESFilePathTypeRelativeToResourceDir, 3269);
}

void
ESGeoNamesData::readA2Names() {
    ESAssert(!_a2Names);
    _a2Names = new ESFileStringArray("/eslocation/loc-a2.dat", ESFilePathTypeRelativeToResourceDir, 569);
}

void
ESGeoNamesData::readA1Codes() {
    ESAssert(!_a1Codes);
    _a1Codes = new ESFileStringArray("/eslocation/loc-a1Codes.dat", ESFilePathTypeRelativeToResourceDir, 3269);
}

void 
ESGeoNamesData::ensureCityData() {
    ESAssert(modifyLock);
    modifyLock->lock();
    if (!_cityData) {
        readCityData();
    }
    modifyLock->unlock();
}

void 
ESGeoNamesData::ensureCityNames() {
    ESAssert(modifyLock);
    modifyLock->lock();
    if (!_cityNames) {
        readCityNames();
    }
    modifyLock->unlock();
}

void 
ESGeoNamesData::ensureNameIndices() {
    ESAssert(modifyLock);
    modifyLock->lock();
    if (!_nameIndices) {
        readNameIndices();
    }
    modifyLock->unlock();
}

void 
ESGeoNamesData::ensureRegions() {
    ESAssert(modifyLock);
    modifyLock->lock();
    if (!_cityRegions) {
        readRegions();
    }
    modifyLock->unlock();
}

void 
ESGeoNamesData::ensureRegionDescs() {
    ESAssert(modifyLock);
    modifyLock->lock();
    if (!_regionDescs) {
        readRegionDescs();
    }
    modifyLock->unlock();
}

void 
ESGeoNamesData::ensureCCNames() {
    ESAssert(modifyLock);
    modifyLock->lock();
    if (!_ccNames) {
        readCCNames();
    }
    modifyLock->unlock();
}

void 
ESGeoNamesData::ensureCCCodes() {
    ESAssert(modifyLock);
    modifyLock->lock();
    if (!_ccCodes) {
        readCCCodes();
    }
    modifyLock->unlock();
}

void 
ESGeoNamesData::ensureA1Names() {
    ESAssert(modifyLock);
    modifyLock->lock();
    if (!_a1Names) {
        readA1Names();
    }
    modifyLock->unlock();
}

void 
ESGeoNamesData::ensureA2Names() {
    ESAssert(modifyLock);
    modifyLock->lock();
    if (!_a2Names) {
        readA2Names();
    }
    modifyLock->unlock();
}

void 
ESGeoNamesData::ensureA1Codes() {
    ESAssert(modifyLock);
    modifyLock->lock();
    if (!_a1Codes) {
        readA1Codes();
    }
    modifyLock->unlock();
}

void 
ESGeoNamesData::ensureTZ() {
    ESAssert(modifyLock);
    modifyLock->lock();
    if (!_tzIndices) {
        readTZ();
    }
    modifyLock->unlock();
}

static float distanceBetweenTwoCoordinates(float lat1, float long1,
					   float lat2, float long2) {
    // Note:  This is somewhat expensive, in particular more expensive than just
    // doing a simple sum-of-squares of the lat/long in degress, which would be
    // perfectly acceptable as a sort or min/max determinant.  But it is more
    // accurate at high positive and negative latitudes.  Leaving in for now until
    // we determine we need something faster.
    return ESLocation::kmBetweenLatLong(lat1 * M_PI / 180, long1 * M_PI / 180,
                                        lat2 * M_PI / 180, long2 * M_PI / 180);
}

#ifndef NDEBUG
void
ESGeoNamesData::findWackyZones() {
    ensureCityData();
    ensureTZ();
    ESTimeInterval now = ESTime::currentTime();
    for (int i = 0; i < _numCities; i++) {
	const ESCityData *thisData = _cityData->array() + i;
	float thisLongitude = thisData->longitude;
	ESTimeZone *thisTZ = ESCalendar_initTimeZoneFromOlsonID(_tzNames->stringAtIndex(_tzIndices->array()[i]));
	float thisTZCenter = ESCalendar_tzOffsetForTimeInterval(thisTZ, now)/3600 - ESCalendar_isDSTAtTimeInterval(thisTZ, now)*15;
        ESCalendar_releaseTimeZone(thisTZ);
	float delta = fabsf(thisLongitude - thisTZCenter);
	if (delta > 180) {
	    delta = 360 - delta;
	}
	if (delta > 25) {
	    tracePrintf3("(%8d) %3.0f %s", thisData->population, delta, cityNameForSelectedIndex(i).c_str());
	}
    }
}
#endif

int
ESGeoNamesData::findClosestCityToLatitudeDegrees(float toLatitude,
                                                 float toLongitude) {
    traceEnter("findClosestCityToLatitudeDegrees");
    ensureCityData();
    float closestDist = 1E20;
    int indx = -1;
    for (int i = 0; i < _numCities; i++) {
	const ESCityData *thisData = _cityData->array() + i;
	float thisDist = distanceBetweenTwoCoordinates(thisData->latitude, thisData->longitude,
						       toLatitude, toLongitude);
	if (thisDist < closestDist) {
	    closestDist = thisDist;
	    indx = i;
	}
    }
    traceExit("findClosestCityToLatitudeDegrees");
    return indx;
}

int
ESGeoNamesData::findBestMatchCityToLatitudeDegrees(float toLatitude,
                                                   float toLongitude) {
    ensureCityData();
    float closestDist = 1E20;
    int indx = -1;
    for (int i = 0; i < _numCities; i++) {
	const ESCityData *thisData = _cityData->array() + i;
	float thisDist = distanceBetweenTwoCoordinates(thisData->latitude, thisData->longitude,
						       toLatitude, toLongitude) / powf(thisData->population, .5);
	if (thisDist < closestDist) {
	    closestDist = thisDist;
	    indx = i;
	}
    }
    return indx;
}

std::string
ESGeoNamesData::getDisplayNameAtNameIndex(int nameIndex) {
    const char *compoundName = _cityNames->array() + nameIndex;
    const char *displayNameStart = strrchr(compoundName, '+');
    if (displayNameStart) {
	displayNameStart++;  // Move past '+'
    } else {
	displayNameStart = compoundName;
    }
    return displayNameStart;
}

static std::string
extractCityFromOlsonName(const std::string &olsonName) {
    std::size_t lastSlashIndex = olsonName.rfind('/');
    std::string lastComponent;
    if (lastSlashIndex == std::string::npos) {
        lastComponent = olsonName;
    } else {
        lastComponent = olsonName.substr(lastSlashIndex + 1);
    }
    std::replace(lastComponent.begin(), lastComponent.end(), '_', ' ');
    // ESErrorReporter::logInfo("extractCityFromOlsonName", "%s => %s", olsonName.c_str(), lastComponent.c_str());
    return lastComponent;
}

bool
ESGeoNamesData::cityAtIndexIsOlsonCity(int index) {
    std::string tzCityName = extractCityFromOlsonName(_tzNames->strings()[_tzIndices->array()[index]]);
    std::string cityName = getDisplayNameAtNameIndex(_nameIndices->array()[index]);
    return cityName == tzCityName;
}

int
ESGeoNamesData::findBestCityForTZName(const std::string &tzName) {
    ensureTZ();
#ifndef NDEBUG
    // static bool testDone = false;
    // if (!testDone) {
    //     testDone = true;
    //     testTZNames();
    // }
#endif
    // First get tz index of tzName
    const char **ptr = _tzNames->strings();
    const char **end = ptr + _tzNames->numStrings();
    int tzIndex = -1;
    for (int i = 0; ptr < end; ptr++, i++) {
        if (tzName == *ptr) {
            tzIndex = i;
            break;
        }
    }
    // ESErrorReporter::logInfo("ESGeoNamesData::findBestCityForTZName", 
    //                          "tz '%s' is at index %d", tzName.c_str(), tzIndex);
    int bestCityIndex = -1;
    ensureCityData();
    if (tzIndex >= 0) {
        // Search for the largest city matching that {name => index}
        ESUINT32 bestPopulation = 0;
        const short *tzIndices = _tzIndices->array();
        for (int i = 0; i < _numCities; i++) {
            if (tzIndices[i] == tzIndex) {
                const ESCityData *thisData = _cityData->array() + i;
                if (thisData->population > bestPopulation) {
                    bestPopulation = thisData->population;
                    bestCityIndex = i;
                }
            }
        }
        if (bestCityIndex >= 0) {
            ESErrorReporter::logInfo("ESGeoNamesData::findBestCityForTZName", 
                                     "Found Olson city matching tz '%s'", tzName.c_str());
            return bestCityIndex;
        }
        ESErrorReporter::logInfo("ESGeoNamesData::findBestCityForTZName", 
                                 "Odd: found no cities matching GeoNames tz '%s'", tzName.c_str());
    }

    // Here we can't find tzName in our database, or (surprisingly) there is no city that matches a
    // recognized tzName.  Use the OS (which is presumably where this name came from) to determine
    // the current offset, the next tz change, and the next tz offset, and compare with all cities.
    // The closest *Olson* city (one whose name matches the tz name) is returned.

    double deviceLatitudeDegrees = ESLocation::deviceLocation()->latitudeDegrees();
    double deviceLongitudeDegrees = ESLocation::deviceLocation()->longitudeDegrees();

    ESTimeInterval now = ESTime::currentTime();

    ESTimeZone *estz = ESCalendar_initTimeZoneFromOlsonID(tzName.c_str());
    int currentOffset = (int)rint(ESCalendar_tzOffsetForTimeInterval(estz, now)) / 60;
    ESTimeInterval nextTransition = ESCalendar_nextDSTChangeAfterTimeInterval(estz, now);
    int stdOffset;
    int dstOffset;
    if (nextTransition) {
        int postTransitionOffset = (int)rint(ESCalendar_tzOffsetForTimeInterval(estz, nextTransition + 7200)) / 60;
        if (currentOffset > postTransitionOffset) {
            stdOffset = postTransitionOffset;
            dstOffset = currentOffset;
        } else {
            stdOffset = currentOffset;
            dstOffset = postTransitionOffset;
        }
    } else {
        stdOffset = dstOffset = currentOffset;
    }
    ESCalendar_releaseTimeZone(estz);

    // We need to compare city names with tz names, so read in city names and indices.
    ensureCityNames();
    ensureNameIndices();
    
    double bestDistance = 1E100;
    const short *tzIndices = _tzIndices->array();
    const ESTZData *cacheArray = _tzCache->array();
    for (int i = 0; i < _numCities; i++) {
        const ESTZData &cacheEntry = cacheArray[tzIndices[i]];
        if (currentOffset == cacheEntry.currentOffset &&
            nextTransition == cacheEntry.nextTransition &&
            stdOffset == cacheEntry.stdOffset &&
            dstOffset == cacheEntry.dstOffset) {
            if (cityAtIndexIsOlsonCity(i)) {
                ESAssert(_cityData->array());
                const ESCityData *thisData = _cityData->array() + i;
                double distance = distanceBetweenTwoCoordinates(deviceLatitudeDegrees, thisData->latitude,
                                                                deviceLongitudeDegrees, thisData->longitude);
                if (distance < bestDistance) {
                    bestDistance = distance;
                    bestCityIndex = i;
                }
            }
        }
    }

    if (bestCityIndex >= 0) {
        ESErrorReporter::logInfo("ESGeoNamesData::findBestCityForTZName", 
                                 "Found closest Olson city matching tz '%s' to be '%s'", 
                                 tzName.c_str(), cityNameForSelectedIndex(bestCityIndex).c_str());
        return bestCityIndex;
    }

    // OK, here we haven't found *any* Olson city in geonames that matches the tzName's information.  That's not as
    // unusual as it might sound; tzName might be a fixed UTC offset.

    // Find the closest Olson tz city which matches tzName's UTC offset. This is the same loop as before, except
    // now we care only about currentOffset, and ignore nextTransition and postTransitionOffset.
    
    for (int i = 0; i < _numCities; i++) {
        const ESTZData &cacheEntry = cacheArray[tzIndices[i]];
        if (currentOffset == cacheEntry.currentOffset) {
            if (cityAtIndexIsOlsonCity(i)) {
                const ESCityData *thisData = _cityData->array() + i;
                double distance = distanceBetweenTwoCoordinates(deviceLatitudeDegrees, thisData->latitude,
                                                                deviceLongitudeDegrees, thisData->longitude);
                if (distance < bestDistance) {
                    bestDistance = distance;
                    bestCityIndex = i;
                }
            }
        }
    }
    if (bestCityIndex >= 0) {
        ESErrorReporter::logInfo("ESGeoNamesData::findBestCityForTZName", 
                                 "Found closest Olson city matching currentOffset of tz '%s' (%d) to be '%s'", 
                                 tzName.c_str(), currentOffset, cityNameForSelectedIndex(bestCityIndex).c_str());
        return bestCityIndex;
    }

    // OK, fine.  We can't find an Olson city in the database that matches our UTC offset.  Revert to finding
    // *any* (non-Olson) city in the database that has our offset, is reasonably close to us, and is of reasonable size.
    int closestCityIndex = -1;
    ESUINT32 populationOfClosestCity = 0;
    double closestDistance = 1E100;

    int indexOfLargestCityWithin15km = -1;
    ESUINT32 populationOfLargestCityWithin15km = 0;
    for (int i = 0; i < _numCities; i++) {
        const ESTZData &cacheEntry = cacheArray[tzIndices[i]];
        if (currentOffset == cacheEntry.currentOffset) {
            const ESCityData *thisData = _cityData->array() + i;
            double distance = ESLocation::kmBetweenLatLong(deviceLatitudeDegrees, thisData->latitude,
                                                           deviceLongitudeDegrees, thisData->longitude);
            if (distance < closestDistance) {
                closestDistance = distance;
                closestCityIndex = i;
                populationOfClosestCity = thisData->population;
            }
            if (distance < 15) {
                ESUINT32 population = thisData->population;
                if (population > populationOfLargestCityWithin15km) {
                    populationOfLargestCityWithin15km = population;
                    indexOfLargestCityWithin15km = i;
                }
            }
        }
    }
    if (indexOfLargestCityWithin15km >= 0) {
        bestCityIndex = indexOfLargestCityWithin15km;
    } else {
        ESErrorReporter::logInfo("ESGeoNamesData::findBestCityForTZName", 
                                 "No city within 15 km of device location");
        bestCityIndex = closestCityIndex;
    }
    if (bestCityIndex >= 0) {
        ESErrorReporter::logInfo("ESGeoNamesData::findBestCityForTZName", 
                                 "Found closest reasonable-sized non-Olson city matching currentOffset of tz '%s' (%d) to be '%s'", 
                                 tzName.c_str(), currentOffset, cityNameForSelectedIndex(bestCityIndex).c_str());
    } else {
        ESErrorReporter::logInfo("ESGeoNamesData::findBestCityForTZName", "Failed to find city for tz '%s'", tzName.c_str());
    }

    return bestCityIndex;
}

void
ESGeoNames::findClosestCityToLatitudeDegrees(float toLatitude,
                                             float toLongitude) {
    ESAssert(sharedData);
    _selectedCityIndex = sharedData->findClosestCityToLatitudeDegrees(toLatitude, toLongitude);
}

void
ESGeoNames::findBestMatchCityToLatitudeDegrees(float toLatitude,
                                               float toLongitude) {
    ESAssert(sharedData);
    _selectedCityIndex = sharedData->findBestMatchCityToLatitudeDegrees(toLatitude, toLongitude);
}

bool
ESGeoNames::findBestCityForTZName(const std::string tzName) {
    ESAssert(sharedData);
    _selectedCityIndex = sharedData->findBestCityForTZName(tzName);
    return _selectedCityIndex >= 0;
}

std::string
ESGeoNames::selectedCityName() {     // returns last found city
    ESAssert(sharedData);
    return sharedData->cityNameForSelectedIndex(_selectedCityIndex);
}

std::string
ESGeoNamesData::cityNameForSelectedIndex(int indx) {
    traceEnter("cityNameForSelectedIndex");
    if (indx < 0) {
	return "";
    }
    ensureCityNames();
    int nameIndex;
    modifyLock->lock();
    if (_nameIndices) {
	nameIndex = _nameIndices->array()[indx];
    } else {
        ESFileArray<int>::readElementFromFileAtIndex("/eslocation/loc-index.dat", ESFilePathTypeRelativeToResourceDir, indx, &nameIndex);
    }
    modifyLock->unlock();
    std::string displayName = getDisplayNameAtNameIndex(nameIndex);
    traceExit("cityNameForSelectedIndex");
    return displayName;
}

std::string
ESGeoNamesData::cityRegionNameForSelectedIndex(int indx) {     // returns last found city's region info
    if (indx < 0) {
	return "";
    }
    //ESTime::noteTimeAtPhase("selectedCityRegionName start");
    short regionIndex;
    ESFileArray<short>::readElementFromFileAtIndex("/eslocation/loc-region.dat", ESFilePathTypeRelativeToResourceDir, indx, &regionIndex);
    //ESTime::noteTimeAtPhase("selectedCityRegionName finished reading region index");
    ESRegionDesc regionDesc;
    ESFileArray<ESRegionDesc>::readElementFromFileAtIndex("/eslocation/loc-regiondesc.dat", ESFilePathTypeRelativeToResourceDir, regionIndex, &regionDesc);
    //ESTime::noteTimeAtPhase("selectedCityRegionName finished reading region descriptor");
    std::string regionString = "";
    if (regionDesc.a2Index >= 0) {
        ensureA2Names();
        const char *a2String = _a2Names->stringAtIndex(regionDesc.a2Index);
	if (*a2String) {
	    regionString = a2String;
	}
    }
    if (regionDesc.a1Index >= 0) {
        ensureA1Names();
        const char *a1String = _a1Names->stringAtIndex(regionDesc.a1Index);
	if (*a1String) {
	    if (regionString.length() > 0) {
		regionString = ESUtil::stringWithFormat("%s, %s", regionString.c_str(), a1String);
	    } else {
		regionString = a1String;
	    }
	}
    }
    if (regionDesc.ccIndex >= 0) {
        ensureCCNames();
        const char *ccString = _ccNames->stringAtIndex(regionDesc.ccIndex);
	if (*ccString) {
	    if (regionString.length() > 0) {
		regionString = ESUtil::stringWithFormat("%s, %s", regionString.c_str(), ccString);
	    } else {
		regionString = ccString;
	    }
	}
    }
    //ESTime::noteTimeAtPhase("selectedCityRegionName end");
    return regionString;
}

std::string
ESGeoNames::selectedCityRegionName() {     // returns last found city's region info
    ESAssert(sharedData);
    return sharedData->cityRegionNameForSelectedIndex(_selectedCityIndex);
}

std::string 
ESGeoNamesData::cityTZNameForSelectedIndex(int indx) {
    if (indx < 0) {
	return "";
    }
    ensureTZ();
    ESAssert(_tzIndices);
    return _tzNames->stringAtIndex(_tzIndices->array()[indx]);
}

float 
ESGeoNamesData::cityLatitudeForSelectedIndex(int indx) {
    ESAssert(_cityData);
    ESAssert(indx >= 0);
    const ESCityData *thisData = _cityData->array() + indx;
    return thisData->latitude;
}

float
ESGeoNames::selectedCityLatitude() {
    ESAssert(sharedData);
    return sharedData->cityLatitudeForSelectedIndex(_selectedCityIndex);
}

float 
ESGeoNamesData::cityLongitudeForSelectedIndex(int indx) {
    ESAssert(_cityData);
    ESAssert(indx >= 0);
    const ESCityData *thisData = _cityData->array() + indx;
    return thisData->longitude;
}

float
ESGeoNames::selectedCityLongitude() {
    ESAssert(sharedData);
    return sharedData->cityLongitudeForSelectedIndex(_selectedCityIndex);
}

unsigned long 
ESGeoNamesData::cityPopulationForSelectedIndex(int indx) {
    ESAssert(_cityData);
    ESAssert(indx >= 0);
    const ESCityData *thisData = _cityData->array() + indx;
    return thisData->population;
}

unsigned long
ESGeoNames::selectedCityPopulation() {
    ESAssert(sharedData);
    return sharedData->cityPopulationForSelectedIndex(_selectedCityIndex);
}

bool 
ESGeoNamesData::cityValidForSlotAtOffsetHourForSelectedIndex(int indx,
                                                             int offsetHours) {   // returns true iff city can fit in slot for (offsetHours:offsetHours+1)
    return validCity(indx, offsetHours/*forSlot*/);
}

bool
ESGeoNames::selectedCityValidForSlotAtOffsetHour(int offsetHours) {
    ESAssert(sharedData);
    return sharedData->cityValidForSlotAtOffsetHourForSelectedIndex(_selectedCityIndex, offsetHours);
}

std::string 
ESGeoNamesData::cityCountryCodeForSelectedIndex(int indx) {
    traceEnter("selectedCityCountryCode");
    if (indx >= 0) {
        ensureCCCodes();
        ensureRegions();
        ESAssert(indx < _numCities);
        short regionIndex = _cityRegions->array()[indx];
        ESRegionDesc regionDesc;
        ESFileArray<ESRegionDesc>::readElementFromFileAtIndex("/eslocation/loc-regiondesc.dat", ESFilePathTypeRelativeToResourceDir, regionIndex, &regionDesc);
        short container = _ccCodes->array()[regionDesc.ccIndex];
        char str[3];
        bcopy(&container, str, 2);
        str[2] = '\0';
        traceExit("selectedCityCountryCode");
        return str;
    } else {
        traceExit("selectedCityCountryCode");
        return "";
    }
}

std::string 
ESGeoNames::selectedCityCountryCode() {
    ESAssert(sharedData);
    return sharedData->cityCountryCodeForSelectedIndex(_selectedCityIndex);
}

std::string
ESGeoNames::selectedCityTZName() {  // returns last found city tz
    return sharedData->cityTZNameForSelectedIndex(_selectedCityIndex);
}

struct ESGeoSortDescriptor {
    int	    index;
    float   sortValue;
    int	    sortValue2;
};

int comparator(const void *v1, const void *v2) {
    ESGeoSortDescriptor *desc1 = (ESGeoSortDescriptor *)v1;
    ESGeoSortDescriptor *desc2 = (ESGeoSortDescriptor *)v2;
    if (desc1->sortValue < desc2->sortValue) {
	return -1;
    } else if (desc1->sortValue == desc2->sortValue) {
	return 0;
    } else {
	return 1;
    }
}

// the concept here is to assign a "confidence value" to each city which we will then sort on to pick the best one
// the values for state, country and code (countryCode) come from the users's address book and hence may be unreliable:
//  - some or all may be empty
//  - they may be misspeled
//  - they may use abbreviations (eg "USA" instead of "United States") or the code may appear in the country field
// so the idea is to match find as much matching info as possible and assign a higher confidence level based on:
//  - which things match (eg. state is more definitive than country)
//  - how many of the quantities match
// we'll probably want to catch a few special cases (eg. "GB" for "UK", "USA" for "United States")
// it must all be case-insensitive compares
int
ESGeoNamesData::regionMatchConfidenceForIndex(int         cityIndex,
                                              const char *state,
                                              const char *country,
                                              const char *code) {
    traceEnter("regionMatchConfidence");
    ESAssert(cityIndex >= 0);
    ESAssert(cityIndex < _numCities);
    
    ensureA1Codes();
    ensureRegions();
    ensureRegionDescs();

    short regionIndex = _cityRegions->array()[cityIndex];
    const ESRegionDesc *regionDesc = _regionDescs->array() + regionIndex;

#ifndef NDEBUG
    static bool firstTime = false;
    if (firstTime) {
	int count = _a1Codes->numStrings();
	int count2 = _a1Names->numStrings();
	ESAssert(count == count2);
	for (int i = 0; i < count; i++) {
	    printf("%15s: %s\n", _a1Codes->stringAtIndex(i), _a1Names->stringAtIndex(i));
	}
	firstTime = false;
    }
#endif

    int confidenceLevel = 0;
    const char *a1String = "";
    std::string a1Code = "";
    const char *cString = "";
    std::string cCode = "";
    if (regionDesc->a1Index >= 0) {
        ensureA1Names();
	a1String = _a1Names->stringAtIndex(regionDesc->a1Index);
	a1Code = std::string(_a1Codes->stringAtIndex(regionDesc->a1Index)).substr(3,2);
	ESAssert(a1String && *a1String);
	ESAssert(a1Code.length());
    }
    if (regionDesc->ccIndex >= 0) {
        ensureCCNames();
	cString = _ccNames->stringAtIndex(regionDesc->ccIndex);
	cCode = std::string(_a1Codes->stringAtIndex(regionDesc->a1Index)).substr(0,2);
    }

    bool statesMatch = false;
    bool countriesMatch = false;
    if (( *a1String             && *state && strcasecmp(state, a1String)       == 0) ||
        (  a1Code.length() > 0  && *state && strcasecmp(state, a1Code.c_str()) == 0)) {
	statesMatch = true;
	confidenceLevel++;
    }
    if ((cCode.length() > 0    && *code    && ( strcasecmp(code, cCode.c_str()) == 0 ||
                                                            (strcasecmp(code, "GB") == 0    && strcasecmp(cCode.c_str(), "UK") == 0))) ||
	(cCode.length() > 0    && *country && ( strcasecmp(country, cCode.c_str()) == 0 ||
                                                            (strcasecmp(country, "GB") == 0 && strcasecmp(cCode.c_str(), "UK") == 0))) ||
        (*cString              && *country && ( strcasecmp(country, cString) == 0 ||
                                                (strcasecmp(country, "USA") == 0 && strcasecmp(cString, "United States")  == 0) ||
                                                (strcasecmp(country, "GB")  == 0 && strcasecmp(cString, "United Kingdom") == 0)))) {
	countriesMatch = true;
	confidenceLevel++;
    }
    
    if ((!statesMatch &&    *state > 0) ||
	(!countriesMatch && (*code || *country))) {
	confidenceLevel = 0;
    }
    // subtract some if the city name matches only partially?

#ifdef ECTRACE
    std::string tmp =  ESUtil::stringWithFormat("%s %s %s %s", a1String, a1Code.c_str(), cString, cCode.c_str());
    std::string tmp2 = ESUtil::stringWithFormat("%s %s %s", state, country, code);
    tracePrintf3("'%s' matches '%s' with confidence %d", tmp2.c_str(), tmp.c_str(), confidenceLevel);
    traceExit ("regionMatchConfidence");
#endif
    return confidenceLevel;
}

int comparator2(const void *v1, const void *v2) {
    ESGeoSortDescriptor *desc1 = (ESGeoSortDescriptor *)v1;
    ESGeoSortDescriptor *desc2 = (ESGeoSortDescriptor *)v2;
    if (desc1->sortValue2 > desc2->sortValue2) {
	return -1;
    } else if (desc1->sortValue2 == desc2->sortValue2) {
	if (desc1->sortValue < desc2->sortValue) {
	    return -1;
	} else if (desc1->sortValue == desc2->sortValue) {
	    return 0;
	} else {
	    return 1;
	}
    } else {
	return 1;
    }

}

static bool
searchForString(const char *searchIn,
		const char *searchFor) {
    while (1) {
	const char *searchResult = strcasestr(searchIn, searchFor);
	if (!searchResult) {
	    return false;
	}
	if (searchResult == searchIn ||   // If it's not the start of the string, then it's past the start of the string and we can look back one char
	    searchResult[-1] == ' ' ||
	    searchResult[-1] == '+') {
	    return true;
	}
	searchIn = searchResult + 1;
    }
}

const char *
ESGeoNamesData::cityNamesArray() {
    return _cityNames->array();
}

const int *
ESGeoNamesData::nameIndicesArray() {
    return _nameIndices->array();
}

const ESCityData *
ESGeoNamesData::cityDataArray() {
    return _cityData->array();
}

void
ESGeoNames::searchForCityNameFragment(const char *cityNameFragment,
                                      bool       proximity) {
    ESAssert(sharedData);
    sharedData->ensureCityData();
    sharedData->ensureCityNames();
    sharedData->ensureNameIndices();

    int numCities = sharedData->numCities();

    ESLocation *deviceLocation = ESLocation::deviceLocation();
    float nameSearchCenterLat = deviceLocation->latitudeDegrees();
    float nameSearchCenterLong = deviceLocation->longitudeDegrees();

    //ESTime::noteTimeAtPhaseWithString(ESUtil::stringWithFormat("search for fragment start: '%s'", cityNameFragment));

    if (!_sortedSearchIndices) {
	_sortedSearchIndices = (ESGeoSortDescriptor *)malloc(numCities * sizeof(ESGeoSortDescriptor));
    }
    _numMatchingCities = 0;
    _numMatchingAtLevel[0] = 0;
    _numMatchingAtLevel[1] = 0;
    _numMatchingAtLevel[2] = 0;
    bool getEmAll = *cityNameFragment == '\0';
    const char *cityNamesArray = sharedData->cityNamesArray();
    const int *nameIndicesArray = sharedData->nameIndicesArray();
    const ESCityData *cityDataArray = sharedData->cityDataArray();
    for (int i = 0; i < numCities; i++) {
	_sortedSearchIndices[i].index = i;
	const char *searchMe = cityNamesArray + nameIndicesArray[i];
	if (getEmAll || searchForString(searchMe, cityNameFragment)) {
	    const ESCityData *data = cityDataArray + i;
	    _sortedSearchIndices[_numMatchingCities].index = i;
	    if (proximity) {
		_sortedSearchIndices[_numMatchingCities++].sortValue = distanceBetweenTwoCoordinates(data->latitude, data->longitude, nameSearchCenterLat, nameSearchCenterLong) / powf(data->population, 2.8);
	    } else {
		_sortedSearchIndices[_numMatchingCities++].sortValue = -data->population;
	    }
	}
    }
    //ESTime::noteTimeAtPhase("sort search start");
    qsort(_sortedSearchIndices, _numMatchingCities, sizeof(ESGeoSortDescriptor), comparator);
    //ESTime::noteTimeAtPhase("sort search finish");
}

/* static */ bool
ESGeoNames::validTZCenteredAt(short tzCenter,
                              int   offsetHours) {
    int centerSlotMinutes = offsetHours * 60 + 30;
    int distanceToCenterFromSlot = centerSlotMinutes - tzCenter;
    if (distanceToCenterFromSlot > 12 * 60) {
	distanceToCenterFromSlot -= 24 * 60;
    } else if (distanceToCenterFromSlot < -12 * 60) {
	distanceToCenterFromSlot += 24 * 60;
    }
    return (abs(distanceToCenterFromSlot) <= 30);
}

/* static */ short
ESGeoNames::tzCenterForTZ(ESTimeZone *tz) {
    ESTimeInterval now = ESTime::currentTime();
    int currentOffset = (int)rint(ESCalendar_tzOffsetForTimeInterval(tz, now));
    ESAssert((currentOffset % 60) == 0);
    ESTimeInterval nextTransition = ESCalendar_nextDSTChangeAfterTimeInterval(tz, now);
    if (nextTransition) {
	int postTransitionOffset = (int)rint(ESCalendar_tzOffsetForTimeInterval(tz, nextTransition + 7200));
	ESAssert((postTransitionOffset % 60) == 0);
	ESAssert(abs(currentOffset - postTransitionOffset) <= 3600);  // no DST transition greater than an hour
	return ((currentOffset + postTransitionOffset) / 60) / 2;
    } else {
	return currentOffset / 60;
    }
}

/* static */ bool
ESGeoNames::validTZ(ESTimeZone *tz,
                    int        offsetHours) {
    return validTZCenteredAt(tzCenterForTZ(tz), offsetHours/*forSlot*/);
}

bool
ESGeoNamesData::validCity(int cityIndex,
                          int offsetHours) {
    ensureTZ();
    ESAssert(_tzIndices);
    ESAssert(_tzCache);
    short tzCenter = (_tzCache->array()[_tzIndices->array()[cityIndex]].stdOffset + _tzCache->array()[_tzIndices->array()[cityIndex]].dstOffset) / 2;
    return ESGeoNames::validTZCenteredAt(tzCenter, offsetHours/*forSlot*/);
}

ESSlotInclusionClass 
ESGeoNamesData::inclusionClassForSlotAtOffsetHourForSelectedIndex(int indx,
                                                                  int offsetHours) {	    // returns a code indicating why city is or isn't in this slot
    if (cityValidForSlotAtOffsetHourForSelectedIndex(indx, offsetHours)) {
	short std = _tzCache->array()[_tzIndices->array()[indx]].stdOffset;
	short dst = _tzCache->array()[_tzIndices->array()[indx]].dstOffset;
	if (std<0) {
	    std += 24*60;
	    dst += 24*60;
	}
	if (offsetHours<0) {
	    offsetHours += 24;
	}
	if (std == 24*60) {
	    std = 0;
	}
	if (dst == 24*60) {
	    dst = 0;
	}
	if (offsetHours == 24) {
	    offsetHours = 0;
	}
	if (std == dst) {
	    // no DST in this zone
	    if ((std % 60) == 0) {
		return (offsetHours * 60) == std ? normalNoDSTRight : normalNoDSTLeft;
	    } else if ((std % 30) == 0) {
		return halfNoDST;
	    }
	    return oddNoDST;
	} else {
	    ESAssert(dst == (std+60) % (24*60));
	    if ((std % 60) == 0) {
		return normalHasDST;
	    } else if ((std % 30) == 0) {
		return (offsetHours * 60) > std ? halfHasDSTRight : halfHasDSTLeft;
	    }
	    return oddHasDST;
	}
    } else {
	return notIncluded;
    }
}

ESSlotInclusionClass
ESGeoNames::selectedCityInclusionClassForSlotAtOffsetHour(int offsetHours) {
    ESAssert(sharedData);
    return sharedData->inclusionClassForSlotAtOffsetHourForSelectedIndex(_selectedCityIndex, offsetHours);
}	 

void
ESGeoNames::searchForCityNameFragmentForNominalTZSlot(const char *cityNameFragment,
                                                      int        offsetHours) {
    traceEnter("searchForCityNameFragmentForNominalTZSlot");
    sharedData->ensureTZ();
    ESAssert(sharedData);
    sharedData->ensureCityData();  // for population, for sorting
    sharedData->ensureCityNames();
    sharedData->ensureNameIndices();

    int numCities = sharedData->numCities();

    //ESTime::noteTimeAtPhaseWithString(ESUtil::stringWithFormat("search for fragment start: '%s'", cityNameFragment.c_str()));

    if (!_sortedSearchIndices) {
	_sortedSearchIndices = (ESGeoSortDescriptor *)malloc(numCities * sizeof(ESGeoSortDescriptor));
    }
    _numMatchingCities = 0;
    _numMatchingAtLevel[0] = 0;
    _numMatchingAtLevel[1] = 0;
    _numMatchingAtLevel[2] = 0;
    bool getEmAll = *cityNameFragment == '\0';
    const char *cityNamesArray = sharedData->cityNamesArray();
    const int *nameIndicesArray = sharedData->nameIndicesArray();
    const ESCityData *cityDataArray = sharedData->cityDataArray();
    for (int i = 0; i < numCities; i++) {
	_sortedSearchIndices[i].index = i;
	const char *searchMe = cityNamesArray + nameIndicesArray[i];
	if (getEmAll || searchForString(searchMe, cityNameFragment)) {
	    if (sharedData->validCity(i, offsetHours/*forSlot*/)) {
		const ESCityData *data = cityDataArray + i;
		_sortedSearchIndices[_numMatchingCities].index = i;
		_sortedSearchIndices[_numMatchingCities++].sortValue = -data->population;
	    }
	}
    }
    //ESTime::noteTimeAtPhase("sort search start");
    qsort(_sortedSearchIndices, _numMatchingCities, sizeof(ESGeoSortDescriptor), comparator);
    //ESTime::noteTimeAtPhase("sort search finish");
    traceExit("searchForCityNameFragmentForNominalTZSlot");
}

int
ESGeoNames::searchForCity(const char *cityName,
                          const char *state,
                          const char *country,
                          const char *code) {
    traceEnter("searchForCity");
    ESAssert(sharedData);
    sharedData->ensureCityData();
    sharedData->ensureCityNames();
    sharedData->ensureNameIndices();

    int numCities = sharedData->numCities();

    ESLocation *deviceLocation = ESLocation::deviceLocation();
    float nameSearchCenterLat = deviceLocation->latitudeDegrees();
    float nameSearchCenterLong = deviceLocation->longitudeDegrees();

#ifdef ETRACE
    //NSString *tmp = ESUtil::stringWithFormat("%s, %s %s %s".c_str().c_str().c_str(), cityName, state, country, code);
    tracePrintf1("search for: '%s, %s %s %s'", cityName, state, country, code);
#endif

    ESAssert(cityName && *cityName);  // nonempty
    
    if (!_sortedSearchIndices) {
	_sortedSearchIndices = (ESGeoSortDescriptor *)malloc(numCities * sizeof(ESGeoSortDescriptor));
    }

    int confidenceLevel = -1;
    _numMatchingCities = 0;
    _numMatchingAtLevel[0] = 0;
    _numMatchingAtLevel[1] = 0;
    _numMatchingAtLevel[2] = 0;
    const char *cityNamesArray = sharedData->cityNamesArray();
    const int *nameIndicesArray = sharedData->nameIndicesArray();
    const ESCityData *cityDataArray = sharedData->cityDataArray();
    for (int i = 0; i < numCities; i++) {
	_sortedSearchIndices[i].index = i;
	const char *searchMe = cityNamesArray + nameIndicesArray[i];
	if (searchForString(searchMe, cityName)) {
	    const ESCityData *data = cityDataArray + i;
	    _sortedSearchIndices[_numMatchingCities].index = i;
	    _sortedSearchIndices[_numMatchingCities].sortValue  = distanceBetweenTwoCoordinates(data->latitude, data->longitude, nameSearchCenterLat, nameSearchCenterLong) / powf(data->population, 2.8);
	    int conf = sharedData->regionMatchConfidenceForIndex(i, state, country, code);
	    _sortedSearchIndices[_numMatchingCities].sortValue2 = conf;
	    confidenceLevel = fmax(confidenceLevel, conf);
	    _numMatchingCities++;
	    _numMatchingAtLevel[conf]++;
	}
    }
    //tracePrintf1("sort search2 start %d matches", _numMatchingCities);
    qsort(_sortedSearchIndices, _numMatchingCities, sizeof(ESGeoSortDescriptor), comparator2);
    traceExit("searchForCity");
    return confidenceLevel;
}

void
ESGeoNames::clearSelection() {
    _selectedCityIndex = -1;
    _numMatchingCities = 0;
    _numMatchingAtLevel[0] = 0;
    _numMatchingAtLevel[1] = 0;
    _numMatchingAtLevel[2] = 0;
    checkFreeMallocArray((void**)&_sortedSearchIndices);
}

void
ESGeoNames::selectNthTopCity(int indx) {  // after search; then after calling this you can use selected* methods above
    ESAssert(_sortedSearchIndices);
    if (indx >= _numMatchingCities) {
	_selectedCityIndex = -1;
    } else {
	_selectedCityIndex = _sortedSearchIndices[indx].index;
    }
}

void 
ESGeoNames::selectCityWithIndex(int index) {     // raw index, without search
    _selectedCityIndex = index;
}

std::string
ESGeoNames::topCityNameAtIndex(int indx) {  // after search
    if (indx >= _numMatchingCities) {
	return "";
    }
    selectNthTopCity(indx);
    return selectedCityName();
}

int
ESGeoNames::numMatches() {  // after search; number of matching city entries
    return _numMatchingCities;
}

int
ESGeoNames::numMatchesAtLevel(int level) {
    return _numMatchingAtLevel[level];
}

