//
//  ESDeviceLocationManager.cpp
//
//  Created by Steve Pucci 11 Jun 2011
//  Copyright Emerald Sequoia LLC 2011. All rights reserved.
//

#include "ESDeviceLocationManager.hpp"
#include "ESLocation.hpp"

/*static*/ double          ESDeviceLocationManager::_lastLatitudeDegrees;
/*static*/ double          ESDeviceLocationManager::_lastLongitudeDegrees;
/*static*/ double          ESDeviceLocationManager::_lastAltitudeMeters;
/*static*/ double          ESDeviceLocationManager::_lastAltitudeAccuracyMeters;
/*static*/ bool            ESDeviceLocationManager::_lastLocationValid;
/*static*/ ESTimeInterval  ESDeviceLocationManager::_lastLocationTimestamp;
/*static*/ double          ESDeviceLocationManager::_lastLocationAccuracyMeters;

/*static*/ void 
ESDeviceLocationManager::userSaidNo() {
    ESLocation::stopUpdating(true/*userSaidNo*/);  // The user said no, dammit
}

