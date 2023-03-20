#ifndef SRC_LIB_ORIONLD_TYPES_ORIONLDHEADER_H_
#define SRC_LIB_ORIONLD_TYPES_ORIONLDHEADER_H_

/*
*
* Copyright 2022 FIWARE Foundation e.V.
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



// -----------------------------------------------------------------------------
//
// OrionldHeaderType -
//
typedef enum OrionldHeaderType
{
  HttpContentLength,
  HttpContentType,
  HttpAccept,
  HttpLink,            // NGSI-LD only
  HttpHost,
  HttpAllow,
  HttpExpect,
  HttpOrigin,
  HttpConnection,
  HttpUserAgent,
  HttpTenant,          // NGSI-LD only
  HttpScope,           // NGSI-LD only
  HttpResultsCount,    // NGSI-LD only
  HttpLocation,
  HttpService,         // NGSI-v1/2 only
  HttpServicePath,     // NGSI-v1/2 only
  HttpXAuthToken,
  HttpXForwardedFor,
  HttpXRealIp,
  HttpAttrsFormat,
  HttpCorrelator,
  HttpNgsiv2Count,     // NGSI-v1/2 only
  HttpAllowOrigin,     // CORS
  HttpAllowHeaders,    // CORS
  HttpAllowMethods,    // CORS
  HttpMaxAge,          // CORS
  HttpExposeHeaders,   // CORS
  HttpAcceptPatch,
  HttpPerformance,
  HttpEntityMap        // For distributed GET /entities
} OrionldHeaderType;



// -----------------------------------------------------------------------------
//
// OrionldHeader -
//
typedef struct OrionldHeader
{
  OrionldHeaderType type;
  char*             sValue;
  int               iValue;
} OrionldHeader;



// -----------------------------------------------------------------------------
//
// OrionldHeaderSet -
//
typedef struct OrionldHeaderSet
{
  OrionldHeader* headerV;
  int            ix;
  int            size;
} OrionldHeaderSet;



// -----------------------------------------------------------------------------
//
// orionldHeaderName -
//
extern const char* orionldHeaderName[];



// -----------------------------------------------------------------------------
//
// orionldHeaderSetCreate -
//
extern OrionldHeaderSet* orionldHeaderSetCreate(int headers);



// -----------------------------------------------------------------------------
//
// orionldHeaderSetInit -
//
extern bool orionldHeaderSetInit(OrionldHeaderSet* setP, int headers);



// -----------------------------------------------------------------------------
//
// orionldHeaderAdd -
//
extern int orionldHeaderAdd(OrionldHeaderSet* setP, OrionldHeaderType type, const char* sValue, int iValue);

#endif  // SRC_LIB_ORIONLD_TYPES_ORIONLDHEADER_H_
