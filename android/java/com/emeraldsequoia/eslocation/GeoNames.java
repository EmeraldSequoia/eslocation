//
//  GeoNames.java
//
//  Created by Steve Pucci 11 Jun 2017
//  Copyright Emerald Sequoia LLC 2017. All rights reserved.
//

package com.emeraldsequoia.eslocation;

import android.util.Log;

// Wrapper for the C++ ESGeoNames module.
public class GeoNames {
    public enum SlotInclusionClass {  // Spefically for Terra I
        notIncluded,                //      doesnt fit in this slot
        normalHasDST,               // 1    fills the slot exactly                                                  Los Angeles
        normalNoDSTLeft,            // 2    on the boundary between this slot and the one to the east               Phoenix
        normalNoDSTRight,           // 2    on the boundary between this slot and the one to the west               Phoenix
        halfHasDSTLeft,             // 2    evenly splits the boundary between this slot and the one to the east    Adelaide
        halfHasDSTRight,            // 2    evenly splits the boundary between this slot and the one to the west    Adelaide
        halfNoDST,                  // 1    in the middle of a slot                                                 Mumbai
        oddHasDST,                  // 1    off center of a slot                                                    <none as of 2010>
        oddNoDST                    // 1    off center of slot                                                      Kathmandu
    }

    private static final String TAG = "ESGeoNames";

    private int mSerialNumber;

    public GeoNames() {
        mSerialNumber = onCreate();
    }

    void Destroy() {
        onDestroy(mSerialNumber);
        mSerialNumber = -1;
    }

    // Methods returning an index
    public void findClosestCityToLatitudeDegrees(double latitudeDegrees, double longitudeDegrees) {
        findClosestCityToLatitudeDegrees(mSerialNumber, latitudeDegrees, longitudeDegrees);
    }
    public void findBestMatchCityToLatitudeDegrees(double latitudeDegrees, double longitudeDegrees) {     // factors in population, too
        findBestMatchCityToLatitudeDegrees(mSerialNumber, latitudeDegrees, longitudeDegrees);
    }
    public boolean findBestCityForTZName(String tzName) {
        return findBestCityForTZName(mSerialNumber, tzName);
    }

    // Search methods
    public void searchForCityNameFragment(String cityNameFragment, boolean proximity) {
        searchForCityNameFragment(mSerialNumber, cityNameFragment, proximity);
    }

    // Post-search methods
    public int      numMatches() {                                // after search; number of matching city entries
        return numMatches(mSerialNumber);
    }
    public int      numMatchesAtLevel(int level) {                // after qualified search; number of matching city entries with confidence level
        return numMatchesAtLevel(mSerialNumber, level);
    }
    public String   topCityNameAtIndex(int index) {       // after search
        return topCityNameAtIndex(mSerialNumber, index);
    }

    // Select method (must call a search method first)
    public void     selectNthTopCity(int index) {         // after search; then after calling this you can use *selected* methods above
        selectNthTopCity(mSerialNumber, index);
    }

    // Post-select methods
    public String   selectedCityName() {          // returns last found city
        return selectedCityName(mSerialNumber);
    }
    public String   selectedCityRegionName() {    // returns last found city's region info
        return selectedCityRegionName(mSerialNumber);
    }
    public String   selectedCityTZName() {        // returns last found city tz name
        return selectedCityTZName(mSerialNumber);
    }
    public float    selectedCityLatitudeDegrees() {      // returns last found city's latitude
        return selectedCityLatitudeDegrees(mSerialNumber);
    }
    public float    selectedCityLongitudeDegrees() {     // returns last found city's longitude
        return selectedCityLongitudeDegrees(mSerialNumber);
    }
    public long     selectedCityPopulation() {    // returns last found city's population
        return selectedCityPopulation(mSerialNumber);
    }
    public String   selectedCityCountryCode() {   // guess what this one does
        return selectedCityCountryCode(mSerialNumber);
    }

    public boolean  selectedCityValidForSlotAtOffsetHour(int offsetHours) {   // returns true iff last found city can fit in slot for (offsetHours:offsetHours+1)
        return selectedCityValidForSlotAtOffsetHour(mSerialNumber, offsetHours);
    }
    public SlotInclusionClass selectedCityInclusionClassForSlotAtOffsetHour(int offsetHours) {  // returns a code indicating why city is or isn't in this slot
        return selectedCityInclusionClassForSlotAtOffsetHour(mSerialNumber, offsetHours);
    }

    static {
        System.loadLibrary("chronometer-jni");
    }

    ///////////////////////////
    // Private delegate methods
    ///////////////////////////
    private native int  onCreate();
    private native void onDestroy(int serialNumber);

    private native void searchForCityNameFragment(int serialNumber, String cityNameFragment, boolean proximity);
    private native void findClosestCityToLatitudeDegrees(int serialNumber, double latitudeDegrees, double longitudeDegrees);
    private native void findBestMatchCityToLatitudeDegrees(int serialNumber, double latitudeDegrees, double longitudeDegrees);     // factors in population, too
    private native boolean findBestCityForTZName(int serialNumber, String tzName);

    // Post-search methods
    private native int      numMatches(int serialNumber);                                // after search; number of matching city entries
    private native int      numMatchesAtLevel(int serialNumber, int level);                // after qualified search; number of matching city entries with confidence level
    private native String   topCityNameAtIndex(int serialNumber, int index);       // after search

    // Select method (must call a search method first)
    private native void     selectNthTopCity(int serialNumber, int index);         // after search; then after calling this you can use *selected* methods above

    // Post-select methods
    private native String   selectedCityName(int serialNumber);          // returns last found city
    private native String   selectedCityRegionName(int serialNumber);    // returns last found city's region info
    private native String   selectedCityTZName(int serialNumber);        // returns last found city tz name
    private native float    selectedCityLatitudeDegrees(int serialNumber);      // returns last found city's latitude
    private native float    selectedCityLongitudeDegrees(int serialNumber);     // returns last found city's longitude
    private native long     selectedCityPopulation(int serialNumber);    // returns last found city's population
    private native String   selectedCityCountryCode(int serialNumber);   // guess

    private native boolean  selectedCityValidForSlotAtOffsetHour(int serialNumber, int offsetHours);   // returns true iff last found city can fit in slot for (offsetHours:offsetHours+1)
    private native SlotInclusionClass selectedCityInclusionClassForSlotAtOffsetHour(int serialNumber, int offsetHours);   // returns a code indicating why city}
}
