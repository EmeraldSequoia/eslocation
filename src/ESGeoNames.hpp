//
//  ECGeoNames.h
//  Emerald Chronometer
//
//  Created by Steve Pucci 9/2009.
//  Copyright Emerald Sequoia LLC 2009. All rights reserved.
//

#include "ESCalendar.hpp"  // For opaque ESTimeZone

#include <string>

// Opaque types
struct ESCityData;
struct ESGeoSortDescriptor;
struct ESRegionDesc;
struct ESTimeZoneRange;
template<class ElementType> class ESFileArray;

class ESFileStringArray;

typedef enum _ESSlotInclusionClass { //slots reason								    example
    notIncluded,		    //	    doesnt fit in this slot
    normalHasDST,		    // 1    fills the slot exactly						    Los Angeles
    normalNoDSTLeft,		    // 2    on the boundary between this slot and the one to the east		    Phoenix
    normalNoDSTRight,		    // 2    on the boundary between this slot and the one to the west		    Phoenix
    halfHasDSTLeft,		    // 2    evenly splits the boundary between this slot and the one to the east    Adelaide
    halfHasDSTRight,		    // 2    evenly splits the boundary between this slot and the one to the west    Adelaide
    halfNoDST,			    // 1    in the middle of a slot						    Mumbai
    oddHasDST,			    // 1    off center of a slot						    <none as of 2010>
    oddNoDST			    // 1    off center of slot							    Kathmandu
} ESSlotInclusionClass;

typedef struct _ESTZData {
    short                   stdOffset;		    // "minutesFromGMT" for standard time
    short                   dstOffset;		    // "minutesFromGMT" in daylight time   (== stdOffset if no DST)
    // NOTE: The following two fields are new June 2017, thus invalidating any old cached data.  But the current code never
    // attempts to read the cache, so all should be well.
    // NOTE FURTHER:  If we ever *do* cache this, these fields will *still* not be valid unless we are still before nextTransition.
    short                   currentOffset;
    ESTimeInterval          nextTransition;
} ESTZData;

// An object of this class is shared amongst all active ESGeoNames objects to save load time when multiple modules are started at once
// that each use location
class ESGeoNamesData {
  public:
    static void             init();  // Call to initialize lock before constructing first ESGeoNames object

                            ESGeoNamesData();

    void                    ensureCityData();
    void                    ensureCityNames();
    void                    ensureNameIndices();
    void                    ensureRegions();
    void                    ensureRegionDescs();
    void                    ensureCCNames();
    void                    ensureCCCodes();
    void                    ensureA1Names();
    void                    ensureA2Names();
    void                    ensureA1Codes();
    void                    ensureTZ();

    const char              *cityNamesArray();
    const ESINT32           *nameIndicesArray();
    const ESCityData        *cityDataArray();
    int                     numCities() { return _numCities; }

    int                     findClosestCityToLatitudeDegrees(float latitudeDegrees,
                                                             float longitudeDegrees);
    int                     findBestMatchCityToLatitudeDegrees(float latitudeDegrees,
                                                               float longitudeDegrees);	// factors in population, too
    int                     findBestCityForTZName(const std::string &tzName);

    std::string             cityNameForSelectedIndex(int indx);
    std::string             cityRegionNameForSelectedIndex(int indx);
    std::string             cityTZNameForSelectedIndex(int indx);
    float                   cityLatitudeForSelectedIndex(int indx);
    float                   cityLongitudeForSelectedIndex(int indx);
    unsigned long           cityPopulationForSelectedIndex(int indx);
    bool                    cityValidForSlotAtOffsetHourForSelectedIndex(int indx,
                                                                         int offsetHours);   // returns true iff city can fit in slot for (offsetHours:offsetHours+1)
    ESSlotInclusionClass    inclusionClassForSlotAtOffsetHourForSelectedIndex(int indx,
                                                                              int offsetHours);	    // returns a code indicating why city is or isn't in this slot
    std::string             cityCountryCodeForSelectedIndex(int indx);
    
    bool                    validCity(int cityIndex,
                                      int offsetHours);

    int                     regionMatchConfidenceForIndex(int         cityIndex,
                                                          const char *state,
                                                          const char *country,
                                                          const char *code);

// Use this to clear storage when exiting location picker or destroying the last ESGeoNames
    void                    clearStorage();

  private:
                            ~ESGeoNamesData();  // Singleton, which never goes away (though it is cleared when the last geoNames is destroyed)

    void                    qualifyNumCities(int numCitiesRead);
    void                    readCityData();
    void                    readCityNames();
    void                    readNameIndices();
    void                    readRegions();
    void                    readRegionDescs();
    void                    readCCNames();
    void                    readCCCodes();
    void                    readA1Names();
    void                    readA2Names();
    void                    readA1Codes();
    void                    readTZ();
    void                    setupTimezoneRangeTable();
    bool                    cityAtIndexIsOlsonCity(int index);
    std::string             getDisplayNameAtNameIndex(int nameIndex);
#ifndef NDEBUG
  public:  // FIX FIX FIX REMOVE
    void                    testTZNames();
    void                    findWackyZones();

    ESFileStringArray       *tzNames();
#endif

    ESFileArray<char>       *_cityNames;         // String, 1 per city, delimited by NULL characters.  Each name has 1+ components separated
                                                //   by '+':  First the ascii search name, then any alternate names ("Munich"), then the display name in full UTF8 if it's different.
                                                //   Loaded from loc-names.txt
    ESFileArray<ESINT32>    *_nameIndices;       // Index,  1 per city, packed, indicating position of city within cityNames.  Loaded from loc-index.dat
    ESFileArray<ESCityData> *_cityData;          // Pop/lat/long, 1 per city, packed.  Loaded from loc-data.dat
    ESFileArray<short>      *_cityRegions;       // Region index, 1 per city, packed.  Loaded from loc-region.dat
    ESFileArray<ESRegionDesc> *_regionDescs;       // Region descriptors, one per unique region index, packed.  Loaded from loc-regionDesc.dat
    ESFileStringArray       *_ccNames;           // Country names based on ESRegionDesc cc index.  Loaded from loc-cc.dat
    ESFileArray<short>      *_ccCodes;           // Two-character country *codes* (e.g., US) based on ESRegionDesc cc index.  Loaded from loc-ccCodes.dat
    ESFileStringArray       *_a1Names;           // Admin1 names based on ESRegionDesc a1 index.  Loaded from loc-a1.dat
    ESFileStringArray       *_a2Names;           // Admin2 names based on ESRegionDesc a2 index.  Loaded from loc-a2.dat
    ESFileStringArray       *_a1Codes;           // Admin1 *codes* (e.g., US.CA) based on ESRegionDesc a1 index.  Loaded from loc-a1Codes.dat
    ESFileArray<short>      *_tzIndices;         // Time zone index, 1 per city.  Loaded from loc-tz.dat
    ESFileStringArray       *_tzNames;           // Name of time zone, delimited by NULL, for each unique time zone index.  Loaded from loc-tzNames.dat
    unsigned int            _tzNamesChecksum;    // Checksum of tzNames array in use (can be used as version id)
    ESFileArray<ESTZData>   *_tzCache;           // Center of offset of time zone in minutes, for each unique time zone index.  Calculated by instantiating time zones.
    int                     _numCities;          // Count of nameIndices, cityData, regionIndices, etc. arrays
    int                     _numRegionDescs;     // Count of regionDescs array
};

class ESGeoNames {
  public:
                            ESGeoNames();
                            ~ESGeoNames();

// Call findClosest first, then you can use the access methods to return the last found city
    void                    findClosestCityToLatitudeDegrees(float latitudeDegrees,
                                                             float longitudeDegrees);
    void                    findBestMatchCityToLatitudeDegrees(float latitudeDegrees,
                                                               float longitudeDegrees);	// factors in population, too
    bool                    findBestCityForTZName(const std::string tzName);
    std::string             selectedCityName();		// returns last found city
    std::string             selectedCityRegionName();	// returns last found city's region info
    std::string             selectedCityTZName();	// returns last found city tz name
    float                   selectedCityLatitude();		// returns last found city's latitude
    float                   selectedCityLongitude();		// returns last found city's longitude
    unsigned long           selectedCityPopulation();	// returns last found city's population
    bool                    selectedCityValidForSlotAtOffsetHour(int offsetHours);   // returns true iff last found city can fit in slot for (offsetHours:offsetHours+1)
    ESSlotInclusionClass    selectedCityInclusionClassForSlotAtOffsetHour(int offsetHours);	    // returns a code indicating why city is or isn't in this slot
    std::string             selectedCityCountryCode();
    
// Sort top N cities first, then retrieve each one's name
    void                    searchForCityNameFragmentForNominalTZSlot(const char *cityNameFragment,
                                                                      int        offsetHours);
    void                    searchForCityNameFragment(const char *cityNameFragment,
                                                      bool       proximity);
    int                     searchForCity(const char *cityName,
                                          const char *state,
                                          const char *country,
                                          const char *code);
    void                    selectCityWithIndex(int index);     // raw index, without search
    std::string             topCityNameAtIndex(int index);	// after search
    void                    selectNthTopCity(int index);		// after search; then after calling this you can use *selected* methods above
    int                     numMatches();				// after search; number of matching city entries
    int                     numMatchesAtLevel(int level);		// after qualified search; number of matching city entries with confidence level

// revert to pre-search state
    void                    clearSelection();

    static bool             validTZ(ESTimeZone *tz,
                                    int        offsetHours);
    static bool             validTZCenteredAt(short tzCenter,
                                              int   offsetHours);
    static short            tzCenterForTZ(ESTimeZone *tz);
  private:
    
    int                     _selectedCityIndex;  // Index of city currently selected either by findClosestCityToLatitudeDegrees or selectNthTopCity

    ESGeoSortDescriptor     *_sortedSearchIndices;   // Sort descriptor (index + sort value) for each name matched by searchForCityNameFragment
    int                     _numMatchingCities;      // Number of matching cities in sortedSearchIndices
    int			    _numMatchingAtLevel[3];	// Number of matching cities in sortedSearchIndices at each confidence level
};


