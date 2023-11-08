This is a library intended for a portable version of various Emerald Sequoia apps.

Note that as of this writing (2023) it is only used by the following apps:

*   The WearOS (aka Android) version of Emerald Chronometer
*   Emerald Observatory
*   Emerald Timestamp (for selecting an appropriate NTP pool server)

The aim of the library is to provide a location to the app, either from the device
or from the user's selection. It contains a copy of the GeoNames database so that
the user can search for and select a specific city (this capability is currently
only used by the Chronometer app when choosing a location, either, on iOS, for the
app as a whole, or for the Terra/Gaia watch faces on both iOS and WearOS.

There are scripts to convert from GeoNames format into the binary
format shipped with the apps, but they have almost never been used
since the database was originally captured from the GeoNames
source. The data files are included directly in each app binary build as assets
(they aren't in the libraries themselves as the libraries just have
code in them).
