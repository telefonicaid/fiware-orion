#ifndef SRC_LIB_ORIONLD_COMMON_PERFORMANCE_H_
#define SRC_LIB_ORIONLD_COMMON_PERFORMANCE_H_

/*
*
* Copyright 2021 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
extern "C"
{
#include "kbase/kTime.h"                                         // kTimeGet
}


// ----------------------------------------------------------------------------
//
// REQUEST_PERFORMANCE
//
#define REQUEST_PERFORMANCE 1

#ifdef REQUEST_PERFORMANCE

// -----------------------------------------------------------------------------
//
// PERFORMANCE -
//
#define PERFORMANCE(timeField)            \
do {                                      \
  kTimeGet(&timestamps.timeField);        \
} while (0)



// -----------------------------------------------------------------------------
//
// PERFORMANCE_BEGIN -
//
#define PERFORMANCE_BEGIN(ix, desc)       \
do {                                      \
  timestamps.srDesc[ix] = (char*) desc;   \
  kTimeGet(&timestamps.srStart[ix]);      \
} while (0)



// -----------------------------------------------------------------------------
//
// PERFORMANCE_END -
//
// Possibility to change the description, in case the measure ends in a different path (error)
//
#define PERFORMANCE_END(ix, desc)         \
do {                                      \
  kTimeGet(&timestamps.srEnd[ix]);        \
  if (desc != NULL)                       \
    timestamps.srDesc[ix] = (char*) desc; \
} while (0)

#else

#define	PERFORMANCE(timeField)
#define PERFORMANCE_BEGIN(ix, desc)
#define PERFORMANCE_END(ix, desc)

#endif

#endif  // SRC_LIB_ORIONLD_COMMON_PERFORMANCE_H_
