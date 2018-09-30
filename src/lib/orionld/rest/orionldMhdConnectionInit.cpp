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
#include <string.h>                                         // strlen
#include <microhttpd.h>                                     // MHD

extern "C"
{
#include "kjson/kjBufferCreate.h"                           // kjBufferCreate
}

#include "logMsg/logMsg.h"                                  // LM_*
#include "logMsg/traceLevels.h"                             // Lmt*

#include "rest/Verb.h"                                      // Verb
#include "rest/ConnectionInfo.h"                            // ConnectionInfo
#include "orionld/rest/temporaryErrorPayloads.h"            // Temporary Error Payloads
#include "orionld/common/orionldErrorResponse.h"            // OrionldBadRequestData, OrionldDetailsString, ...
#include "orionld/rest/orionldMhdConnectionInit.h"          // Own interface



// -----------------------------------------------------------------------------
//
// clientIp - from src/lib/rest.cpp
//
extern __thread char  clientIp[IP_LENGTH_MAX + 1];



// ----------------------------------------------------------------------------
//
// External declarations - tmp - should be in their own files and included here
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
    char c7 = method[6];

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
  char            ip[32];
  unsigned short  port = 0;

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
  static int reqNo = 0;

  ++reqNo;

  //
  // This call to LM_TMP should not be removed. Only commented out
  //
  LM_TMP(("------------------------- Servicing NGSI-LD request %03d: %s %s --------------------------", reqNo, method, url));

  //
  // 0. Prepare connectionInfo
  //
  ConnectionInfo* ciP = new ConnectionInfo();

  //
  // Mark connection as NGSI-LD V1  
  //
  ciP->apiVersion = NGSI_LD_V1;

  //
  // Remember ciP for consequent connection callbacks from MHD
  //
  *con_cls = ciP;

  //
  // Creating kjson environment for KJson parse and render
  //
  ciP->kjsonP = kjBufferCreate();      
  if (ciP->kjsonP == NULL)
    LM_X(1, ("Out of memory"));
  LM_TMP(("Allocated ciP->kjsonP at %p", ciP->kjsonP));


  //
  // The 'connection', as given by MHD is very important. No responses can be sent without it
  //
  ciP->connection = connection;

  //
  // Flagging all as OK - errors will be flagged when occurring
  //
  ciP->httpStatusCode = SccOk;


  //
  // IP Address and port of caller
  //
  ipAddressAndPort(ciP);

  // Save URL path in ConnectionInfo
  ciP->urlPath = (char*) url;

  //
  // Does the URL path end in a '/'?
  // If so, remove it.
  // If more than one, ERROR
  //
  int urlLen = strlen(ciP->urlPath);

  if (ciP->urlPath[urlLen - 1] == '/')
  {
    LM_T(LmtUriPath, ("URI PATH ends in SLASH - removing it"));
    ciP->urlPath[urlLen - 1] = 0;
    urlLen -= 1;

    // Now check for a second '/'
    if (ciP->urlPath[urlLen - 1] == '/')
    {
      LM_T(LmtUriPath, ("URI PATH ends in DOUBLE SLASH - flagging error"));
      ciP->responsePayload  = (char*) doubleSlashPayload;
      ciP->httpStatusCode   = SccBadRequest;
      return MHD_YES;
    }
  }

  // 3.  Check invalid verb
  ciP->verb = verbGet(method);
  if (ciP->verb == NOVERB)
  {
    LM_T(LmtVerb, ("NOVERB for (%s)", method));
    ciP->responsePayload  = (char*) invalidVerbPayload;
    ciP->httpStatusCode   = SccBadRequest;
    return MHD_YES;
  }

  // 4.  Check payload too big
  if (ciP->httpHeaders.contentLength > 2000000)
  {
    ciP->responsePayload  = (char*) payloadTooLargePayload;
    ciP->httpStatusCode   = SccBadRequest;
    return MHD_YES;
  }

  // 5.  Get HTTP Headers
  MHD_get_connection_values(connection, MHD_HEADER_KIND, httpHeaderGet, ciP);

  // 6. Set servicePath: "/#" for GET requests, "/" for all others (ehmmm ... creation of subscriptions ...)
  ciP->servicePathV.push_back((ciP->verb == GET)? "/#" : "/");

  
  // 7.  Check that GET/DELETE has no payload
  // 8.  Check that POST/PUT/PATCH has payload
  // 9.  Check validity of tenant
  // 10. Check Accept header
  // 11. Check URL path is OK
  // 12. Check Content-Type is accepted
  LM_T(LmtHttpHeaders, ("Content-Type: %s", ciP->httpHeaders.contentType.c_str()));
  LM_T(LmtHttpHeaders, ("Accepted: %s", ciP->httpHeaders.accept.c_str()));

  // 13. Get URI parameters
  MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, uriArgumentGet, ciP);

  // 14. Check ...

  // 20. Lookup the Service Routine
  // 21. Not found?  Look it up in the badVerb vector
  // 22. Not found still? Return error


  //
  // 23. Format of response payload
  //
  if (ciP->prettyPrint == true)
  {
    // Human readable output
    ciP->kjsonP->spacesPerIndent   = ciP->prettyPrintSpaces;
    ciP->kjsonP->nlString          = (char*) "\n";
    ciP->kjsonP->stringBeforeColon = (char*) "";
    ciP->kjsonP->stringAfterColon  = (char*) " ";
  }
  else
  {
    // By default, no whitespace in output
    ciP->kjsonP->spacesPerIndent   = 0;
    ciP->kjsonP->nlString          = (char*) "";
    ciP->kjsonP->stringBeforeColon = (char*) "";
    ciP->kjsonP->stringAfterColon  = (char*) "";
  }

  //
  // NGSI-LD only accepts the verbs POST, GET, DELETE and PATCH
  // If any other verb is used, even if a valid REST Verb, a generic error will be returned
  //
  if ((ciP->verb != POST) && (ciP->verb != GET) && (ciP->verb != DELETE) && (ciP->verb != PATCH))
  {
    LM_T(LmtVerb, ("The verb '%s' is not supported by NGSI-LD", method));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Verb not supported by NGSI-LD", method, OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
  }

  LM_T(LmtMhd, ("Connection Init DONE"));
  return MHD_YES;
}
