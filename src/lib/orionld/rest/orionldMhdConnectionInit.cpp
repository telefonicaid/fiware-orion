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
#include <string.h>                                            // strlen
#include <microhttpd.h>                                        // MHD

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/Verb.h"                                         // Verb
#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionld/common/orionldErrorResponse.h"               // OrionldBadRequestData, ...
#include "orionld/common/orionldState.h"                       // orionldState, orionldStateInit
#include "orionld/common/SCOMPARE.h"                           // SCOMPARE
#include "orionld/context/orionldContextListPresent.h"         // orionldContextListPresent
#include "orionld/rest/temporaryErrorPayloads.h"               // Temporary Error Payloads
#include "orionld/rest/orionldMhdConnectionInit.h"             // Own interface



// -----------------------------------------------------------------------------
//
// clientIp - from src/lib/rest.cpp
//
extern __thread char  clientIp[IP_LENGTH_MAX + 1];



// ----------------------------------------------------------------------------
//
// External declarations - tmp - should be in their own files (not rest.cpp) and included here
//
extern int httpHeaderGet(void* cbDataP, MHD_ValueKind kind, const char* ckey, const char* value);
extern int uriArgumentGet(void* cbDataP, MHD_ValueKind kind, const char* ckey, const char* val);



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
  char      ip[32];
  uint16_t  port = 0;

  const union MHD_ConnectionInfo* mciP = MHD_get_connection_info(ciP->connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);

  if (mciP != NULL)
  {
    struct sockaddr* addr = (struct sockaddr*) mciP->client_addr;

    port = (addr->sa_data[0] << 8) + addr->sa_data[1];
    snprintf(ip, sizeof(ip), "%d.%d.%d.%d",
             addr->sa_data[2] & 0xFF,
             addr->sa_data[3] & 0xFF,
             addr->sa_data[4] & 0xFF,
             addr->sa_data[5] & 0xFF);
    snprintf(clientIp, sizeof(clientIp), "%s", ip);
  }
  else
  {
    port = 0;
    snprintf(ip, sizeof(ip), "IP unknown");
  }

  ciP->port = port;
}



// -----------------------------------------------------------------------------
//
// orionldUriArgumentGet -
//
static int orionldUriArgumentGet(void* cbDataP, MHD_ValueKind kind, const char* key, const char* value)
{
  LM_TMP(("Got URI Param: '%s' == '%s'", key, value));

  if (SCOMPARE3(key, 'i', 'd', 0))
    orionldState.uriParams.id = (char*) value;
  else if (SCOMPARE5(key, 't', 'y', 'p', 'e', 0))
    orionldState.uriParams.type = (char*) value;
  else if (SCOMPARE10(key, 'i', 'd', 'P', 'a', 't', 't', 'e', 'r', 'n', 0))
    orionldState.uriParams.idPattern = (char*) value;
  else if (SCOMPARE6(key, 'a', 't', 't', 'r', 's', 0))
    orionldState.uriParams.attrs = (char*) value;

  return MHD_YES;
}



// -----------------------------------------------------------------------------
//
// uriArgumentsPresent - temp - FIXME: TO BE REMOVED
//
static void uriArgumentsPresent(void)
{
  LM_TMP(("orionldUriArguments: id:        '%s'", orionldState.uriParams.id));
  LM_TMP(("orionldUriArguments: type:      '%s'", orionldState.uriParams.type));
  LM_TMP(("orionldUriArguments: idPattern: '%s'", orionldState.uriParams.idPattern));
  LM_TMP(("orionldUriArguments: attrs:     '%s'", orionldState.uriParams.attrs));
}



// -----------------------------------------------------------------------------
//
// orionldMhdConnectionInit -
//
int orionldMhdConnectionInit
(
  MHD_Connection*  connection,
  const char*      url,
  const char*      method,
  const char*      version,
  void**           con_cls
)
{
  ++requestNo;

  //
  // This call to LM_TMP should not be removed. Only commented out
  //
  LM_TMP(("------------------------- Servicing NGSI-LD request %03d: %s %s --------------------------", requestNo, method, url));
  orionldContextListPresent();


  //
  // 1. Prepare connectionInfo
  //
  ConnectionInfo* ciP = new ConnectionInfo();

  // Mark connection as NGSI-LD V1
  ciP->apiVersion = NGSI_LD_V1;

  // Remember ciP for consequent connection callbacks from MHD
  *con_cls = ciP;

  //
  // 1. Prepare orionldState
  //
  orionldStateInit();
  orionldState.ciP = ciP;


  // The 'connection', as given by MHD is very important. No responses can be sent without it
  ciP->connection = connection;

  // Flagging all as OK - errors will be flagged when occurring
  ciP->httpStatusCode = SccOk;


  // IP Address and port of caller
  ipAddressAndPort(ciP);

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
    LM_T(LmtUriPath, ("URI PATH ends in SLASH - removing it"));
    orionldState.urlPath[urlLen - 1] = 0;
    urlLen -= 1;

    // Now check for a second '/'
    if (orionldState.urlPath[urlLen - 1] == '/')
    {
      LM_T(LmtUriPath, ("URI PATH ends in DOUBLE SLASH - flagging error"));
      orionldState.responsePayload = (char*) doubleSlashPayload;
      ciP->httpStatusCode          = SccBadRequest;
      return MHD_YES;
    }
  }

  // 3.  Check invalid verb
  ciP->verb = verbGet(method);
  if (ciP->verb == NOVERB)
  {
    LM_T(LmtVerb, ("NOVERB for (%s)", method));
    orionldErrorResponseCreate(OrionldBadRequestData, "not a valid verb", method);
    ciP->httpStatusCode   = SccBadRequest;
    return MHD_YES;
  }

  // 4.  Check payload too big
  if (ciP->httpHeaders.contentLength > 2000000)
  {
    orionldState.responsePayload = (char*) payloadTooLargePayload;
    ciP->httpStatusCode          = SccBadRequest;
    return MHD_YES;
  }

  // 5.  Get HTTP Headers
  MHD_get_connection_values(connection, MHD_HEADER_KIND, httpHeaderGet, ciP);  // FIXME: to be reimplemented in C !!!

  // 6. Set servicePath: "/#" for GET requests, "/" for all others (ehmmm ... creation of subscriptions ...)
  ciP->servicePathV.push_back((ciP->verb == GET)? "/#" : "/");


  // 7.  Check that GET/DELETE has no payload
  // 8.  Check that POST/PUT/PATCH has payload
  // 9.  Check validity of tenant
  // 10. Check Accept header
  // 11. Check URL path is OK

  // 12. Check Content-Type is accepted
  if ((ciP->verb == POST) || (ciP->verb == PATCH))
  {
    //
    // FIXME: Instead of multiple strcmps, save an enum constant in ciP about content-type
    //
    if ((strcmp(ciP->httpHeaders.contentType.c_str(), "application/json") != 0) && (strcmp(ciP->httpHeaders.contentType.c_str(), "application/ld+json") != 0))
    {
      orionldErrorResponseCreate(OrionldBadRequestData,
                                 "unsupported format of payload",
                                 "only application/json and application/ld+json are supported");
      ciP->httpStatusCode = SccUnsupportedMediaType;
      return MHD_YES;
    }
  }

  // 13. Get URI parameters
  MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, uriArgumentGet, ciP);           // FIXME: To Be Removed!
  MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, orionldUriArgumentGet, NULL);
  uriArgumentsPresent();

  // 14. Check ...

  // 20. Lookup the Service Routine
  // 21. Not found?  Look it up in the badVerb vector
  // 22. Not found still? Return error


  //
  // 23. Format of response payload
  //
  if (orionldState.prettyPrint == true)
  {
    // Human readable output
    orionldState.kjsonP->spacesPerIndent   = orionldState.prettyPrintSpaces;
    orionldState.kjsonP->nlString          = (char*) "\n";
    orionldState.kjsonP->stringBeforeColon = (char*) "";
    orionldState.kjsonP->stringAfterColon  = (char*) " ";
  }
  else
  {
    // By default, no whitespace in output
    orionldState.kjsonP->spacesPerIndent   = 0;
    orionldState.kjsonP->nlString          = (char*) "";
    orionldState.kjsonP->stringBeforeColon = (char*) "";
    orionldState.kjsonP->stringAfterColon  = (char*) "";
  }

  //
  // NGSI-LD only accepts the verbs POST, GET, DELETE and PATCH
  // If any other verb is used, even if a valid REST Verb, a generic error will be returned
  //
  if ((ciP->verb != POST) && (ciP->verb != GET) && (ciP->verb != DELETE) && (ciP->verb != PATCH))
  {
    LM_T(LmtVerb, ("The verb '%s' is not supported by NGSI-LD", method));
    orionldErrorResponseCreate(OrionldBadRequestData, "Verb not supported by NGSI-LD", method);
    ciP->httpStatusCode = SccBadRequest;
  }

  LM_T(LmtMhd, ("Connection Init DONE"));
  return MHD_YES;
}
