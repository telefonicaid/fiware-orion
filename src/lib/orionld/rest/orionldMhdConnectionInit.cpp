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
#include <string.h>                                              // strlen
#include <microhttpd.h>                                          // MHD

extern "C"
{
#include "kbase/kMacros.h"                                       // K_FT, K_VEC_SIZE
#include "kbase/kTime.h"                                         // kTimeGet, kTimeDiff
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "common/wsStrip.h"                                      // wsStrip
#include "common/MimeType.h"                                     // mimeTypeParse
#include "rest/Verb.h"                                           // Verb
#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "orionld/common/orionldErrorResponse.h"                 // OrionldBadRequestData, ...
#include "orionld/common/orionldState.h"                         // orionldState, orionldStateInit
#include "orionld/common/performance.h"                          // REQUEST_PERFORMANCE
#include "orionld/common/tenantList.h"                           // tenant0
#include "orionld/serviceRoutines/orionldBadVerb.h"              // orionldBadVerb
#include "orionld/payloadCheck/pcheckUri.h"                      // pcheckUri
#include "orionld/rest/orionldServiceInit.h"                     // orionldRestServiceV
#include "orionld/rest/orionldServiceLookup.h"                   // orionldServiceLookup
#include "orionld/rest/temporaryErrorPayloads.h"                 // Temporary Error Payloads
#include "orionld/rest/OrionLdRestService.h"                     // ORIONLD_URIPARAM_LIMIT, ...
#include "orionld/rest/orionldMhdConnectionInit.h"               // Own interface



// -----------------------------------------------------------------------------
//
// clientIp - move to orionldState
//
extern __thread char  clientIp[IP_LENGTH_MAX + 1];



// ----------------------------------------------------------------------------
//
// External declarations - tmp - should be in their own files (not rest.cpp) and included here
//
extern MHD_Result httpHeaderGet(void* cbDataP, MHD_ValueKind kind, const char* ckey, const char* value);
extern MHD_Result uriArgumentGet(void* cbDataP, MHD_ValueKind kind, const char* ckey, const char* val);



// -----------------------------------------------------------------------------
//
// connectionInfo - as a thread variable
//
// This to avoid thousands of mallocs/constructor calls every second in a busy broker.
// Unfortunately this doesn't work as long as ConnectionInfo is a "complex class".
// To avoid the call to malloc/free for every connection (thousands per second),
// we would need to simplify ConnectionInfo a little.
// Seems easy enough.
//
// thread_local ConnectionInfo connectionInfo = {};
//



// -----------------------------------------------------------------------------
//
// verbGet
//
Verb verbGet(const char* method)
{
  int sLen = strlen(method);

  if (sLen < 3)
    return NOVERB;

  char c0   = method[0];
  char c1   = method[1];
  char c2   = method[2];
  char c3   = method[3];

  if (sLen == 3)
  {
    if ((c0 == 'G') && (c1 == 'E') && (c2 == 'T') && (c3 == 0))
      return GET;
    if ((c0 == 'P') && (c1 == 'U') && (c2 == 'T') && (c3 == 0))
      return PUT;
  }
  else if (sLen == 4)
  {
    char c4 = method[4];

    if ((c0 == 'P') && (c1 == 'O') && (c2 == 'S') && (c3 == 'T') && (c4 == 0))
      return POST;
  }
  else if (sLen == 6)
  {
    char c4 = method[4];
    char c5 = method[5];
    char c6 = method[6];

    if ((c0 == 'D') && (c1 == 'E') && (c2 == 'L') && (c3 == 'E') && (c4 == 'T') && (c5 == 'E') && (c6 == 0))
      return DELETE;
  }
  else if (sLen == 5)
  {
    char c4 = method[4];
    char c5 = method[5];

    if ((c0 == 'P') && (c1 == 'A') && (c2 == 'T') && (c3 == 'C') && (c4 == 'H') && (c5 == 0))
      return PATCH;
  }
  else if (sLen == 7)
  {
    char c4 = method[4];
    char c5 = method[5];
    char c6 = method[6];
    char c7 = method[7];

    if ((c0 == 'O') && (c1 == 'P') && (c2 == 'T') && (c3 == 'I') && (c4 == 'O') && (c5 == 'N') && (c6 == 'S') && (c7 == 0))
      return OPTIONS;
  }

  return NOVERB;
}



// -----------------------------------------------------------------------------
//
// ipAddressAndPort -
//
static void ipAddressAndPort(void)
{
  const union MHD_ConnectionInfo* mciP = MHD_get_connection_info(orionldState.mhdConnection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);

  if (mciP != NULL)
  {
    struct sockaddr* addr = (struct sockaddr*) mciP->client_addr;

    snprintf(clientIp, sizeof(clientIp), "%d.%d.%d.%d",
             addr->sa_data[2] & 0xFF,
             addr->sa_data[3] & 0xFF,
             addr->sa_data[4] & 0xFF,
             addr->sa_data[5] & 0xFF);
  }
  else
    strncpy(clientIp, "IP unknown", sizeof(clientIp) - 1);
}



// -----------------------------------------------------------------------------
//
// optionsParse -
//
static void optionsParse(const char* options)
{
  char* optionStart = (char*) options;
  char* cP          = (char*) options;

  while (1)
  {
    if ((*cP == ',') || (*cP == 0))  // Found the end of an option
    {
      bool done  = (*cP == 0);
      char saved = *cP;

      *cP = 0;  // Zero-terminate

      if      (strcmp(optionStart, "update")        == 0)  orionldState.uriParamOptions.update        = true;
      else if (strcmp(optionStart, "replace")       == 0)  orionldState.uriParamOptions.replace       = true;
      else if (strcmp(optionStart, "noOverwrite")   == 0)  orionldState.uriParamOptions.noOverwrite   = true;
      else if (strcmp(optionStart, "keyValues")     == 0)  orionldState.uriParamOptions.keyValues     = true;
      else if (strcmp(optionStart, "sysAttrs")      == 0)  orionldState.uriParamOptions.sysAttrs      = true;
      else if (strcmp(optionStart, "append")        == 0)  orionldState.uriParamOptions.append        = true;  // NGSIv2 compatibility
      else if (strcmp(optionStart, "count")         == 0)  orionldState.uriParams.count               = true;  // NGSIv2 compatibility
      else if (strcmp(optionStart, "values")        == 0)  orionldState.uriParamOptions.values        = true;  // NGSIv2 compatibility
      else if (strcmp(optionStart, "unique")        == 0)  orionldState.uriParamOptions.uniqueValues  = true;  // NGSIv2 compatibility
      else if (strcmp(optionStart, "dateCreated")   == 0)  orionldState.uriParamOptions.dateCreated   = true;  // NGSIv2 compatibility
      else if (strcmp(optionStart, "dateModified")  == 0)  orionldState.uriParamOptions.dateModified  = true;  // NGSIv2 compatibility
      else if (strcmp(optionStart, "noAttrDetail")  == 0)  orionldState.uriParamOptions.noAttrDetail  = true;  // NGSIv2 compatibility
      else if (strcmp(optionStart, "upsert")        == 0)  orionldState.uriParamOptions.upsert        = true;  // NGSIv2 compatibility
      else
      {
        LM_W(("Unknown 'options' value: %s", optionStart));
        orionldState.httpStatusCode = 400;
        orionldErrorResponseCreate(OrionldBadRequestData, "Unknown value for 'options' URI parameter", optionStart);
        return;
      }

      if (done == true)
        break;

      *cP = saved;
      optionStart = &cP[1];
    }

    ++cP;
  }
}



// -----------------------------------------------------------------------------
//
// strSplit -
//
static int strSplit(char* s, char delimiter, char** outV, int outMaxItems)
{
  if (s == NULL)
    return 0;

  if (*s == 0)
    return 0;

  int   outIx = 0;
  char* start = s;

  //
  // Loop over 's':
  // - Search for the delimiter
  // - zero-terminate
  // - assign and
  // - continue
  //
  while (*s != 0)
  {
    if (*s == delimiter)
    {
      *s = 0;
      outV[outIx] = wsStrip(start);
      start = &s[1];

      // Check that the scope starts with a slash, etc ...
      // if (scopeCheck(outV[outIx]) == false) ...

      if (++outIx > outMaxItems)
        return -1;
    }

    ++s;
  }

  outV[outIx] = wsStrip(start);

  ++outIx;

  return outIx;
}



/* ****************************************************************************
*
* contentTypeParse -
*/
MimeType contentTypeParse(const char* contentType, char** charsetP)
{
  char* s;
  char* cP = (char*) contentType;

  if ((s = strstr(cP, ";")) != NULL)
  {
    *s = 0;
    ++s;
    s = wsStrip(s);

    if ((charsetP != NULL) && (strncmp(s, "charset=", 8) == 0))
      *charsetP = &s[8];
  }

  cP = wsStrip(cP);

  if      (strcmp(cP, "*/*") == 0)                  return JSON;
  else if (strcmp(cP, "text/json") == 0)            return JSON;
  else if (strcmp(cP, "application/json") == 0)     return JSON;
  else if (strcmp(cP, "application/ld+json") == 0)  return JSONLD;
  else if (strcmp(cP, "application/geo+json") == 0) return GEOJSON;
  else if (strcmp(cP, "application/html") == 0)     return HTML;
  else if (strcmp(cP, "text/plain") == 0)           return TEXT;

  return JSON;
}



// -----------------------------------------------------------------------------
//
// orionldHttpHeaderGet -
//
static MHD_Result orionldHttpHeaderGet(void* cbDataP, MHD_ValueKind kind, const char* key, const char* value)
{
  LM_TMP(("KZ: Header '%s' = '%s', orionldState.httpStatusCode == %d", key, value, orionldState.httpStatusCode));
  if (strcmp(key, "NGSILD-Scope") == 0)
  {
    orionldState.scopes = strSplit((char*) value, ',', orionldState.scopeV, K_VEC_SIZE(orionldState.scopeV));
    if (orionldState.scopes == -1)
    {
      LM_W(("Bad Input (too many scopes)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "Bad value for HTTP header /NGSILD-Scope/", value);
      orionldState.httpStatusCode = 400;
    }
  }
  else if (strcmp(key, "Ngsiv2-AttrsFormat") == 0)  // FIXME: This header name needs to change for NGSI-LD
  {
    orionldState.attrsFormat = (char*) value;
  }
  else if (strcmp(key, "X-Auth-Token") == 0)
  {
    orionldState.xAuthToken = (char*) value;
  }
  else if (strcmp(key, "Fiware-Correlator") == 0)
  {
    orionldState.correlator = (char*) value;
  }
  else if (strcmp(key, "Content-Type") == 0)
  {
    orionldState.in.contentType = contentTypeParse(value, NULL);
  }
  LM_TMP(("KZ: Header '%s' = '%s', orionldState.httpStatusCode == %d", key, value, orionldState.httpStatusCode));

  return MHD_YES;
}



// -----------------------------------------------------------------------------
//
// orionldUriArgumentGet -
//
MHD_Result orionldUriArgumentGet(void* cbDataP, MHD_ValueKind kind, const char* key, const char* value)
{
  LM_TMP(("KZ: key: '%s', value: '%s'", key, value));
  if ((value == NULL) || (*value == 0))
  {
    char errorString[256];

    LM_TMP(("KZ: Setting  orionldState.httpStatusCode  to 400"));
    snprintf(errorString, sizeof(errorString) - 1, "Empty right-hand-side for URI param /%s/", key);
    orionldErrorResponseCreate(OrionldBadRequestData, "Error in URI param", errorString);
    orionldState.httpStatusCode = 400;

    return MHD_YES;
  }

  if (strcmp(key, "id") == 0)
  {
    orionldState.uriParams.id = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_IDLIST;
  }
  else if (strcmp(key, "type") == 0)
  {
    orionldState.uriParams.type = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_TYPELIST;
  }
  else if (strcmp(key, "typePattern") == 0)
  {
    orionldState.uriParams.typePattern = (char*) value;
  }
  else if (strcmp(key, "idPattern") == 0)
  {
    orionldState.uriParams.idPattern = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_IDPATTERN;
  }
  else if (strcmp(key, "attrs") == 0)
  {
    orionldState.uriParams.attrs = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_ATTRS;
  }
  else if (strcmp(key, "offset") == 0)
  {
    if (value[0] == '-')
    {
      LM_W(("Bad Input (negative value for /offset/ URI param)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "Bad value for URI parameter /offset/", value);
      orionldState.httpStatusCode = 400;
      return MHD_YES;
    }

    orionldState.uriParams.offset = atoi(value);

    orionldState.uriParams.mask |= ORIONLD_URIPARAM_OFFSET;
  }
  else if (strcmp(key, "limit") == 0)
  {
    if (value[0] == '-')
    {
      LM_W(("Bad Input (negative value for /limit/ URI param)"));
      orionldErrorResponseCreate(OrionldBadRequestData, "Bad value for URI parameter /limit/", value);
      orionldState.httpStatusCode = 400;
      return MHD_YES;
    }

    orionldState.uriParams.limit = atoi(value);

    if (orionldState.uriParams.limit > 1000)
    {
      LM_W(("Bad Input (too big value for /limit/ URI param: %d - max allowed is 1000)", orionldState.uriParams.limit));
      orionldErrorResponseCreate(OrionldBadRequestData, "Bad value for URI parameter /limit/ (valid range: 0-1000)", value);
      orionldState.httpStatusCode = 400;
      return MHD_YES;
    }

    orionldState.uriParams.mask |= ORIONLD_URIPARAM_LIMIT;
  }
  else if (strcmp(key, "options") == 0)
  {
    orionldState.uriParams.options = (char*) value;
    optionsParse(value);
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_OPTIONS;
  }
  else if (strcmp(key, "geometry") == 0)
  {
    orionldState.uriParams.geometry = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_GEOMETRY;
  }
  else if (strcmp(key, "coordinates") == 0)
  {
    orionldState.uriParams.coordinates = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_COORDINATES;
  }
  else if (strcmp(key, "coords") == 0)  // Only NGSIv1/v2
  {
    orionldState.uriParams.coordinates = (char*) value;
  }
  else if (strcmp(key, "georel") == 0)
  {
    orionldState.uriParams.georel = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_GEOREL;
  }
  else if (strcmp(key, "geoproperty") == 0)
  {
    orionldState.uriParams.geoproperty = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_GEOPROPERTY;
  }
  else if (strcmp(key, "geometryProperty") == 0)
  {
    orionldState.uriParams.geometryProperty = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_GEOMETRYPROPERTY;
  }
  else if (strcmp(key, "count") == 0)
  {
    if (strcmp(value, "true") == 0)
      orionldState.uriParams.count = true;
    else if (strcmp(value, "false") != 0)
    {
      LM_W(("Bad Input (invalid value for URI parameter 'count': %s)", value));
      orionldErrorResponseCreate(OrionldBadRequestData, "Bad value for URI parameter /count/", value);
      orionldState.httpStatusCode = 400;
      return MHD_YES;
    }

    orionldState.uriParams.mask |= ORIONLD_URIPARAM_COUNT;
  }
  else if (strcmp(key, "q") == 0)
  {
    orionldState.uriParams.q = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_Q;
  }
  else if (strcmp(key, "mq") == 0)
  {
    orionldState.uriParams.mq = (char*) value;
  }
  else if (strcmp(key, "datasetId") == 0)
  {
    char* detail;

    if (pcheckUri((char*) value, true, &detail) == false)
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Not a URI", value);  // FIXME: Include 'detail' and name (datasetId)
      orionldState.httpStatusCode = 400;
      return MHD_YES;
    }

    orionldState.uriParams.datasetId = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_DATASETID;
  }
  else if (strcmp(key, "deleteAll") == 0)
  {
    if (strcmp(value, "true") == 0)
      orionldState.uriParams.deleteAll = true;
    else if (strcmp(value, "false") == 0)
      orionldState.uriParams.deleteAll = false;
    else
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid value for uri parameter 'deleteAll'", value);
      orionldState.httpStatusCode = 400;
      return MHD_YES;
    }

    orionldState.uriParams.mask |= ORIONLD_URIPARAM_DELETEALL;
  }
  else if (strcmp(key, "timeproperty") == 0)
  {
    orionldState.uriParams.timeproperty = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_TIMEPROPERTY;
  }
  else if (strcmp(key, "timerel") == 0)
  {
    // FIXME: Check the value of timerel
    orionldState.uriParams.timerel = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_TIMEREL;
  }
  else if (strcmp(key, "timeAt") == 0)
  {
    // FIXME: Check the value
    orionldState.uriParams.timeAt = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_TIMEAT;
  }
  else if (strcmp(key, "endTimeAt") == 0)
  {
    // FIXME: Check the value
    orionldState.uriParams.endTimeAt = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_ENDTIMEAT;
  }
  else if (strcmp(key, "details") == 0)
  {
    if (strcmp(value, "true") == 0)
      orionldState.uriParams.details = true;
    else if (strcmp(value, "false") == 0)
      orionldState.uriParams.details = false;
    else if (strcmp(value, "on") == 0)
      orionldState.uriParams.details = true;
    else
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid value for uri parameter 'details'", value);
      orionldState.httpStatusCode = 400;
      return MHD_YES;
    }

    orionldState.uriParams.mask |= ORIONLD_URIPARAM_DETAILS;
  }
  else if (strcmp(key, "prettyPrint") == 0)
  {
    if (strcmp(value, "yes") == 0)
      orionldState.uriParams.prettyPrint = true;
    else if (strcmp(value, "no") == 0)
      orionldState.uriParams.prettyPrint = false;
    else
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid value for uri parameter 'prettyPrint'", value);
      orionldState.httpStatusCode = 400;
      return MHD_YES;
    }

    orionldState.uriParams.mask |= ORIONLD_URIPARAM_PRETTYPRINT;
  }
  else if (strcmp(key, "spaces") == 0)
  {
    orionldState.uriParams.spaces = atoi(value);
    orionldState.uriParams.mask  |= ORIONLD_URIPARAM_SPACES;
  }
  else if (strcmp(key, "subscriptionId") == 0)
  {
    orionldState.uriParams.subscriptionId  = (char*) value;
    orionldState.uriParams.mask           |= ORIONLD_URIPARAM_SUBSCRIPTION_ID;
  }
  else if (strcmp(key, "location") == 0)
  {
    if (strcmp(value, "true") == 0)
      orionldState.uriParams.location = true;
    else if (strcmp(value, "false") == 0)
      orionldState.uriParams.location = false;
    else
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid value for uri parameter 'location'", value);
      orionldState.httpStatusCode = 400;
      return MHD_YES;
    }

    orionldState.uriParams.mask |= ORIONLD_URIPARAM_LOCATION;
  }
  else if (strcmp(key, "url") == 0)
  {
    orionldState.uriParams.url   = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_URL;
  }
  else if (strcmp(key, "reload") == 0)
  {
    orionldState.uriParams.reload = true;
    orionldState.uriParams.mask  |= ORIONLD_URIPARAM_RELOAD;
  }
  else if (strcmp(key, "exist") == 0)
  {
    orionldState.uriParams.exists = (char*) value;
  }
  else if (strcmp(key, "!exist") == 0)
  {
    orionldState.uriParams.notExists = (char*) value;
    orionldState.uriParams.mask  |= ORIONLD_URIPARAM_NOTEXISTS;
  }
  else if (strcmp(key, "metadata") == 0)
  {
    orionldState.uriParams.metadata = (char*) value;
  }
  else if (strcmp(key, "orderBy") == 0)
  {
    orionldState.uriParams.orderBy = (char*) value;
  }
  else if (strcmp(key, "collapse") == 0)
  {
    if (strcmp(value, "true") == 0)
      orionldState.uriParams.collapse = true;
  }
  //
  // FIXME: attributeFormat AND attributesFormat ???
  //
  else if (strcmp(key, "attributeFormat") == 0)
  {
    orionldState.uriParams.attributeFormat = (char*) value;
  }
  else if (strcmp(key, "attributesFormat") == 0)
  {
    orionldState.uriParams.attributeFormat = (char*) value;
  }
  else if (strcmp(key, "reset") == 0)
  {
    if (strcmp(value, "true") == 0)
      orionldState.uriParams.reset = true;
  }
  else if (strcmp(key, "level") == 0)
  {
    orionldState.uriParams.level = (char*) value;
  }
  else if (strcmp(key, "entity::type") == 0)
  {
    orionldState.uriParams.type = (char*) value;
  }
  else
  {
    LM_W(("Bad Input (unknown URI parameter: '%s')", key));
    orionldState.httpStatusCode = 400;
    orionldErrorResponseCreate(OrionldBadRequestData, "Unknown URI parameter", key);
    return MHD_YES;
  }

  return MHD_YES;
}



// -----------------------------------------------------------------------------
//
// serviceLookup - lookup the Service
//
// orionldMhdConnectionInit guarantees that a valid verb is used. I.e. POST, GET, DELETE or PATCH
// orionldServiceLookup makes sure the URL supprts the verb
//
static OrionLdRestService* serviceLookup(ConnectionInfo* ciP)
{
  OrionLdRestService* serviceP;

  serviceP = orionldServiceLookup(&orionldRestServiceV[orionldState.verb]);
  if (serviceP == NULL)
  {
    if (orionldBadVerb(ciP) == true)
      orionldState.httpStatusCode = 405;  // SccBadVerb
    else
    {
      orionldErrorResponseCreate(OrionldResourceNotFound, "Service Not Found", orionldState.urlPath);
      orionldState.httpStatusCode = 404;
    }
  }

  return serviceP;
}



// -----------------------------------------------------------------------------
//
// tenantCheck -
//
static bool tenantCheck(char* tenantName, char* title, int titleLen, char* detail, int detailLen)
{
  int   len    = 0;

  while (tenantName[len] != 0)
  {
    if (len > SERVICE_NAME_MAX_LEN)
    {
      snprintf(title, titleLen, "Invalid Tenant");
      snprintf(detail, detailLen, "a tenant name can be max %d characters long", SERVICE_NAME_MAX_LEN);
      return false;
    }

    if ((!isalnum(tenantName[len])) && (tenantName[len] != '_'))
    {
      snprintf(title, titleLen, "Invalid Tenant");
      snprintf(detail, detailLen, "bad character in tenant name - only underscore and alphanumeric characters are allowed");
      return false;
    }

    ++len;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// orionldMhdConnectionInit -
//
MHD_Result orionldMhdConnectionInit
(
  MHD_Connection*  connection,
  const char*      url,
  const char*      method,
  const char*      version,
  void**           con_cls
)
{
  ++requestNo;

//  if (requestNo % 1000 == 0)
    LM_TMP(("------------------------- Servicing NGSI-LD request %03d: %s %s --------------------------", requestNo, method, url));  // if not REQUEST_PERFORMANCE

  //
  // 1. Prepare connectionInfo
  //
  ConnectionInfo* ciP = new ConnectionInfo();

  // Remember ciP for consequent connection callbacks from MHD
  *con_cls = ciP;

  //
  // 2. Prepare orionldState
  //
  orionldStateInit(connection);
  orionldState.ciP         = ciP;
  orionldState.httpVersion = (char*) version;

  LM_TMP(("KZ: orionldState.httpStatusCode == %d", orionldState.httpStatusCode));
  // IP Address and port of caller
  ipAddressAndPort();

  // By default, no whitespace in output
  orionldState.kjsonP->spacesPerIndent   = 0;
  orionldState.kjsonP->nlString          = (char*) "";
  orionldState.kjsonP->stringBeforeColon = (char*) "";
  orionldState.kjsonP->stringAfterColon  = (char*) "";

  // Flagging all as OK - errors will be flagged when occurring
  orionldState.httpStatusCode = 200;

  // Keep a pointer to the method/verb
  orionldState.verbString = (char*) method;

  // Save URL path in ConnectionInfo
  orionldState.urlPath = (char*) url;

  //
  // Does the URL path end in a '/'?
  // If so, remove it.
  // If more than one, ERROR
  //
  int urlLen = strlen(orionldState.urlPath);

  if (orionldState.urlPath[urlLen - 1] == '/')
  {
    orionldState.urlPath[urlLen - 1] = 0;
    urlLen -= 1;

    // Now check for a second '/'
    if (orionldState.urlPath[urlLen - 1] == '/')
    {
      orionldState.responsePayload = (char*) doubleSlashPayload;  // FIXME: use orionldErrorResponseCreate - after setting prettyPrint
      orionldState.httpStatusCode  = 400;
      return MHD_YES;
    }
  }

  LM_TMP(("KZ: orionldState.httpStatusCode == %d", orionldState.httpStatusCode));
  // 3. Check invalid verb
  orionldState.verbString = (char*) method;
  orionldState.verb       = verbGet(method);

  if (orionldState.verb == NOVERB)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "not a valid verb", method);  // FIXME: do this after setting prettyPrint
    orionldState.httpStatusCode   = 400;
    return MHD_YES;
  }

  LM_TMP(("KZ: orionldState.httpStatusCode == %d", orionldState.httpStatusCode));
  // 4. GET Service Pointer from VERB and URL-PATH
  orionldState.serviceP = serviceLookup(ciP);

  LM_TMP(("KZ: orionldState.httpStatusCode == %d", orionldState.httpStatusCode));
  if (orionldState.serviceP == NULL)  // 405 or 404 - no need to continue - prettyPrint not possible here
    return MHD_YES;

  //
  // 5. GET URI params
  //
  MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, orionldUriArgumentGet, NULL);

  //
  // Format of response payload
  //
  if (orionldState.uriParams.prettyPrint == true)
  {
    // Human readable output
    orionldState.kjsonP->spacesPerIndent   = orionldState.uriParams.spaces;
    orionldState.kjsonP->nlString          = (char*) "\n";
    orionldState.kjsonP->stringBeforeColon = (char*) "";
    orionldState.kjsonP->stringAfterColon  = (char*) " ";
  }


  //
  // NGSI-LD only accepts the verbs POST, GET, DELETE and PATCH
  // If any other verb is used, even if a valid REST Verb, a generic error will be returned
  //
  if ((orionldState.verb != POST) && (orionldState.verb != GET) && (orionldState.verb != DELETE) && (orionldState.verb != PATCH))
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Verb not supported by NGSI-LD", method);
    orionldState.httpStatusCode = 400;
  }

  LM_TMP(("KZ: orionldState.httpStatusCode == %d", orionldState.httpStatusCode));
  //
  // Check validity of URI parameters
  //
  if ((orionldState.uriParams.limit == 0) && (orionldState.uriParams.count == false))
  {
    LM_E(("Invalid value for URI parameter 'limit': 0"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid value for URI parameter /limit/", "must be an integer value >= 1, if /count/ is not set");
    orionldState.httpStatusCode = 400;
  }

  LM_TMP(("KZ: orionldState.httpStatusCode == %d", orionldState.httpStatusCode));
  if (orionldState.uriParams.limit > 1000)
  {
    LM_E(("Invalid value for URI parameter 'limit': %d", orionldState.uriParams.limit));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid value for URI parameter /limit/", "must be an integer value <= 1000");
    orionldState.httpStatusCode = 400;
  }

  LM_TMP(("KZ: orionldState.httpStatusCode == %d", orionldState.httpStatusCode));
  //
  // Get HTTP Headers
  // First we call the Orion-LD function 'orionldHttpHeaderGet' and then the Orion/NGSIv2 function 'httpHeaderGet'
  // Any header cannot be part of both functions.
  // The idea is to move all headers from httpHeaderGet to orionldHttpHeaderGet
  //
  LM_TMP(("KZ: orionldState.httpStatusCode == %d", orionldState.httpStatusCode));
  MHD_get_connection_values(connection, MHD_HEADER_KIND, orionldHttpHeaderGet, NULL);
  LM_TMP(("KZ: orionldState.httpStatusCode == %d", orionldState.httpStatusCode));
  MHD_get_connection_values(connection, MHD_HEADER_KIND, httpHeaderGet, ciP);
  LM_TMP(("KZ: orionldState.httpStatusCode == %d", orionldState.httpStatusCode));

  //
  // Any error detected during httpHeaderGet calls?
  //
  if (orionldState.httpStatusCode != 200)
    return MHD_YES;  // httpHeaderGet stes the error
  LM_TMP(("KZ: orionldState.httpStatusCode == %d", orionldState.httpStatusCode));

  if (orionldState.tenantP == NULL)
    orionldState.tenantP = &tenant0;
  else
  {
    char title[80];
    char detail[256];

  LM_TMP(("KZ: orionldState.httpStatusCode == %d", orionldState.httpStatusCode));
    if (tenantCheck(orionldState.tenantName, title, sizeof(title), detail, sizeof(detail)) == false)
    {
      LM_E(("Invalid value for tenant: '%s'", orionldState.tenantName));
      orionldErrorResponseCreate(OrionldBadRequestData, title, detail);
      orionldState.httpStatusCode = 400;
      return MHD_YES;
    }
  LM_TMP(("KZ: orionldState.httpStatusCode == %d", orionldState.httpStatusCode));
  }

  LM_TMP(("KZ: orionldState.httpStatusCode == %d", orionldState.httpStatusCode));
  if ((orionldState.ngsildContent == true) && (orionldState.linkHttpHeaderPresent == true))
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "invalid combination of HTTP headers Content-Type and Link", "Content-Type is 'application/ld+json' AND Link header is present - not allowed");
    orionldState.httpStatusCode  = 400;
    return MHD_YES;
  }

  LM_TMP(("KZ: orionldState.httpStatusCode == %d", orionldState.httpStatusCode));
  // Check payload too big
  if (ciP->httpHeaders.contentLength > 2000000)
  {
    orionldState.responsePayload = (char*) payloadTooLargePayload;
    orionldState.httpStatusCode  = 400;
    return MHD_YES;
  }

  LM_TMP(("KZ: orionldState.httpStatusCode == %d", orionldState.httpStatusCode));
  // Set servicePath: "/#" for GET requests, "/" for all others (ehmmm ... creation of subscriptions ...)
  ciP->servicePathV.push_back((orionldState.verb == GET)? "/#" : "/");
  LM_TMP(("KZ: orionldState.httpStatusCode == %d", orionldState.httpStatusCode));


  // Check that GET/DELETE has no payload
  // Check that POST/PUT/PATCH has payload
  // Check validity of tenant
  // Check Accept header
  // Check URL path is OK

  // Check Content-Type is accepted
  if ((orionldState.verb == PATCH) && (strcmp(ciP->httpHeaders.contentType.c_str(), "application/merge-patch+json") == 0))
    ciP->httpHeaders.contentType = "application/json";
  else if ((orionldState.verb == POST) || (orionldState.verb == PATCH))
  {
    //
    // FIXME: Instead of multiple strcmps, save an enum constant in ciP about content-type
    //
  LM_TMP(("KZ: orionldState.httpStatusCode == %d", orionldState.httpStatusCode));
    if ((strcmp(ciP->httpHeaders.contentType.c_str(), "application/json") != 0) && (strcmp(ciP->httpHeaders.contentType.c_str(), "application/ld+json") != 0))
    {
      LM_W(("Bad Input (invalid Content-Type: '%s'", ciP->httpHeaders.contentType.c_str()));
      orionldErrorResponseCreate(OrionldBadRequestData,
                                 "unsupported format of payload",
                                 "only application/json and application/ld+json are supported");
      orionldState.httpStatusCode = 415;  // Unsupported Media Type
      return MHD_YES;
    }
  LM_TMP(("KZ: orionldState.httpStatusCode == %d", orionldState.httpStatusCode));
  }

  LM_TMP(("KZ: orionldState.httpStatusCode == %d", orionldState.httpStatusCode));
  return MHD_YES;
}
