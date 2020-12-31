#ifndef SRC_LIB_ORIONLD_REST_ORIONLDRESTSERVICE_H_
#define SRC_LIB_ORIONLD_REST_ORIONLDRESTSERVICE_H_

/*
*
* Copyright 2018 FIWARE Foundation e.V.
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
#include "rest/ConnectionInfo.h"



// -----------------------------------------------------------------------------
//
// OrionldServiceRoutine -
//
typedef bool (*OrionldServiceRoutine)(ConnectionInfo* ciP);



// -----------------------------------------------------------------------------
//
// OrionldTroeRoutine -
//
typedef bool (*OrionldTroeRoutine)(ConnectionInfo* ciP);



// -----------------------------------------------------------------------------
//
// OrionLdRestServiceSimplified -
//
// This struct is a simplified OrionLdRestService.
// To create an OrionLd service, all that is needed is the URL and the service routine.
// In the initialization stage, the URL is parsed and data is taken out of it to make
// the URL parse (service routine lookup) faster.
// A lot faster actually, as string comparisons are avoided and instead integers are compared.
//
// The info extracted from this initialization stage, plus the url and service routine, is
// stored in the "real" OrionLdRestService struct, which is used during lookup of URL->service-routine.
// The struct OrionLdRestServiceSimplified is no longer used after the creation of the OrionLdRestService items.
// However, the URL is not copied to the OrionLdRestService items, but just pointed to from OrionLdRestService to
// OrionLdRestServiceSimplified, so, the OrionLdRestServiceSimplified vectors must stay intact during
// the entire lifetime of the broker.
//
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
#define ORIONLD_SERVICE_OPTION_MAKE_SURE_TENANT_EXISTS               (1 << 3)



// -----------------------------------------------------------------------------
//
// URI Parameters
//
#define ORIONLD_URIPARAM_LIMIT                (1 << 0)
#define ORIONLD_URIPARAM_OFFSET               (1 << 1)
#define ORIONLD_URIPARAM_IDLIST               (1 << 2)
#define ORIONLD_URIPARAM_TYPELIST             (1 << 3)
#define ORIONLD_URIPARAM_IDPATTERN            (1 << 4)
#define ORIONLD_URIPARAM_ATTRS                (1 << 5)
#define ORIONLD_URIPARAM_Q                    (1 << 6)
#define ORIONLD_URIPARAM_GEOREL               (1 << 7)
#define ORIONLD_URIPARAM_GEOMETRY             (1 << 8)
#define ORIONLD_URIPARAM_COORDINATES          (1 << 9)
#define ORIONLD_URIPARAM_GEOPROPERTY          (1 << 10)
#define ORIONLD_URIPARAM_GEOMETRYPROPERTY     (1 << 11)
#define ORIONLD_URIPARAM_CSF                  (1 << 12)
#define ORIONLD_URIPARAM_OPTIONS              (1 << 13)
#define ORIONLD_URIPARAM_COUNT                (1 << 14)
#define ORIONLD_URIPARAM_DATASETID            (1 << 15)
#define ORIONLD_URIPARAM_DELETEALL            (1 << 16)
#define ORIONLD_URIPARAM_TIMEPROPERTY         (1 << 17)
#define ORIONLD_URIPARAM_TIMEREL              (1 << 18)
#define ORIONLD_URIPARAM_TIMEAT               (1 << 19)
#define ORIONLD_URIPARAM_ENDTIMEAT            (1 << 20)
#define ORIONLD_URIPARAM_DETAILS              (1 << 21)
#define ORIONLD_URIPARAM_PRETTYPRINT          (1 << 22)
#define ORIONLD_URIPARAM_SPACES               (1 << 23)



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
  OrionldTroeRoutine     troeRoutine;                   // Function pointer to routines that saves temporal values
  int                    wildcards;                     // Number of wildcards in URL: 0, 1, or 2
  int                    charsBeforeFirstWildcard;      // E.g. 9 for [/ngsi-ld/v1/]entities/*
  int                    charsBeforeFirstWildcardSum;   // -"-  'e' + 'n' + 't' + 'i' + 't' + 'i' + 'e' + 's'
  int                    charsBeforeSecondWildcard;     // E.g. 5 for [/ngsi-ld/v1]/entities/*/attrs/*
  int                    charsBeforeSecondWildcardSum;  // -"- 'a' + 't' + 't' + 'r' + 's' - used to find start of wildcard no 2
  char                   matchForSecondWildcard[16];    // E.g. "/attrs/" for [/ngsi-ld/v1]/entities/*/attrs/*
  int                    matchForSecondWildcardLen;     // strlen of last path to match
  uint32_t               options;                       // Peculiarities of this type of requests (bitmask)
  uint32_t               uriParams;                     // Supported URI parameters (bitmask)
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
