//
//  LocationInfo.java
//
//  Created by Steve Pucci 25 Jun 2017
//  Copyright Emerald Sequoia LLC 2017. All rights reserved.
//

package com.emeraldsequoia.eslocation;

import java.util.Date;

public class LocationInfo {
    public double latitudeInDegrees;
    public double longitudeInDegrees;
    public double altitudeInMeters;
    public double altitudeAccuracyInMeters;
    public double accuracyInMeters;
    public Date lastUpdate;

    private LocationInfo(double lat, double lon, double alt, double altAcc, double acc, Date upd) {
        latitudeInDegrees = lat;
        longitudeInDegrees = lon;
        altitudeInMeters = alt;
        altitudeAccuracyInMeters = altAcc;
        accuracyInMeters = acc;
        lastUpdate = upd;
    }

    public static native LocationInfo getCurrentLocation();
    public static native double kmBetweenLatLongDegrees(double lat1Degrees, double long1Degrees,
                                                        double lat2Degrees, double long2Degrees);
}
