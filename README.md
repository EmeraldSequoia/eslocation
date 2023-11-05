This is a library intended for a portable version of various Emerald Sequoia apps.

Note that as of this writing (2023) it is only used by the following apps:

*   The WearOS (aka Android) version of Emerald Chronometer
*   Emerald Observatory
*   Emerald Timestamp (for selecting an appropriate NTP pool server)

The aim of the library is to provide a location to the app, either from the device
or from the user's selection. It contains a copy of the GeoNames database so that
the user can search for and select a specific city (this capability is currently
only used by the Chronometer app).

