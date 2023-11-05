//
//  CityInfo.java
//
//  Created by Steve Pucci 11 Jun 2017
//  Copyright Emerald Sequoia LLC 2017. All rights reserved.
//

package com.emeraldsequoia.eslocation;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.google.android.gms.wearable.DataMap;

import com.emeraldsequoia.eslocation.GeoNames;

public class CityInfo implements Parcelable {
    private static final String TAG = "CityInfo";

    public String name;
    public String regionName;
    public String countryCode;
    public String tzName;

    public double latitudeDegrees;
    public double longitudeDegrees;
    
    public long population;

    public int primarySlot;
    public int secondarySlot;

    public CityInfo() {}

    // Fills in the city info from geoNames, which must have an item selected.
    public CityInfo(GeoNames geoNames) {
        name = geoNames.selectedCityName();
        regionName = geoNames.selectedCityRegionName();
        countryCode = geoNames.selectedCityCountryCode();
        tzName = geoNames.selectedCityTZName();
        latitudeDegrees = geoNames.selectedCityLatitudeDegrees();
        longitudeDegrees = geoNames.selectedCityLongitudeDegrees();
        population = geoNames.selectedCityPopulation();

        primarySlot = -1;
        secondarySlot = -1;
        // Work backwards, so that dual-slot cities show up on the right,
        // so their dot is on the left.
        for (int i = 23; i >= 0; i--) {
            if (geoNames.selectedCityValidForSlotAtOffsetHour(i)) {
                if (primarySlot == -1) {
                    primarySlot = i;
                } else if (secondarySlot == -1) {
                    secondarySlot = i;
                } else {
                    Log.d(TAG, "Loading from geoNames found more than 2 slots acceptable ???");
                }
            }
        }
        if (secondarySlot != -1 && secondarySlot != primarySlot - 1) {
            // Probably a wrap?
            if (primarySlot == 23 && secondarySlot == 0) {
                primarySlot = 0;
                secondarySlot = 23;
            } else {
                Log.d(TAG, "Weird: non-contiguous primary/secondary slot, disabling secondary slot");
                secondarySlot = -1;
            }
        }
    }
                                
    public CityInfo(DataMap dataMap) {
        name = dataMap.getString("name");
        regionName = dataMap.getString("regionName");
        countryCode = dataMap.getString("countryCode");
        tzName = dataMap.getString("tzName");
        latitudeDegrees = dataMap.getDouble("latitudeDegrees");
        longitudeDegrees = dataMap.getDouble("longitudeDegrees");
        population = dataMap.getLong("population");
        primarySlot = dataMap.getInt("primarySlot");
        secondarySlot = dataMap.getInt("secondarySlot");
    }

    public void writeToDataMap(DataMap dataMap) {
        dataMap.putString("name", name);
        dataMap.putString("regionName", regionName);
        dataMap.putString("countryCode", countryCode);
        dataMap.putString("tzName", tzName);
        dataMap.putDouble("latitudeDegrees", latitudeDegrees);
        dataMap.putDouble("longitudeDegrees", longitudeDegrees);
        dataMap.putLong("population", population);
        dataMap.putInt("primarySlot", primarySlot);
        dataMap.putInt("secondarySlot", secondarySlot);
    }

    // Parcelable
    @Override
    public int describeContents() {
        return 0;
    }

    // Parcelable
    @Override
    public void writeToParcel(Parcel parcel, int flags) {
        parcel.writeString(name);
        parcel.writeString(regionName);
        parcel.writeString(countryCode);
        parcel.writeString(tzName);
        parcel.writeDouble(latitudeDegrees);
        parcel.writeDouble(longitudeDegrees);
        parcel.writeLong(population);
        parcel.writeInt(primarySlot);
        parcel.writeInt(secondarySlot);
    }

    private static CityInfo readFromParcel(Parcel parcel) {
        CityInfo cityInfo = new CityInfo();
        cityInfo.name = parcel.readString();
        cityInfo.regionName = parcel.readString();
        cityInfo.countryCode = parcel.readString();
        cityInfo.tzName = parcel.readString();
        cityInfo.latitudeDegrees = parcel.readDouble();
        cityInfo.longitudeDegrees = parcel.readDouble();
        cityInfo.population = parcel.readLong();
        cityInfo.primarySlot = parcel.readInt();
        cityInfo.secondarySlot = parcel.readInt();
        return cityInfo;
    }

    // Parcelable
    public static final Parcelable.Creator CREATOR = new Parcelable.Creator() {
            public CityInfo createFromParcel(Parcel parcel) {
                return readFromParcel(parcel);
            }

            public CityInfo[] newArray(int size) {
                return new CityInfo[size];
            }
        };
}
