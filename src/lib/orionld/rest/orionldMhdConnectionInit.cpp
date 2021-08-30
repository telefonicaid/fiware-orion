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
#include "kbase/kMacros.h"                                       // K_FT
#include "kbase/kTime.h"                                         // kTimeGet, kTimeDiff
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/Verb.h"                                           // Verb
#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "orionld/common/orionldErrorResponse.h"                 // OrionldBadRequestData, ...
#include "orionld/common/orionldState.h"                         // orionldState, orionldStateInit
#include "orionld/common/SCOMPARE.h"                             // SCOMPARE
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
// clientIp - from src/lib/rest.cpp
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
static Verb verbGet(const char* method)
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
static void ipAddressAndPort(ConnectionInfo* ciP)
{
  const union MHD_ConnectionInfo* mciP = MHD_get_connection_info(ciP->connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);

  if (mciP != NULL)
  {
    struct sockaddr* addr = (struct sockaddr*) mciP->client_addr;

    ciP->port = (addr->sa_data[0] << 8) + addr->sa_data[1];
    snprintf(clientIp, sizeof(clientIp), "%d.%d.%d.%d",
             addr->sa_data[2] & 0xFF,
             addr->sa_data[3] & 0xFF,
             addr->sa_data[4] & 0xFF,
             addr->sa_data[5] & 0xFF);
  }
  else
  {
    ciP->port = 0;
    strncpy(clientIp, "IP unknown", sizeof(clientIp) - 1);
  }
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

      if      (strcmp(optionStart, "update")      == 0)  orionldState.uriParamOptions.update      = true;
      else if (strcmp(optionStart, "replace")     == 0)  orionldState.uriParamOptions.replace     = true;
      else if (strcmp(optionStart, "noOverwrite") == 0)  orionldState.uriParamOptions.noOverwrite = true;
      else if (strcmp(optionStart, "keyValues")   == 0)  orionldState.uriParamOptions.keyValues   = true;
      else if (strcmp(optionStart, "sysAttrs")    == 0)  orionldState.uriParamOptions.sysAttrs    = true;
      else if (strcmp(optionStart, "count")       == 0)  orionldState.uriParams.count             = true;  // NGSIv2 compatibility
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


#if 0
// -----------------------------------------------------------------------------
//
// orionldHttpHeaderGet -
//
static int orionldHttpHeaderGet(void* cbDataP, MHD_ValueKind kind, const char* key, const char* value)
{
  if (strcmp(key, "NGSILD-Tenant") == 0)
    orionldState.httpHeaders.tenant = value;
}
#endif



// -----------------------------------------------------------------------------
//
// orionldUriArgumentGet -
//
static MHD_Result orionldUriArgumentGet(void* cbDataP, MHD_ValueKind kind, const char* key, const char* value)
{
  if (SCOMPARE3(key, 'i', 'd', 0))
  {
    orionldState.uriParams.id = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_IDLIST;
  }
  else if (SCOMPARE5(key, 't', 'y', 'p', 'e', 0))
  {
    orionldState.uriParams.type = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_TYPELIST;
  }
  else if (SCOMPARE10(key, 'i', 'd', 'P', 'a', 't', 't', 'e', 'r', 'n', 0))
  {
    orionldState.uriParams.idPattern = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_IDPATTERN;
  }
  else if (SCOMPARE6(key, 'a', 't', 't', 'r', 's', 0))
  {
    orionldState.uriParams.attrs = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_ATTRS;
  }
  else if (SCOMPARE7(key, 'o', 'f', 'f', 's', 'e', 't', 0))
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
  else if (SCOMPARE6(key, 'l', 'i', 'm', 'i', 't', 0))
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
  else if (SCOMPARE8(key, 'o', 'p', 't', 'i', 'o', 'n', 's', 0))
  {
    orionldState.uriParams.options = (char*) value;
    optionsParse(value);
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_OPTIONS;
  }
  else if (SCOMPARE9(key, 'g', 'e', 'o', 'm', 'e', 't', 'r', 'y', 0))
  {
    orionldState.uriParams.geometry = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_GEOMETRY;
  }
  else if (SCOMPARE12(key, 'c', 'o', 'o', 'r', 'd', 'i', 'n', 'a', 't', 'e', 's', 0))
  {
    orionldState.uriParams.coordinates = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_COORDINATES;
  }
  else if (SCOMPARE7(key, 'g', 'e', 'o', 'r', 'e', 'l', 0))
  {
    orionldState.uriParams.georel = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_GEOREL;
  }
  else if (SCOMPARE12(key, 'g', 'e', 'o', 'p', 'r', 'o', 'p', 'e', 'r', 't', 'y', 0))
  {
    orionldState.uriParams.geoproperty = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_GEOPROPERTY;
  }
  else if (SCOMPARE17(key, 'g', 'e', 'o', 'm', 'e', 't', 'r', 'y', 'P', 'r', 'o', 'p', 'e', 'r', 't', 'y', 0))
  {
    orionldState.uriParams.geometryProperty = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_GEOMETRYPROPERTY;
  }
  else if (SCOMPARE6(key, 'c', 'o', 'u', 'n', 't', 0))
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
  else if (SCOMPARE2(key, 'q', 0))
  {
    orionldState.uriParams.q = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_Q;
  }
  else if (SCOMPARE10(key, 'd', 'a', 't', 'a', 's', 'e', 't', 'I', 'd', 0))
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
  else if (SCOMPARE10(key, 'd', 'e', 'l', 'e', 't', 'e', 'A', 'l', 'l', 0))
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
  else if (SCOMPARE13(key, 't', 'i', 'm', 'e', 'p', 'r', 'o', 'p', 'e', 'r', 't', 'y', 0))
  {
    orionldState.uriParams.timeproperty = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_TIMEPROPERTY;
  }
  else if (SCOMPARE8(key, 't', 'i', 'm', 'e', 'r', 'e', 'l', 0))
  {
    // FIXME: Check the value of timerel
    orionldState.uriParams.timerel = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_TIMEREL;
  }
  else if (SCOMPARE7(key, 't', 'i', 'm', 'e', 'A', 't', 0))
  {
    // FIXME: Check the value
    orionldState.uriParams.timeAt = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_TIMEAT;
  }
  else if (SCOMPARE10(key, 'e', 'n', 'd', 'T', 'i', 'm', 'e', 'A', 't', 0))
  {
    // FIXME: Check the value
    orionldState.uriParams.endTimeAt = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_ENDTIMEAT;
  }
  else if (SCOMPARE8(key, 'd', 'e', 't', 'a', 'i', 'l', 's', 0))
  {
    if (strcmp(value, "true") == 0)
      orionldState.uriParams.details = true;
    else if (strcmp(value, "false") == 0)
      orionldState.uriParams.details = false;
    else
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid value for uri parameter 'details'", value);
      orionldState.httpStatusCode = 400;
      return MHD_YES;
    }

    orionldState.uriParams.mask |= ORIONLD_URIPARAM_DETAILS;
  }
  else if (SCOMPARE12(key, 'p', 'r', 'e', 't', 't', 'y', 'P', 'r', 'i', 'n', 't', 0))
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
  else if (SCOMPARE7(key, 's', 'p', 'a', 'c', 'e', 's', 0))
  {
    orionldState.uriParams.spaces = atoi(value);
    orionldState.uriParams.mask  |= ORIONLD_URIPARAM_SPACES;
  }
  else if (SCOMPARE15(key, 's', 'u', 'b', 's', 'c', 'r', 'i', 'p', 't', 'i', 'o', 'n', 'I', 'd', 0))
  {
    orionldState.uriParams.subscriptionId  = (char*) value;
    orionldState.uriParams.mask           |= ORIONLD_URIPARAM_SUBSCRIPTION_ID;
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

  if (requestNo % 1000 == 0)
    LM_TMP(("------------------------- Servicing NGSI-LD request %03d: %s %s --------------------------", requestNo, method, url));  // if not REQUEST_PERFORMANCE

  //
  // 1. Prepare connectionInfo
  //
  ConnectionInfo* ciP = new ConnectionInfo();

  // Mark connection as NGSI-LD V1
  ciP->apiVersion = NGSI_LD_V1;

  // Remember ciP for consequent connection callbacks from MHD
  *con_cls = ciP;

  // The 'connection', as given by MHD is very important. No responses can be sent without it
  ciP->connection = connection;


  // IP Address and port of caller
  ipAddressAndPort(ciP);

  //
  // 2. Prepare orionldState
  //
  orionldStateInit();
  orionldState.ciP = ciP;


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

  // 3. Check invalid verb
  orionldState.verb = verbGet(method);
  ciP->verb = orionldState.verb;  // FIXME: to be removed
  if (orionldState.verb == NOVERB)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "not a valid verb", method);  // FIXME: do this after setting prettyPrint
    orionldState.httpStatusCode   = 400;
    return MHD_YES;
  }

  // 4. GET Service Pointer from VERB and URL-PATH
  orionldState.serviceP = serviceLookup(ciP);

  if (orionldState.serviceP == NULL)  // 405 or 404 - no need to continue - prettyPrint not possible here
    return MHD_YES;

  //
  // 5. GET URI params
  //    Those Service Routines that DON'T USE mongoBackend don't need to call uriArgumentGet
  //    [ mongoBackend needs stuff in ciP->uriParams ]
  //
  if ((orionldState.serviceP->options & ORIONLD_SERVICE_OPTION_NO_V2_URI_PARAMS) == 0)
    MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, uriArgumentGet, ciP);

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

  //
  // Check validity of URI parameters
  //
  if ((orionldState.uriParams.limit == 0) && (orionldState.uriParams.count == false))
  {
    LM_E(("Invalid value for URI parameter 'limit': 0"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid value for URI parameter /limit/", "must be an integer value >= 1, if /count/ is not set");
    orionldState.httpStatusCode = 400;
  }

  if (orionldState.uriParams.limit > 1000)
  {
    LM_E(("Invalid value for URI parameter 'limit': %d", orionldState.uriParams.limit));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid value for URI parameter /limit/", "must be an integer value <= 1000");
    orionldState.httpStatusCode = 400;
  }

  // Get HTTP Headers
  MHD_get_connection_values(connection, MHD_HEADER_KIND, httpHeaderGet, ciP);  // FIXME: implement orionldHttpHeaderGet in C !!!

  //
  // Any error detected during httpHeaderGet calls?
  //
  if (orionldState.httpStatusCode != 200)
    return MHD_YES;  // httpHeaderGet stes the error

  if (orionldState.tenantP == NULL)
    orionldState.tenantP = &tenant0;
  else
  {
    char title[80];
    char detail[256];

    if (tenantCheck(orionldState.tenantName, title, sizeof(title), detail, sizeof(detail)) == false)
    {
      LM_E(("Invalid value for tenant: '%s'", orionldState.tenantName));
      orionldErrorResponseCreate(OrionldBadRequestData, title, detail);
      orionldState.httpStatusCode = 400;
      return MHD_YES;
    }
  }

  if ((orionldState.ngsildContent == true) && (orionldState.linkHttpHeaderPresent == true))
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "invalid combination of HTTP headers Content-Type and Link", "Content-Type is 'application/ld+json' AND Link header is present - not allowed");
    orionldState.httpStatusCode  = 400;
    return MHD_YES;
  }

  // Check payload too big
  if (ciP->httpHeaders.contentLength > 2000000)
  {
    orionldState.responsePayload = (char*) payloadTooLargePayload;
    orionldState.httpStatusCode  = 400;
    return MHD_YES;
  }

  // Set servicePath: "/#" for GET requests, "/" for all others (ehmmm ... creation of subscriptions ...)
  ciP->servicePathV.push_back((orionldState.verb == GET)? "/#" : "/");


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
    if ((strcmp(ciP->httpHeaders.contentType.c_str(), "application/json") != 0) && (strcmp(ciP->httpHeaders.contentType.c_str(), "application/ld+json") != 0))
    {
      LM_W(("Bad Input (invalid Content-Type: '%s'", ciP->httpHeaders.contentType.c_str()));
      orionldErrorResponseCreate(OrionldBadRequestData,
                                 "unsupported format of payload",
                                 "only application/json and application/ld+json are supported");
      orionldState.httpStatusCode = 415;  // Unsupported Media Type
      return MHD_YES;
    }
  }

  return MHD_YES;
}
