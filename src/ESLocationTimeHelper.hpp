//
//  ESLocationTimeHelper.hpp
//
//  Created by Steven Pucci 28 Jul 2011
//  Copyright Emerald Sequoia LLC 2011. All rights reserved.
//

#ifndef _ESLOCATIONTIMEHELPER_HPP_
#define _ESLOCATIONTIMEHELPER_HPP_

#include "ESLocation.hpp"

class ESChildThread;
class ESGeoNames;

/*! This class is used to assist the ESTime module in determining an
 *  appropriate NTP server to use.  There is a bit of an ordering
 *  dependency between ESTime and ESLocation:  Simplistically, ESTime
 *  wants to depend on ESLocation, because ESTime wants to know what
 *  server to use.  But the dependency the other direction is much
 *  more direct:  ESTimers (necessarily) live in ESTime, and in
 *  general the location code wants support from ESTime for time zone
 *  APIs.  So we handle this via this helper class, which lives in
 *  ESLocation and whose job it is to *tell* ESTime (without being
 *  asked) what the device location is, at startup and when the
 *  location changes sufficiently.
 *
 *  The only public API here is the ctor (and dtor).  All you need
 *  to do is create a helper and then it does all the work, continuing
 *  to do all of the work until it is destroyed.
 */
class ESLocationTimeHelper : ESLocationObserver {
  public:
                            ESLocationTimeHelper();
                            ~ESLocationTimeHelper();

    /*virtual*/ void        newLocationAvailable(ESLocation *location);

    // Methods called by internals
    void                    notifyTime(ESGeoNames *geoNames);
    void                    doHelpInThread(ESGeoNames *geoNames);
    bool                    countryCheckNeeded();

  private:
    void                    doHelp();
    ESChildThread           *_thread;
    bool                    _firstNotificationDone;
    double                  _lastNotifiedLatitudeDegrees;
    double                  _lastNotifiedLongitudeDegrees;
};

#endif  // _ESLOCATIONTIMEHELPER_HPP_
