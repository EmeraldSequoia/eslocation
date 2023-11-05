//
//  ESTimeLocEnvironmentInl.hpp
//
//  Created by Steven Pucci 26 Aug 2012
//  Copyright Emerald Sequoia LLC 2012. All rights reserved.
//

#ifndef _ESTIMELOCENVIRONMENT_HPP_
#error "Include this file only from ESTimeLocEnvironment.hpp"
#endif

#ifdef _ESTIMELOCENVIRONMENT_INL_HPP_
#error "Include this file only once from ESTimeLocEnvironment.hpp"
#endif

#define _ESTIMELOCENVIRONMENT_INL_HPP_

#include "ESErrorReporter.hpp"

inline ESLocation *
ESTimeLocEnvironment::location() {
    return _location;
}

inline std::string
ESTimeLocEnvironment::cityName() {
    return _cityName;
}

// NOTE: The following is a method on ESTimeEnvironment *which is only defined here*
inline ESTimeLocEnvironment *
ESTimeEnvironment::asLocationEnv() {
    ESAssert(isLocationEnv()); return (ESTimeLocEnvironment *)this;
}
