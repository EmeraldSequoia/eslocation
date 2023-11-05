//
//  ESTimeLocEnvironment.cpp
//
//  Created by Steven Pucci 27 Aug 2012
//  Copyright Emerald Sequoia LLC 2012. All rights reserved.
//

#include "ESTimeLocEnvironment.hpp"
#include "ESLocation.hpp"

ESTimeLocEnvironment::ESTimeLocEnvironment(const char *timeZoneName,
                                           bool       observingIPhoneTime)
:   ESTimeEnvironment(timeZoneName, observingIPhoneTime),
    _location(new ESLocation("EOPrimaryLocation"))
{
}


ESTimeLocEnvironment::ESTimeLocEnvironment(const char *timeZoneName,
                                           bool       observingIPhoneTime,
                                           const char *locationPrefsPrefix)
:   ESTimeEnvironment(timeZoneName, observingIPhoneTime),
    _location(new ESLocation(locationPrefsPrefix))
{
}


ESTimeLocEnvironment::ESTimeLocEnvironment(const char *timeZoneName,
                                           const char *cityName,
                                           double     latitudeInDegrees,
                                           double     longitudeInDegrees) 
:   ESTimeEnvironment(timeZoneName, false /* observingIPhoneTime */),
    _location(new ESLocation(latitudeInDegrees, longitudeInDegrees)),
    _cityName(cityName)
{
}

ESTimeLocEnvironment::~ESTimeLocEnvironment() {
    delete _location;
}
