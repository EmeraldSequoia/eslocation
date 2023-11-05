//
//  LocationSelectActivity.java
//
//  Created by Steve Pucci 11 Jun 2017
//  Copyright Emerald Sequoia LLC 2017. All rights reserved.
//

package com.emeraldsequoia.eslocation;

import java.util.Arrays;

import android.app.Activity;
import android.app.RemoteInput;
import android.content.Intent;
import android.os.Bundle;
import androidx.annotation.Keep;
// import android.support.v7.widget.RecyclerView;
import android.support.wearable.input.RemoteInputConstants;
import android.support.wearable.input.RemoteInputIntent;
import android.support.wearable.view.WearableRecyclerView;
import android.util.Log;
import android.widget.Toast;

import com.emeraldsequoia.chronometer.wearable.ListAdapter;

import com.emeraldsequoia.chronometer.common.R;

// This class asks the user for a city in the city database, and returns a CityInfo
// for the chosen city as the Activity result.
@Keep
public class LocationSelectActivity extends Activity {
    private static final String TAG = "LocationSelectActivity";

    public static final String CITY_INFO = "CityInfo";

    private static final int REQUEST_SEARCH = 1;  // Searching for a city
    private static final String KEY_MATCHING_CITIES = "matching_cities";
    private static final String KEY_SEARCH_TEXT = "search_text";
    private static final String[] POPULAR_CITIES = new String[] { 
        // The space before each city lets us distinguish between selection from the list
        // and just entering a string, under the theory that disambiguation is unnecessary
        // if the user picked one of these but necessary if the user just typed (or spoke)
        // that name.
        " Los Angeles",
        " Denver",
        " Chicago",
        " New York",
        " Santiago",
        " Rio de Janeiro",
        " Dakar",
        " London",
        " Paris",
        " Cairo",
        " Moscow",
        " Dubai",
        " Delhi",
        " Dhaka",
        " Bangkok",
        " Hong Kong",
        " Tokyo",
        " Sydney",
        " Nouméa",
        " Auckland",
        " Pago Pago",
        " Honolulu",
        " Anchorage",
    };
    private static final int NUM_POPULAR_CITIES = POPULAR_CITIES.length;
    private static String[] sCitiesWithInstructions;

    private CityInfo[] mMatchingCities;
    private boolean mMatchesTruncated = false;

    private boolean mRestarting = false;
    private GeoNames mGeoNames = null;
    private WearableRecyclerView mRecyclerView;
    private ListAdapter mListAdapter;

    private void askUserToSearch() {
        if (sCitiesWithInstructions == null) {
            sCitiesWithInstructions = Arrays.copyOf(POPULAR_CITIES, NUM_POPULAR_CITIES + 2);

            // Add note at end of list so user knows this isn't the complete list.
            sCitiesWithInstructions[NUM_POPULAR_CITIES] = getString(R.string.additional_city_help_1);
            sCitiesWithInstructions[NUM_POPULAR_CITIES + 1] = getString(R.string.additional_city_help_2);
        }
        Bundle remoteInputExtras = new Bundle();
        remoteInputExtras.putBoolean(RemoteInputConstants.EXTRA_DISALLOW_EMOJI, true);
        remoteInputExtras.putInt(RemoteInputConstants.EXTRA_INPUT_ACTION_TYPE, RemoteInputConstants.INPUT_ACTION_TYPE_SEARCH);
        RemoteInput[] remoteInputs = new RemoteInput[] {
            new RemoteInput.Builder(KEY_SEARCH_TEXT)
            .setLabel(getResources().getText(R.string.config_search_for_city))
            .setChoices(sCitiesWithInstructions)
            .setAllowFreeFormInput(true)
            .addExtras(remoteInputExtras)
            .build()
        };
        Intent intent = new Intent(RemoteInputIntent.ACTION_REMOTE_INPUT);
        intent.putExtra(RemoteInputIntent.EXTRA_REMOTE_INPUTS, remoteInputs);
        startActivityForResult(intent, REQUEST_SEARCH);
    }

    private void askUserToSelectMatchingCity() {
        if (mMatchingCities == null || mMatchingCities.length == 0) {
            throw new Error("Internal error: askUserToSelectMatchingCity called with no mMatchingCities");
        }
        int listSize = mMatchingCities.length;
        if (mMatchesTruncated) {
            listSize += 1;
        }
        String[] cityNames = new String[listSize];
        for (int i = 0; i < mMatchingCities.length; i++) {
            cityNames[i] = 
                mMatchingCities[i].name + ", " + 
                mMatchingCities[i].regionName + ", " + 
                mMatchingCities[i].countryCode;
        }
        if (mMatchesTruncated) {
            cityNames[mMatchingCities.length] = "...";
        }
        int[] imageResources = new int[listSize];
        for (int i = 0; i < listSize; i++) {
            imageResources[i] = R.drawable.list_view_image;
        }

        mListAdapter = new ListAdapter(cityNames, imageResources, new ListAdapter.TapListener() {
                @Override
                public void onItemTapped(int itemNumber) {
                    LocationSelectActivity.this.onListItemClicked(itemNumber);
                }
                @Override
                public void onCancel() {
                    // Do nothing for now
                }
            });
        mRecyclerView.setAdapter(mListAdapter);
    }

    private void onListItemClicked(int position) {
        Log.d(TAG, "onListItemClicked at position " + position);
        if (position == mMatchingCities.length) {
            // It's the placeholder for truncated cities.
            Log.d(TAG, "Placeholder '...' selected, restarting");
            Toast toast = Toast.makeText(this, R.string.too_many_city_matches_toast_text, Toast.LENGTH_LONG);
            toast.show();
            return;
        }
        Intent returnIntent = new Intent();
        returnIntent.putExtra(CITY_INFO, mMatchingCities[position]);
        mMatchingCities = null;
        setResult(RESULT_OK, returnIntent);
        finish();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate start");
        mRestarting = false;

        setContentView(R.layout.activity_location_chooser);
        mRecyclerView = (WearableRecyclerView) findViewById(R.id.recycler_view);

        // Don't display the adapter until we have a list of cities.
    }

    @Override
    protected void onStart() {
        super.onStart();
        Log.d(TAG, "onStart");
        if (mGeoNames == null) {
            mGeoNames = new GeoNames();
        }
    }
 
    // onPostCreate is called after both onStart() and onRestoreInstanceState(Bundle).
    @Override
    protected void onPostCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onPostCreate");
        if (mRestarting) {
            mRestarting = false;
        } else {
            askUserToSearch();
        }
        super.onPostCreate(savedInstanceState);
    }

    @Override
    protected void onRestart() {
        super.onRestart();
        Log.d(TAG, "onRestart");
        mRestarting = true;
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.d(TAG, "onPause");
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        Log.d(TAG, "onSaveInstanceState");

        if (mMatchingCities != null) {
            outState.putParcelableArray(KEY_MATCHING_CITIES, mMatchingCities);
        }
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        Log.d(TAG, "onRestoreInstanceState");

        if (savedInstanceState != null) {
            mMatchingCities = (CityInfo[])savedInstanceState.getParcelableArray(KEY_MATCHING_CITIES);
            mRestarting = true;
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.d(TAG, "onResume");
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "onDestroy " + (isFinishing() ? "from finish()" : "NOT from finish()"));
        mGeoNames = null;
        mMatchingCities = null;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent intent) {
        if (requestCode == REQUEST_SEARCH) {
            if (intent != null) {
                Bundle results = RemoteInput.getResultsFromIntent(intent);
                if (results != null) {
                    String searchText = results.getCharSequence(KEY_SEARCH_TEXT).toString();
                    Log.d(TAG, "onActivityResult got '" + searchText + "'");
                
                    // Check for leading space, which indicates a city chosen from the list.
                    boolean cityWasChosenFromList = searchText.startsWith(" ");
                    if (cityWasChosenFromList) {
                        Log.d(TAG, "...chosen from list, so no further disambiguation necessary");
                        searchText = searchText.substring(1);
                    }

                    // Check against magic end token.  If the user selected that, just put them
                    // back into the previous potentially-narrowed list.  This will have the wonderful side
                    // effect of scrolling back to the top of the list. :)
                    if (searchText.equals(getString(R.string.additional_city_help_1)) ||
                        searchText.equals(getString(R.string.additional_city_help_2))) {
                        Log.d(TAG, "... was one of instructional fake end items, restarting");
                        askUserToSearch();
                        return;
                    }

                    // Go get matched cities.
                    mGeoNames.searchForCityNameFragment(searchText, false /* proximity */);
                    int numMatches = mGeoNames.numMatches();
                    Log.d(TAG, "... found " + numMatches + " matches:");
                    if (numMatches == 0) {
                        Log.d(TAG, "... ... no matches, restarting");
                        Toast toast = Toast.makeText(this, R.string.no_city_matches_toast_text, Toast.LENGTH_LONG);
                        toast.show();
                        askUserToSearch();
                        return;
                    }
                    if (numMatches > 30) {
                        numMatches = 30;
                        mMatchesTruncated = true;
                    } else {
                        mMatchesTruncated = false;
                    }
                    if (cityWasChosenFromList) {
                        mGeoNames.selectNthTopCity(0);                
                        CityInfo cityInfo = new CityInfo(mGeoNames);
                    
                        Intent returnIntent = new Intent();
                        returnIntent.putExtra(CITY_INFO, cityInfo);
                        setResult(RESULT_OK, returnIntent);
                        finish();
                    }
                    mMatchingCities = new CityInfo[numMatches];
                    for (int i = 0; i < numMatches; i++) {
                        mGeoNames.selectNthTopCity(i);
                        mMatchingCities[i] = new CityInfo(mGeoNames);
                        Log.d(TAG, "... " + 
                              mMatchingCities[i].name + ", " + 
                              mMatchingCities[i].regionName + ", " + 
                              mMatchingCities[i].countryCode + " : " + 
                              mMatchingCities[i].tzName);
                    }
                    askUserToSelectMatchingCity();
                } else {
                    Log.d(TAG, "onActivityResult got no results");
                    askUserToSearch();
                }
            } else {
                Log.d(TAG, "onActivityResult got no intent, assuming canceled");
                setResult(RESULT_CANCELED, null);
                finish();
            }
        }
    }

}
