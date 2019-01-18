#ifndef SRC_LIB_ORIONLD_CONTEXT_ORIONLDCORECONTEXT_H_
#define SRC_LIB_ORIONLD_CONTEXT_ORIONLDCORECONTEXT_H_

/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
extern "C"
{
#include "kjson/KjNode.h"                               // KjNode
}

#include "orionld/context/OrionldContext.h"             // OrionldContext



// -----------------------------------------------------------------------------
//
// ORIONLD_CORE_CONTEXT_URL -
//
#define ORIONLD_CORE_CONTEXT_URL (char*) \
  "http://uri.etsi.org/ngsi-ld/v1/ngsi-ld-core-context.jsonld"


// -----------------------------------------------------------------------------
//
// ORIONLD_DEFAULT_URL_CONTEXT_URL -
//
#define ORIONLD_DEFAULT_URL_CONTEXT_URL (char*) \
  "https://forge.etsi.org/gitlab/NGSI-LD/NGSI-LD/raw/master/defaultContext/defaultContextVocab.jsonld"



// -----------------------------------------------------------------------------
//
// ORIONLD_DEFAULT_CONTEXT_URL -
//
#define ORIONLD_DEFAULT_CONTEXT_URL (char*) \
  "https://forge.etsi.org/gitlab/NGSI-LD/NGSI-LD/raw/master/defaultContext/defaultContext.jsonld"



// -----------------------------------------------------------------------------
//
// orionldCoreContext
//
extern OrionldContext orionldCoreContext;



// -----------------------------------------------------------------------------
//
// orionldDefaultUrlContext
//
extern OrionldContext orionldDefaultUrlContext;



// -----------------------------------------------------------------------------
//
// orionldDefaultContext
//
extern OrionldContext orionldDefaultContext;



// -----------------------------------------------------------------------------
//
// orionldCoreContextString - to avoid download during functest
//
extern const char* orionldCoreContextString;



// -----------------------------------------------------------------------------
//
// orionldDefaultUrlContextString - to avoid download during functest
//
extern const char* orionldDefaultUrlContextString;



// -----------------------------------------------------------------------------
//
// orionldDefaultContextString - to avoid download during functest
//
extern const char* orionldDefaultContextString;



// -----------------------------------------------------------------------------
//
// orionldDefaultUrl -
//
extern char* orionldDefaultUrl;



// -----------------------------------------------------------------------------
//
// orionldDefaultUrlLen -
//
extern int orionldDefaultUrlLen;

#endif  // SRC_LIB_ORIONLD_CONTEXT_ORIONLDCORECONTEXT_H_
