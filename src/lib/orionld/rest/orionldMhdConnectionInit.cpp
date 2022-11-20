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

#include "common/wsStrip.h"                                      // wsStrip
#include "common/MimeType.h"                                     // MimeType
#include "common/string.h"                                       // toLowercase
#include "common/globals.h"                                      // parse8601Time
#include "alarmMgr/alarmMgr.h"                                   // alarmMgr
#include "rest/Verb.h"                                           // Verb
#include "rest/OrionError.h"                                     // OrionError
#include "parse/forbiddenChars.h"                                // forbiddenChars

#include "orionld/common/orionldState.h"                         // orionldState, orionldStateInit
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/performance.h"                          // REQUEST_PERFORMANCE
#include "orionld/common/tenantList.h"                           // tenant0
#include "orionld/common/mimeTypeFromString.h"                   // mimeTypeFromString
#include "orionld/common/orionldTenantLookup.h"                  // orionldTenantLookup
#include "orionld/serviceRoutines/orionldBadVerb.h"              // orionldBadVerb
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri
#include "orionld/rest/orionldServiceInit.h"                     // orionldRestServiceV
#include "orionld/rest/orionldServiceLookup.h"                   // orionldServiceLookup
#include "orionld/rest/OrionLdRestService.h"                     // ORIONLD_URIPARAM_LIMIT, ...
#include "orionld/rest/orionldMhdConnectionInit.h"               // Own interface



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
      else if (strcmp(optionStart, "simplified")    == 0)  orionldState.uriParamOptions.keyValues     = true;
      else if (strcmp(optionStart, "concise")       == 0)  orionldState.uriParamOptions.concise       = true;
      else if (strcmp(optionStart, "normalized")    == 0)  orionldState.uriParamOptions.normalized    = true;
      else if (strcmp(optionStart, "sysAttrs")      == 0)  orionldState.uriParamOptions.sysAttrs      = true;
      else if (strcmp(optionStart, "fromDb")        == 0)  orionldState.uriParamOptions.fromDb        = true;
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
        LM_W(("Invalid 'options' value: %s", optionStart));
        orionldError(OrionldBadRequestData, "Invalid value for URI param /options/", optionStart, 400);
        return;
      }

      if (done == true)
        break;

      *cP = saved;
      optionStart = &cP[1];
    }

    ++cP;
  }

  if (orionldState.uriParamOptions.keyValues && orionldState.uriParamOptions.concise && orionldState.uriParamOptions.normalized)
    orionldError(OrionldBadRequestData, "Incoherent value for /options/ URI param", "All three output formats (simplified, concise, and normalized) are set", 400);
  else if (orionldState.uriParamOptions.keyValues && orionldState.uriParamOptions.concise)
    orionldError(OrionldBadRequestData, "Incoherent value for /options/ URI param", "Both /simplified/ and /concise/ output formats are set", 400);
  else if (orionldState.uriParamOptions.keyValues && orionldState.uriParamOptions.normalized)
    orionldError(OrionldBadRequestData, "Incoherent value for /options/ URI param", "Both /simplified/ and /normalized/ output formats are set", 400);
  else if (orionldState.uriParamOptions.concise && orionldState.uriParamOptions.normalized)
    orionldError(OrionldBadRequestData, "Incoherent value for /options/ URI param", "Both /concise/ and /normalized/ output formats are set", 400);
  else if (orionldState.uriParamOptions.keyValues && orionldState.uriParamOptions.sysAttrs)
    orionldError(OrionldBadRequestData, "Incoherent value for /options/ URI param", "Can't have system attributes when /simplified/ output format is selected", 400);
  else if (orionldState.uriParamOptions.keyValues)
    orionldState.out.format = RF_KEYVALUES;
  else if (orionldState.uriParamOptions.concise)
    orionldState.out.format = RF_CONCISE;
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




// -----------------------------------------------------------------------------
//
// acceptHeaderParse -
//
// Wildcards:
//   Accept: <MIME_type>/<MIME_subtype>
//   Accept: <MIME_type>/*
//   Accept: */*
//
// Example with weights:
//   Accept: text/html, application/xhtml+xml, application/xml;q=0.9, image/webp, */*;q=0.8
//
//
MimeType acceptHeaderParse(char* accept, bool textOk)
{
  // Split MimeTypes according to commas
  char* mimeV[20];
  int   mimes = 1;

  // Step over any initial WS
  while (*accept == ' ')
  {
    if (*accept == 0)
      return NOMIMETYPE;

    ++accept;
  }

  mimeV[0] = accept;

  while (*accept != 0)
  {
    if (*accept == ',')
    {
      *accept = 0;
      mimeV[mimes++] = &accept[1];
    }

    ++accept;
  }

  //
  MimeType winner        = NOMIMETYPE;
  float    winningWeight = 0;

  for (int ix = 0; ix < mimes; ix++)
  {
    MimeType  mimeType;
    float     weight = 1;  // Default weight is 1 - q=xxx can modify that

    // Extract weight, if there
    char* q = strstr(mimeV[ix], ";q=");
    if (q != NULL)
    {
      *q     = 0;
      weight = atof(&q[3]);
    }

    //
    // GET the MIME type, check for new winner
    // REMEMBER:  JSON is the default Mime Type and if equal weight, JSON wins
    //
    mimeType                 = mimeTypeFromString(mimeV[ix], NULL, true, textOk, &orionldState.acceptMask);
    orionldState.acceptMask |= (1 << mimeType);  // It's OK to include "NOMIMETYPE"

    if (mimeType > NOMIMETYPE)
    {
      if (winner == NOMIMETYPE)
      {
        winner        = mimeType;
        winningWeight = weight;
      }
      else if (weight > winningWeight)
      {
        winner        = mimeType;
        winningWeight = weight;
      }
      else if ((weight == winningWeight) && (mimeType == JSON) && (orionldState.apiVersion == NGSI_LD_V1))
      {
        winner        = mimeType;
        winningWeight = weight;
      }
    }
  }

  return winner;
}



// -----------------------------------------------------------------------------
//
// pCheckTenantName -
//
bool pCheckTenantName(const char* dbName)
{
  size_t len = strlen(dbName);

  if (len > 52)
  {
    orionldError(OrionldBadRequestData, "Invalid tenant name", "Tenant name too long (maximum is set to 52 characters)", 400);
    return false;
  }

  if (strcspn(dbName, "/. \"\\$") != len)
  {
    orionldError(OrionldBadRequestData, "Invalid tenant name", "Invalid character for a mongo database name", 400);
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// orionldHttpHeaderReceive -
//
static MHD_Result orionldHttpHeaderReceive(void* cbDataP, MHD_ValueKind kind, const char* key, const char* value)
{
  if (strcasecmp(key, "Orionld-Legacy") == 0)
  {
    if (mongocOnly == false)
      orionldState.in.legacy = (char*) value;
  }
  else if (strcasecmp(key, "Performance") == 0)
    orionldState.in.performance = true;
  else if (strcasecmp(key, "NGSILD-Scope") == 0)
  {
    orionldState.scopes = strSplit((char*) value, ',', orionldState.scopeV, K_VEC_SIZE(orionldState.scopeV));
    if (orionldState.scopes == -1)
    {
      LM_W(("Bad Input (too many scopes)"));
      orionldError(OrionldBadRequestData, "Bad value for HTTP header /NGSILD-Scope/", value, 400);
    }
  }
  else if (strcasecmp(key, "Accept") == 0)
  {
    orionldState.out.contentType = acceptHeaderParse((char*) value, false);

    if (orionldState.out.contentType == NOMIMETYPE)
    {
      const char* details = "HTTP Header /Accept/ contains none of 'application/json', 'application/ld+json', or 'application/geo+json'";

      LM_W(("Bad Input (HTTP Header /Accept/ none of 'application/json', 'application/ld+json', or 'application/geo+json')"));
      orionldError(OrionldBadRequestData, "Invalid Accept mime-type", details, 406);
    }
  }
  else if (strcasecmp(key, "Ngsiv2-AttrsFormat") == 0) orionldState.attrsFormat         = (char*) value;
  else if (strcasecmp(key, "X-Auth-Token")       == 0) orionldState.in.xAuthToken       = (char*) value;
  else if (strcasecmp(key, "Authorization")      == 0) orionldState.in.authorization    = (char*) value;
  else if (strcasecmp(key, "Fiware-Correlator")  == 0) orionldState.correlator          = (char*) value;
  else if (strcasecmp(key, "Content-Length")     == 0) orionldState.in.contentLength    = atoi(value);
  else if (strcasecmp(key, "Prefer")             == 0) orionldState.preferHeader        = (char*) value;
  else if (strcasecmp(key, "Origin")             == 0) orionldState.in.origin           = (char*) value;
  else if (strcasecmp(key, "Host")               == 0) orionldState.in.host             = (char*) value;
  else if (strcasecmp(key, "X-Real-IP")          == 0) orionldState.in.xRealIp          = (char*) value;
  else if (strcasecmp(key, "Connection")         == 0) orionldState.in.connection       = (char*) value;
  else if (strcasecmp(key, "X-Forwarded-For")    == 0) orionldState.in.xForwardedFor    = (char*) value;
  else if (strcasecmp(key, "Content-Type")       == 0)
  {
    orionldState.in.contentType       = mimeTypeFromString(value, NULL, false, false, &orionldState.acceptMask);
    orionldState.in.contentTypeString = (char*) value;
  }
  else if (strcasecmp(key, "Link") == 0)
  {
    orionldState.link                  = (char*) value;
    orionldState.linkHttpHeaderPresent = true;
  }
  else if ((strcasecmp(key, "Fiware-Service") == 0) || (strcasecmp(key, "NGSILD-Tenant") == 0))
  {
    if (multitenancy == true)  // Has the broker been started with multi-tenancy enabled (it's disabled by default)
    {
      if (pCheckTenantName(value) == false)
        return MHD_YES;

      toLowercase((char*) value);  // All tenants are lowercase - mongo decides that

      // Tenant already given?
      if (orionldState.tenantName != NULL)
      {
        if (strcmp(orionldState.tenantName, value) != 0)
        {
          orionldError(OrionldBadRequestData, "HTTP header duplication", "Tenant set twice (or perhaps both Fiware-Service and NGSILD-Tenant?)", 400);
          return MHD_YES;
        }
      }
      else
      {
        orionldState.tenantName = (char*) value;  // FIXME: Deprecate this field
        orionldState.in.tenant  = (char*) value;
        orionldState.tenantP    = orionldTenantLookup(orionldState.tenantName);
      }
    }
    else
    {
      // Tenant used when tenant is not supported by the broker - silently ignored for NGSIv2/v2, error for NGSI-LD
      orionldError(OrionldBadRequestData, "Tenants not supported", "tenant in use but tenant support is not enabled for the broker", 400);
    }
  }

  return MHD_YES;
}



// -----------------------------------------------------------------------------
//
// orionldUriArgumentGet -
//
MHD_Result orionldUriArgumentGet(void* cbDataP, MHD_ValueKind kind, const char* key, const char* value)
{
  // NULL/empty URI param value
  if ((value == NULL) || (*value == 0))
  {
    orionldError(OrionldBadRequestData, "Empty right-hand-side for a URI parameter", key, 400);
    return MHD_YES;
  }

  //
  // Forbidden characters in URI param value - not for NGSI-LD - for now at least ...
  //
  if (orionldState.apiVersion != NGSI_LD_V1)
  {
    bool containsForbiddenChars = false;

    if ((strcmp(key, "geometry") == 0) || (strcmp(key, "georel") == 0))
      containsForbiddenChars = forbiddenChars(value, "=;");
    else if (strcmp(key, "coords") == 0)
      containsForbiddenChars = forbiddenChars(value, ";");
    else if ((strcmp(key, "q") != 0) && (strcmp(key, "mq") != 0) && (strcmp(key, "idPattern") != 0) && (strcmp(key, "typePattern") != 0))
      containsForbiddenChars = forbiddenChars(key) || forbiddenChars(value);

    if (containsForbiddenChars == true)
    {
      OrionError  error(SccBadRequest, "invalid character in a URI parameter");
      char        details[256];

      snprintf(details, sizeof(details), "invalid character in URI param '%s'", key);
      orionldError(OrionldBadRequestData, "invalid character in a URI parameter", key, 400);

      alarmMgr.badInput(clientIp, details);

      LM_W(("Bad Input (forbidden character in URI parameter value: %s=%s)", key, value));
      return MHD_YES;
    }
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
      orionldError(OrionldBadRequestData, "Bad value for URI parameter /offset/", value, 400);
      return MHD_YES;
    }

    if (strspn(value, "0123456789") != strlen(value))
    {
      orionldError(OrionldBadRequestData, "Invalid value for URI parameter /offset/", "must be an integer value >= 0", 400);
      return MHD_YES;
    }

    orionldState.uriParams.offset = atoi(value);
    orionldState.uriParams.mask  |= ORIONLD_URIPARAM_OFFSET;
  }
  else if (strcmp(key, "limit") == 0)
  {
    if (value[0] == '-')
    {
      orionldError(OrionldBadRequestData, "Bad value for URI parameter /limit/", value, 400);
      return MHD_YES;
    }

    if (strspn(value, "0123456789") != strlen(value))
    {
      orionldError(OrionldBadRequestData, "Invalid value for URI parameter /limit/", "must be an integer value >= 1", 400);
      return MHD_YES;
    }

    orionldState.uriParams.limit = atoi(value);
    if (orionldState.uriParams.limit > 1000)
    {
      orionldError(OrionldBadRequestData, "Bad value for URI parameter /limit/ (valid range: 0-1000)", value, 400);
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
      orionldError(OrionldBadRequestData, "Bad value for URI parameter /count/", value, 400);
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
    if (pCheckUri((char*) value, "datasetId", true) == false)
      return MHD_YES;

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
      orionldError(OrionldBadRequestData, "Invalid value for uri parameter /deleteAll/", value, 400);
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
      orionldError(OrionldBadRequestData, "Invalid value for uri parameter /details/", value, 400);
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
      orionldError(OrionldBadRequestData, "Invalid value for uri parameter /prettyPrint/", value, 400);
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
      orionldError(OrionldBadRequestData, "Invalid value for uri parameter /location/", value, 400);
      return MHD_YES;
    }

    orionldState.uriParams.mask |= ORIONLD_URIPARAM_LOCATION;
  }
  else if (strcmp(key, "url") == 0)
  {
    orionldState.uriParams.url   = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_URL;
  }
  else if (strcmp(key, "observedAt") == 0)
  {
    orionldState.uriParams.observedAtAsDouble = parse8601Time((char*) value);

    if (orionldState.uriParams.observedAtAsDouble == -1)
    {
      orionldError(OrionldBadRequestData, "Invalid value for uri parameter /observedAt/ (not a valid ISO8601 timestamp)", value, 400);
      return MHD_YES;
    }
    else
    {
      orionldState.uriParams.observedAt  = (char*) value;
      orionldState.uriParams.mask       |= ORIONLD_URIPARAM_OBSERVEDAT;
    }
  }
  else if (strcmp(key, "lang") == 0)
  {
    orionldState.uriParams.lang        = (char*) value;
    orionldState.uriParams.mask       |= ORIONLD_URIPARAM_LANG;
  }
  else if (strcmp(key, "reload") == 0)
  {
    orionldState.uriParams.reload = true;
    orionldState.uriParams.mask  |= ORIONLD_URIPARAM_RELOAD;
  }
  else if (strcmp(key, "exist") == 0)
  {
    orionldState.uriParams.exists = (char*) value;

    if (strcmp(value, "entity::type") == 0)
      orionldState.in.entityTypeExists = true;
  }
  else if (strcmp(key, "!exist") == 0)
  {
    orionldState.uriParams.notExists = (char*) value;
    orionldState.uriParams.mask  |= ORIONLD_URIPARAM_NOTEXISTS;

    if (strcmp(value, "entity::type") == 0)
      orionldState.in.entityTypeDoesNotExist = true;
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
  // NOTE: Seems like both "attributeFormat" AND "attributesFormat" need to be supported
  //
  else if ((strcmp(key, "attributeFormat") == 0) || (strcmp(key, "attributesFormat") == 0))
  {
    orionldState.uriParams.attributeFormat = (char*) value;
    if (strcmp(value, "object") == 0)
      orionldState.in.attributeFormatAsObject = true;
  }
  else if (strcmp(key, "relationships") == 0)
  {
    orionldState.uriParams.relationships   = (char*) value;
    orionldState.uriParams.mask           |= ORIONLD_URIPARAM_RELATIONSHIPS;
  }
  else if (strcmp(key, "geoproperties") == 0)
  {
    orionldState.uriParams.geoproperties   = (char*) value;
    orionldState.uriParams.mask           |= ORIONLD_URIPARAM_GEOPROPERTIES;
  }
  else if (strcmp(key, "languageproperties") == 0)
  {
    orionldState.uriParams.languageproperties  = (char*) value;
    orionldState.uriParams.mask               |= ORIONLD_URIPARAM_LANGUAGEPROPERTIES;
  }
  else if (strcmp(key, "reset") == 0)
  {
    if (strcmp(value, "true") == 0)
      orionldState.uriParams.reset = true;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_RESET;
  }
  else if (strcmp(key, "level") == 0)
  {
    orionldState.uriParams.level = (char*) value;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_LEVEL;
  }
  else if (strcmp(key, "local") == 0)
  {
    if (strcmp(value, "true") == 0)
      orionldState.uriParams.local = true;
    orionldState.uriParams.mask |= ORIONLD_URIPARAM_LOCAL;
  }
  else if (strcmp(key, "entity::type") == 0)  // Is NGSIv1 ?entity::type=X the same as NGSIv2 ?type=X ?
  {
    orionldState.uriParams.type = (char*) value;
  }
  else
  {
    orionldError(OrionldBadRequestData, "Unknown URI parameter", key, 400);
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
static OrionLdRestService* serviceLookup(void)
{
  OrionLdRestService* serviceP;

  serviceP = orionldServiceLookup(&orionldRestServiceV[orionldState.verb]);
  if (serviceP == NULL)
  {
    if (orionldBadVerb() == true)
      orionldState.httpStatusCode = 405;  // SccBadVerb
    else
      orionldError(OrionldResourceNotFound, "Service Not Found", orionldState.urlPath, 404);
  }

  return serviceP;
}



// -----------------------------------------------------------------------------
//
// tenantCheck -
//
static bool tenantCheck(char* tenantName)
{
  int len = 0;

  while (tenantName[len] != 0)
  {
    if (len > SERVICE_NAME_MAX_LEN)
    {
      orionldError(OrionldBadRequestData, "Invalid Tenant", "a tenant name can be max 50 characters long", 400);
      return false;
    }

    if ((!isalnum(tenantName[len])) && (tenantName[len] != '_'))
    {
      orionldError(OrionldBadRequestData, "Invalid Tenant", "bad character in tenant name - only underscore and alphanumeric characters are allowed", 400);
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

  // if ((requestNo % 100 == 0) || (requestNo == 1))
  LM(("------------------------- Servicing NGSI-LD request %03d: %s %s --------------------------", requestNo, method, url));  // if not REQUEST_PERFORMANCE

  //
  // 2. Prepare orionldState
  //
  orionldStateInit(connection);
  orionldState.apiVersion  = NGSI_LD_V1;
  orionldState.httpVersion = (char*) version;

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
      orionldError(OrionldBadRequestData, "Invalid URL PATH", "Double Slash", 400);
      return MHD_YES;
    }
  }

  // 2. NGSI-LD requests don't support the broker to be started with -noCache
  if (noCache == true)
  {
    orionldError(OrionldBadRequestData, "Not Implemented", "Running without Subscription Cache is not implemented for NGSI-LD requests", 501);
    return MHD_YES;
  }

  // 3. Check invalid verb
  orionldState.verbString = (char*) method;
  orionldState.verb       = verbGet(method);

  if (orionldState.verb == NOVERB)
  {
    orionldError(OrionldBadRequestData, "not a valid verb", method, 400);  // FIXME: do this after setting prettyPri, 400nt
    return MHD_YES;
  }

  // 4. GET Service Pointer from VERB and URL-PATH
  orionldState.serviceP = serviceLookup();

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
  // Is only mongoc supporting operations allowed?
  //
  if ((mongocOnly) && ((orionldState.serviceP->options & ORIONLD_SERVICE_OPTION_MONGOC_SUPPORT) == 0) && (orionldState.verb != OPTIONS))
  {
    orionldError(OrionldOperationNotSupported, "Not Implemented", "this request does not support the new mongoc driver", 501);
    return MHD_YES;
  }

  //
  // NGSI-LD only accepts the verbs POST, GET, DELETE, PATCH, PUT, and OPTIONS (if CORS is enabled)
  // If any other verb is used, even if a valid REST Verb, like HEAD, a generic error will be returned
  //
  if ((orionldState.verb != POST)    &&
      (orionldState.verb != GET)     &&
      (orionldState.verb != DELETE)  &&
      (orionldState.verb != PATCH)   &&
      (orionldState.verb != PUT)     &&
      (orionldState.verb != OPTIONS))
  {
    orionldError(OrionldBadRequestData, "Verb not supported by NGSI-LD", method, 400);
  }

  //
  // Check validity of URI parameters
  //
  if ((orionldState.uriParams.limit == 0) && (orionldState.uriParams.count == false))
    orionldError(OrionldBadRequestData, "Invalid value for URI parameter /limit/", "must be an integer value >= 1, if /count/ is not set", 400);

  if (orionldState.uriParams.limit > 1000)
    orionldError(OrionldBadRequestData, "Invalid value for URI parameter /limit/", "must be an integer value <= 1000", 400);

  //
  // Get HTTP Headers
  //
  MHD_get_connection_values(connection, MHD_HEADER_KIND, orionldHttpHeaderReceive, NULL);

  if (orionldState.httpStatusCode != 200)
  {
    LM_W(("Error detected in a HTTP header: %s: %s", orionldState.pd.title, orionldState.pd.detail));
    return MHD_YES;  // orionldHttpHeaderReceive sets the error
  }

  if (orionldState.tenantP == NULL)
    orionldState.tenantP = &tenant0;
  else if (tenantCheck(orionldState.tenantName) == false)  // tenantCheck calls orionldError
    return MHD_YES;

  if ((orionldState.in.contentType == JSONLD) && (orionldState.linkHttpHeaderPresent == true))
  {
    orionldError(OrionldBadRequestData, "invalid combination of HTTP headers Content-Type and Link", "Content-Type is 'application/ld+json' AND Link header is present - not allowed", 400);
    return MHD_YES;
  }

  // Check payload too big
  if (orionldState.in.contentLength > 2000000)
  {
    orionldError(OrionldBadRequestData, "Invalid Payload", "Payload too large", 400);
    return MHD_YES;
  }

  // Check that GET/DELETE has no payload
  // Check that POST/PUT/PATCH has payload
  // Check validity of tenant
  // Check Accept header
  // Check URL path is OK

  // Check Content-Type is accepted
  if ((orionldState.verb == PATCH) && (orionldState.in.contentType == MERGEPATCHJSON))
    orionldState.in.contentType = JSON;
  else if ((orionldState.verb == POST) || (orionldState.verb == PATCH))
  {
    if ((orionldState.in.contentType != JSON) && (orionldState.in.contentType != JSONLD))
    {
      LM_W(("Bad Input (invalid Content-Type: '%s')", orionldState.in.contentTypeString));
      orionldError(OrionldBadRequestData, "unsupported format of payload", "only application/json and application/ld+json are supported", 415);
      return MHD_YES;
    }
  }

  return MHD_YES;
}
