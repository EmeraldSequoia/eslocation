package com.emeraldsequoia.eslocation;

import android.location.Location;
import android.util.Log;

import com.google.android.gms.location.LocationListener;

public class ESLocationListenerImpl implements LocationListener {
    private static final String TAG = "ESLocationListenerImpl";

    public ESLocationListenerImpl() {
    }

    // LocationListener method
    @Override
    public void onLocationChanged (Location location) {
        Log.d(TAG, "onLocationChanged");
        locationChanged(location);
    }

    private native void locationChanged(Location location);
}
