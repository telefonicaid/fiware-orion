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
#include <microhttpd.h>

#include "logMsg/logMsg.h"

#include "rest/ConnectionInfo.h"
#include "rest/restReply.h"

#include "orionld/serviceRoutines/orionldPostEntities.h"
#include "orionld/serviceRoutines/orionldPostEntity.h"
#include "orionld/serviceRoutines/orionldPostSubscriptions.h"
#include "orionld/serviceRoutines/orionldPostRegistrations.h"
#include "orionld/serviceRoutines/orionldGetEntity.h"
#include "orionld/serviceRoutines/orionldGetEntities.h"
#include "orionld/serviceRoutines/orionldGetSubscriptions.h"
#include "orionld/serviceRoutines/orionldGetSubscription.h"
#include "orionld/serviceRoutines/orionldGetRegistrations.h"
#include "orionld/serviceRoutines/orionldGetRegistration.h"
#include "orionld/serviceRoutines/orionldPatchEntity.h"
#include "orionld/serviceRoutines/orionldPatchSubscription.h"
#include "orionld/serviceRoutines/orionldPatchRegistration.h"

#include "orionld/serviceRoutines/orionldDeleteEntity.h"
#include "orionld/serviceRoutines/orionldDeleteAttribute.h"
#include "orionld/serviceRoutines/orionldDeleteSubscription.h"
#include "orionld/serviceRoutines/orionldDeleteRegistration.h"

#include "orionld/rest/orionldServiceLookup.h"
#include "orionld/rest/RestServiceLd.h"
#include "orionld/rest/orionldMhdConnection.h"



// ----------------------------------------------------------------------------
//
// External declarations - tmp - should be in their own files and included here
//
extern int httpHeaderGet(void* cbDataP, MHD_ValueKind kind, const char* ckey, const char* value);
extern int uriArgumentGet(void* cbDataP, MHD_ValueKind kind, const char* ckey, const char* val);

extern __thread char  static_buffer[STATIC_BUFFER_SIZE + 1];
extern __thread char  clientIp[IP_LENGTH_MAX + 1];



/* ****************************************************************************
*
* connectionInfo - as a thread variable
*
* This to avoid thousands of mallocs/constructor calls every second in a busy broker.
* Unfortunately this doesn't work as long as ConnectionInfo is a ""
*
* thread_local ConnectionInfo connectionInfo = {};
*/
ConnectionInfo* connectionInfoP = NULL;



/* ****************************************************************************
*
* getServices -
*/
RestServiceLd getServices[] =
{
  { "/ngsi-ld/v1/entities/*",          orionldGetEntity          },
  { "/ngsi-ld/v1/entities",            orionldGetEntities        },
  { "/ngsi-ld/v1/subscriptions",       orionldGetSubscriptions   },
  { "/ngsi-ld/v1/subscriptions/*",     orionldGetSubscription    },
  { "/ngsi-ld/v1/registrations",       orionldGetRegistrations   },
  { "/ngsi-ld/v1/registrations/*",     orionldGetRegistration    }
};



/* ****************************************************************************
*
* postServices -
*/
RestServiceLd postServices[] =
{
  { "/ngsi-ld/v1/entities",            orionldPostEntities       },
  { "/ngsi-ld/v1/entities/*/attrs",    orionldPostEntity         },
  { "/ngsi-ld/v1/subscriptions",       orionldPostSubscriptions  },
  { "/ngsi-ld/v1/registrations",       orionldPostRegistrations  }
};



/* ****************************************************************************
*
* patchServices -
*/
RestServiceLd patchServices[] =
{
  { "/ngsi-ld/v1/entities/*/attrs",    orionldPatchEntity        },
  { "/ngsi-ld/v1/subscriptions/*",     orionldPatchSubscription  },
  { "/ngsi-ld/v1/registrations/*",     orionldPatchRegistration  }
};



/* ****************************************************************************
*
* deleteServices -
*/
RestServiceLd deleteServices[] =
{
  { "/ngsi-ld/v1/entities/*/attrs/*",  orionldDeleteAttribute    },  // Very important that orionldDeleteAttribute comes before orionldDeleteEntity
  { "/ngsi-ld/v1/entities/*",          orionldDeleteEntity       },
  { "/ngsi-ld/v1/subscriptions/*",     orionldDeleteSubscription },
  { "/ngsi-ld/v1/registrations/*",     orionldDeleteRegistration }
};



// -----------------------------------------------------------------------------
//
// RestServiceVector -
//
typedef struct RestServiceVector
{
  RestServiceLd* serviceV;
  int            services;
} RestServiceVector;



// -----------------------------------------------------------------------------
//
// restServiceVV -
//
RestServiceVector restServiceVV[] =
{
  { getServices,    6 },
  { NULL,           0 },
  { postServices,   4 },
  { deleteServices, 4 },
  { patchServices,  3 },
  { NULL,           0 },
  { NULL,           0 },
  { NULL,           0 },
  { NULL,           0 },
  { NULL,           0 }
};
  


OrionLdRestServiceVector orionLdRestServiceV[9];



// -----------------------------------------------------------------------------
//
// orionLdServiceInitPresent -
//
void orionLdServiceInitPresent(void)
{
  for (int svIx = 0; svIx < 9; svIx++)
  {
    OrionLdRestServiceVector* serviceV = &orionLdRestServiceV[svIx];
    
    printf("%d REST Services for %s\n", serviceV->services, verbName((Verb) svIx));

    if (serviceV->services == 0)
      continue;

    for (int sIx = 0; sIx < serviceV->services; sIx++)
    {
      OrionLdRestService* serviceP = &serviceV->serviceV[sIx];

      printf("  %s %s\n", verbName((Verb) svIx), serviceP->url);
      printf("  Service routine at:           %p\n", serviceP->serviceRoutine);
      printf("  Wildcards:                    %d\n", serviceP->wildcards);
      printf("  charsBeforeFirstWildcard:     %d\n", serviceP->charsBeforeFirstWildcard);
      printf("  charsBeforeFirstWildcardSum:  %d\n", serviceP->charsBeforeFirstWildcardSum);
      printf("  charsBeforeSecondWildcard:    %d\n", serviceP->charsBeforeSecondWildcard);
      printf("  charsBeforeSecondWildcardSum: %d\n", serviceP->charsBeforeSecondWildcardSum);
      printf("  matchForSecondWildcard:       %s\n", serviceP->matchForSecondWildcard);
      printf("  matchForSecondWildcardLen:    %d\n", serviceP->matchForSecondWildcardLen);
      printf("  supportedVerbMask:            0x%x\n", serviceP->supportedVerbMask);
      printf("\n");
    }
  }
}



// -----------------------------------------------------------------------------
//
// restServicePrepare -
//
void restServicePrepare(OrionLdRestService* serviceP, RestServiceLd* simpleServiceP)
{
  // 1. Simply copy/reference the two fields of RestServiceLd
  serviceP->url             = (char*) simpleServiceP->url;
  serviceP->serviceRoutine  = simpleServiceP->serviceRoutine;

  // 2. Loop over the URL Path, count wildcards, charsBeforeFirstWildcard, etc
  int    ix            = 11;    // strlen("/ngsi-ld/v1/") - 1
  char*  wildCardStart = NULL;
  char*  wildCardEnd   = NULL;

  while (serviceP->url[++ix] != 0)
  {
    char c = serviceP->url[ix];

    if (c == '*')
    {
      if (serviceP->wildcards == 0)
        wildCardStart = &serviceP->url[ix + 1];
      else if (serviceP->wildcards == 1)
        wildCardEnd = &serviceP->url[ix];
        
      LM_TMP(("Found a wildcard in index %d of '%s'", ix, serviceP->url));
      serviceP->wildcards += 1;
      continue;
    }

    if (serviceP->wildcards == 0)
    {
      ++serviceP->charsBeforeFirstWildcard;
      serviceP->charsBeforeFirstWildcardSum += c;
    }
    else if (serviceP->wildcards == 1)
    {
      ++serviceP->charsBeforeSecondWildcard;
      serviceP->charsBeforeSecondWildcardSum += c;
    }
  }

  if (serviceP->wildcards != 0)
  {
    if (wildCardEnd == NULL)  // If only one '*', make wildCardEnd point to end of URL
      wildCardEnd = &serviceP->url[ix];

    serviceP->matchForSecondWildcardLen = wildCardEnd - wildCardStart;

    if (serviceP->matchForSecondWildcardLen < 0)
      LM_X(1, ("Negative length of matchForSecondWildcard - not possible. SW bug"));
    if (serviceP->matchForSecondWildcardLen > (int) sizeof(serviceP->matchForSecondWildcard))
      LM_X(1, ("Too big matchForSecondWildcard - not possible. SW bug"));

    if (serviceP->matchForSecondWildcardLen != 0)
    {
      strncpy(serviceP->matchForSecondWildcard, wildCardStart, wildCardEnd - wildCardStart);
      LM_TMP(("matchForSecondWildcard: %s", serviceP->matchForSecondWildcard));
    }
  }
}



// -----------------------------------------------------------------------------
//
// orionLdServiceInit -
//
// This function converts the RestServiceLd vectors to OrionLdRestService vectors
//
void orionLdServiceInit(void)
{
  unsigned int svIx;  // Service Vector Index

  bzero(orionLdRestServiceV, sizeof(orionLdRestServiceV));

  for (svIx = 0; svIx < sizeof(restServiceVV) / sizeof(restServiceVV[0]); svIx++)
  {
    // svIx is really the Verb (GET=0, POST=2, up to NOVERB=9. See src/lib/rest/Verb.h)

    LM_TMP(("svIx: %d", svIx));
    if (restServiceVV[svIx].serviceV == NULL)
      continue;

    int services = restServiceVV[svIx].services;
    
    orionLdRestServiceV[svIx].serviceV  = (OrionLdRestService*) calloc(sizeof(OrionLdRestService), services);
    orionLdRestServiceV[svIx].services  = services;

    int sIx;  // Service Index inside Rest Service vector

    for (sIx = 0; sIx < services; sIx++)
    {
      LM_TMP(("sIx: %d", sIx));
      restServicePrepare(&orionLdRestServiceV[svIx].serviceV[sIx], &restServiceVV[svIx].serviceV[sIx]);
    }
  }
}



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
    char c7 = method[6];

    if ((c0 == 'O') && (c1 == 'P') && (c2 == 'T') && (c3 == 'I') && (c4 == 'O') && (c5 == 'N') && (c6 == 'S') && (c7 == 0))
      return OPTIONS;
  }

  return NOVERB;
}



/* ****************************************************************************
*
* Predefined payloads
*/
const char* invalidVerbPayload     = "{\"error\":\"Invalid Verb\"}";
const char* payloadTooLargePayload = "{\"error\":\"Payload too large\"}";
const char* genericErrorPayload    = "{\"error\":\"Generic Error\"}";
const char* doubleSlashPayload     = "{\"error\":\"Double Slash in URL PATH\"}";
const char* notFoundPayload        = "{\"error\":\"Service not found\"}";



/* ****************************************************************************
*
* orionldMhdConnectionInit -
*/
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
  
  LM_TMP(("------------------------- Servicing NGSI-LD request %03d: %s %s --------------------------", reqNo, method, url));

  //
  // 0. Prepare connectionInfo
  //
  ConnectionInfo* ciP = new ConnectionInfo();

  //
  // Remember ciP for consequent connection callbacks from MHD
  //
  *con_cls = ciP;

  //
  // The 'connection', as given by MHD is very important. No responses can be sent without it
  //
  ciP->connection = connection;

  //
  // Flagging all as OK - errors will be flagged when occurring
  //
  ciP->httpStatusCode = SccOk;


  //
  // 01. IP Address and port of caller
  //
  char            ip[32];
  unsigned short  port = 0;

  const union MHD_ConnectionInfo* mciP = MHD_get_connection_info(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);

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


  // 2. Save URL path in ConnectionInfo
  ciP->urlPath = (char*) url;

  //
  // Does the URL path end in a '/'?
  // If so, remove it.
  // If more than one, ERROR
  //
  int urlLen = strlen(ciP->urlPath);

  if (ciP->urlPath[urlLen - 1] == '/')
  {
    LM_TMP(("URI PATH ends in SLASH - removing it"));
    ciP->urlPath[urlLen - 1] = 0;
    urlLen -= 1;

    // Now check for a second '/'
    if (ciP->urlPath[urlLen - 1] == '/')
    {
      LM_TMP(("URI PATH ends in DOUBLE SLASH - flagging error"));
      ciP->responsePayload  = (char*) doubleSlashPayload;
      ciP->httpStatusCode   = SccBadRequest;
      return MHD_YES;
    }
  }

  // 3.  Check invalid verb
  ciP->verb = verbGet(method);
  if (ciP->verb == NOVERB)
  {
    LM_TMP(("NOVERB for (%s)", method));
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

  // 4.  Get HTTP Headers
  MHD_get_connection_values(connection, MHD_HEADER_KIND, httpHeaderGet, ciP);

  // 5.  Check that GET/DELETE has no payload
  // 6.  Check that POST/PUT/PATCH has payload
  // 7.  Check validity of tenant
  // 8.  Check Accept header
  // 9.  Check URL path is OK
  // 10. Check Content-Type is accepted
  
  // 11. Get URI parameters
  MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, uriArgumentGet, ciP);

  // 12. Check ...

  // 20. Lookup the Service Routine
  // 21. Not found?  Look it up in the badVerb vector
  // 22. Not found still? Return error
  // 23. Done - next step is to read the payload
  
  LM_TMP(("Connection Init DONE"));
  return MHD_YES;
}



/* ****************************************************************************
*
* orionldMhdConnectionPayloadRead -
*/
int orionldMhdConnectionPayloadRead
(
  ConnectionInfo*  ciP,
  size_t*          upload_data_size,
  const char*      upload_data
)
{
  size_t  dataLen = *upload_data_size;

  LM_TMP(("Reading %d bytes of payload", dataLen));

  //
  // If the HTTP header says the request is bigger than our PAYLOAD_MAX_SIZE,
  // just silently "eat" the entire message.
  //
  // The problem occurs when the broker is lied to and there aren't ciP->httpHeaders.contentLength
  // bytes to read.
  // When this happens, MHD blocks until it times out (MHD_OPTION_CONNECTION_TIMEOUT defaults to 5 seconds),
  // and the broker isn't able to respond. MHD just closes the connection.
  // Question asked in mhd mailing list.
  //
  // See github issue:
  //   https://github.com/telefonicaid/fiware-orion/issues/2761
  //
  if (ciP->httpHeaders.contentLength > PAYLOAD_MAX_SIZE)
  {
    //
    // Errors can't be returned yet, postpone ...
    //
    *upload_data_size = 0;
    return MHD_YES;
  }

  //
  // First call with payload - use the thread variable "static_buffer" if possible,
  // otherwise allocate a bigger buffer
  //
  // FIXME P1: This could be done in "Part I" instead, saving an "if" for each "Part II" call
  //           Once we *really* look to scratch some efficiency, this change should be made.
  //
  if (ciP->payloadSize == 0)  // First call with payload
  {
    if (ciP->httpHeaders.contentLength > STATIC_BUFFER_SIZE)
    {
      ciP->payload = (char*) malloc(ciP->httpHeaders.contentLength + 1);
    }
    else
    {
      ciP->payload = static_buffer;
    }
  }

  // Copy the chunk
  memcpy(&ciP->payload[ciP->payloadSize], upload_data, dataLen);

  // Add to the size of the accumulated read buffer
  ciP->payloadSize += dataLen;

  // Zero-terminate the payload
  ciP->payload[ciP->payloadSize] = 0;

  // Acknowledge the data and return
  *upload_data_size = 0;

  LM_TMP(("Got payload '%s'", ciP->payload));

  return MHD_YES;
}



// -----------------------------------------------------------------------------
//
// badVerb -
//
static void badVerb(ConnectionInfo* ciP)
{
  LM_TMP(("Inplement badVerb!!!!!"));

  ciP->responsePayload = (char*) notFoundPayload;
  ciP->httpStatusCode  = SccContextElementNotFound;
}



// -----------------------------------------------------------------------------
//
// orionldMhdConnectionTreat -
//
int orionldMhdConnectionTreat(ConnectionInfo* ciP)
{
  LM_TMP(("Read all the payload - treating the request!"));

  // If no error predetected, lookup the service and call it
  if (ciP->httpStatusCode == SccOk)
  {
    if ((ciP->verb == POST) || (ciP->verb == GET) || (ciP->verb == DELETE) || (ciP->verb == PATCH))
      ciP->serviceP = orionldServiceLookup(ciP, &orionLdRestServiceV[ciP->verb]);

    if (ciP->serviceP != NULL)
    {
      LM_TMP(("Calling the service routine"));
      ciP->serviceP->serviceRoutine(ciP);
    }
    else
    {
      badVerb(ciP);
    }
  }


#if 0
  // Is there a KJSON response tree to render?
  if (ciP->jsonResponseTree != NULL)
    ciP->responsePayload = kjsonRender(ciP->jsonResponseTree);
#endif

  LM_TMP(("Responding"));
  ciP->outMimeType = JSON;  // ALL responses have payload
  
  if (ciP->responsePayload != NULL)
  {
    LM_TMP(("Responding with '%s'", ciP->responsePayload));
    restReply(ciP, ciP->responsePayload);
  }
  else
  {
    restReply(ciP, genericErrorPayload);
  }

  LM_TMP(("Read all the payload"));

  return MHD_YES;
}
