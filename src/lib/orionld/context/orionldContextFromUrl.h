#ifndef SRC_LIB_ORIONLD_CONTEXT_ORIONLDCONTEXTFROMURL_H_
#define SRC_LIB_ORIONLD_CONTEXT_ORIONLDCONTEXTFROMURL_H_

/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
#include "orionld/types/OrionldContext.h"                        // OrionldContext



// -----------------------------------------------------------------------------
//
// contextDownloadListInit - initialize the 'context download list'
//
extern void contextDownloadListInit(void);



// -----------------------------------------------------------------------------
//
// contextDownloadListRelease - release all items in the 'context download cache'
//
// The cache is self-cleaning and this function isn't really necessary - except perhaps
// if the broker is killed while serving requests, e.g. while running tests.
//
// This function is ONLY called from the main exit-function, to avoid leaks for valgrind tests.
//
extern void contextDownloadListRelease(void);



// -----------------------------------------------------------------------------
//
// orionldContextFromUrl -
//
extern OrionldContext* orionldContextFromUrl(char* url, char* id);

#endif  // SRC_LIB_ORIONLD_CONTEXT_ORIONLDCONTEXTFROMURL_H_
