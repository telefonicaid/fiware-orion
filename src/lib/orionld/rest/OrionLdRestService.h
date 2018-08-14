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
* ServiceRoutine -
*/
typedef bool (*ServiceRoutine)(ConnectionInfo* ciP);



// -----------------------------------------------------------------------------
//
// OrionLdRestService -
//
// NOTE
//   The initial string '/ngsi-ld/v1/' doesn't count in the four charsBefore fields.
//   That the URL starts with that string will be made sure before we get as far as to
//   lookup a URL.
//
typedef struct OrionLdRestService
{
  char*           url;                           // URL Path 
  ServiceRoutine  serviceRoutine;                // Function pointer to service routine
  int             wildcards;                     // Number of wildcards in URL: 0, 1, or 2
  int             charsBeforeFirstWildcard;      // E.g. 8 for [/ngsi-ld/v1]/entities/*
  int             charsBeforeFirstWildcardSum;   // -"-  'e' + 'n' + 't' + 'i' + 't' + 'i' + 'e' + 's'
  int             charsBeforeSecondWildcard;     // E.g. 5 for [/ngsi-ld/v1]/entities/*/attrs/*
  int             charsBeforeSecondWildcardSum;  // -"- 'a' + 't' + 't' + 'r' + 's' - used to find start of wildcard no 2 
  char            matchForSecondWildcard[16];    // E.g. "/attrs/" for [/ngsi-ld/v1]/entities/*/attrs/*
  int             matchForSecondWildcardLen;     // strlen of last path to match
  int             supportedVerbMask;             // Bitmask of supported verbs
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

#endif  // SRC_LIB_REST_ORIOMLDRESTSERVICE_H_
