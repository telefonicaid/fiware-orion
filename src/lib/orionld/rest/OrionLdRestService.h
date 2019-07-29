#ifndef SRC_LIB_ORIONLD_REST_ORIONLDRESTSERVICE_H_
#define SRC_LIB_ORIONLD_REST_ORIONLDRESTSERVICE_H_

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
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* OrionldServiceRoutine -
*/
typedef bool (*OrionldServiceRoutine)(ConnectionInfo* ciP);



/* ****************************************************************************
*
* OrionLdRestServiceSimplified -
*
* This struct is a simplified OrionLdRestService.
* To create an OrionLd service, all that is needed is the URL and the service routine.
* In the initialization stage, the URL is parsed and data is taken out of it to make
* the URL parse (service routine lookup) faster.
* A lot faster actually, as string comparisons are avoided and instead integers are compared.
*
* The info extracted from this initialization stage, plus the url and service routine, is
* stored in the "real" OrionLdRestService struct, which is used during lookup of URL->service-routine.
* The struct OrionLdRestServiceSimplified is no longer used after the creation of the OrionLdRestService items.
* However, the URL is not copied to the OrionLdRestService items, but just pointed to from OrionLdRestService to
* OrionLdRestServiceSimplified, so, the OrionLdRestServiceSimplified vectors must stay intact during
* the entire lifetime of the broker.
*/
typedef struct OrionLdRestServiceSimplified
{
  const char*            url;
  OrionldServiceRoutine  serviceRoutine;
} OrionLdRestServiceSimplified;



// -----------------------------------------------------------------------------
//
// OrionLdRestServiceSimplifiedVector -
//
typedef struct OrionLdRestServiceSimplifiedVector
{
  OrionLdRestServiceSimplified*  serviceV;
  int                            services;
} OrionLdRestServiceSimplifiedVector;



// -----------------------------------------------------------------------------
//
// ORION_LD_SERVICE_PREFIX_LEN -
//
// This is the length of the prefix of URL paths for orionld services.
// The prefix is "/ngsi-ld/" and its string length is 9
//
#define ORION_LD_SERVICE_PREFIX_LEN 9



// -----------------------------------------------------------------------------
//
// Options
//
#define ORIONLD_SERVICE_OPTION_PREFETCH_ID_AND_TYPE                  (1 << 0)
#define ORIONLD_SERVICE_OPTION_CREATE_CONTEXT                        (1 << 1)
#define ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD  (1 << 2)


// -----------------------------------------------------------------------------
//
// OrionLdRestService -
//
// NOTE
//   The initial string '/ngsi-ld/' doesn't count in the four charsBefore fields.
//   That the URL starts with that string will be made sure before we get as far as to
//   lookup a URL.
//
typedef struct OrionLdRestService
{
  char*                  url;                           // URL Path
  OrionldServiceRoutine  serviceRoutine;                // Function pointer to service routine
  int                    wildcards;                     // Number of wildcards in URL: 0, 1, or 2
  int                    charsBeforeFirstWildcard;      // E.g. 9 for [/ngsi-ld/v1/]entities/*
  int                    charsBeforeFirstWildcardSum;   // -"-  'e' + 'n' + 't' + 'i' + 't' + 'i' + 'e' + 's'
  int                    charsBeforeSecondWildcard;     // E.g. 5 for [/ngsi-ld/v1]/entities/*/attrs/*
  int                    charsBeforeSecondWildcardSum;  // -"- 'a' + 't' + 't' + 'r' + 's' - used to find start of wildcard no 2
  char                   matchForSecondWildcard[16];    // E.g. "/attrs/" for [/ngsi-ld/v1]/entities/*/attrs/*
  int                    matchForSecondWildcardLen;     // strlen of last path to match
  uint32_t               options;                       // Peculiarities of this type of requests
} OrionLdRestService;



// -----------------------------------------------------------------------------
//
// OrionLdRestServiceVector -
//
typedef struct OrionLdRestServiceVector
{
  OrionLdRestService*  serviceV;
  int                  services;
} OrionLdRestServiceVector;

#endif  // SRC_LIB_ORIONLD_REST_ORIONLDRESTSERVICE_H_
