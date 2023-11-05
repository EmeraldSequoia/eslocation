//
//  ESTimeLocEnvironment.hpp
//
//  Created by Steven Pucci 26 Aug 2012
//  Copyright Emerald Sequoia LLC 2012. All rights reserved.
//

#ifndef _ESTIMELOCENVIRONMENT_HPP_
#define _ESTIMELOCENVIRONMENT_HPP_

#include "ESTimeEnvironment.hpp"

class ESLocation;

// *****************************************************************
// ESTimeLocEnvironment
//
// The ECWatchEnvironment used by Chronometer was split into
// multiple parts.  ESTimeEnvironment records merely the timezone
// and associated caches, so that ESWatchTime can be implemented in
// terms of it.
//
// This class (ESTimeLocEnvironment) is built on ESTimeEnvironment, andd adds
// location information and caching.
// ESTimeLocAstroEnvironment is built on this class, and additionally includes an astro mgr.
// *****************************************************************

class ESTimeLocEnvironment : public ESTimeEnvironment {
  public:
                            ESTimeLocEnvironment(const char *timeZoneName,
                                                 bool       observingIPhoneTime);
                            ESTimeLocEnvironment(const char *timeZoneName,
                                                 bool       observingIPhoneTime,
                                                 const char *locationPrefsPrefix);
                            ESTimeLocEnvironment(const char *timeZoneName,
                                                 const char *cityName,
                                                 double     latitudeInDegrees,
                                                 double     longitudeInDegrees);
                            ~ESTimeLocEnvironment();
    ESLocation              *location();
    std::string             cityName();

    virtual bool            isLocationEnv() const { return true; }

  private:
    std::string             _cityName;
    ESLocation              *_location;
};

// Include this only once, here
#include "ESTimeLocEnvironmentInl.hpp"

#endif  // _ESTIMELOCENVIRONMENT_HPP_
