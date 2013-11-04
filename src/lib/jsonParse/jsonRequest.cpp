/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <exception>
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi/Request.h"
#include "ngsi/ParseData.h"
#include "jsonParse/JsonNode.h"
#include "jsonParse/jsonParse.h"
#include "jsonParse/jsonRequest.h"
#include "jsonParse/jsonRegisterContextRequest.h"
#include "jsonParse/jsonDiscoverContextAvailabilityRequest.h"
#include "jsonParse/jsonSubscribeContextAvailabilityRequest.h"
#include "jsonParse/jsonQueryContextRequest.h"
#include "jsonParse/jsonUpdateContextRequest.h"
#include "jsonParse/jsonSubscribeContextRequest.h"
#include "jsonParse/jsonUnsubscribeContextRequest.h"
#include "jsonParse/jsonNotifyContextRequest.h"
#include "rest/restReply.h"



/* ****************************************************************************
*
* jsonRequest - 
*/
static JsonRequest jsonRequest[] = 
{
  // NGSI9
  { RegisterContext,                       "POST", "registerContextRequest",                        jsonRcrParseVector,  jsonRcrInit,  jsonRcrCheck,   jsonRcrPresent,  jsonRcrRelease  },
  { DiscoverContextAvailability,           "POST", "discoverContextAvailabilityRequest",            jsonDcarParseVector, jsonDcarInit, jsonDcarCheck,  jsonDcarPresent, jsonDcarRelease },
  { SubscribeContextAvailability,          "POST", "subscribeContextAvailabilityRequest",           jsonScarParseVector, jsonScarInit, jsonScarCheck,  jsonScarPresent, jsonScarRelease },
  { UnsubscribeContextAvailability,        "POST", "unsubscribeContextAvailabilityRequest",         NULL, NULL, NULL, NULL, NULL },
  { UpdateContextAvailabilitySubscription, "POST", "updateContextAvailabilitySubscriptionRequest",  NULL, NULL, NULL, NULL, NULL },

  // NGSI10
  { QueryContext,                          "POST", "queryContextRequest",                           jsonQcrParseVector,  jsonQcrInit,  jsonQcrCheck,   jsonQcrPresent,  jsonQcrRelease  },
  { UpdateContext,                         "POST", "updateContextRequest",                          jsonUpcrParseVector, jsonUpcrInit, jsonUpcrCheck,  jsonUpcrPresent, jsonUpcrRelease },
  { SubscribeContext,                      "POST", "subscribeContextRequest",                       jsonScrParseVector,  jsonScrInit,  jsonScrCheck,   jsonScrPresent,  jsonScrRelease  },
  { NotifyContext,                         "POST", "notifyContextRequest",                          jsonNcrParseVector,  jsonNcrInit,  jsonNcrCheck,   jsonNcrPresent,  jsonNcrRelease  },
  
  { UpdateContextSubscription,             "POST", "updateContextSubscriptionRequest",              NULL, NULL, NULL, NULL, NULL },
  { UnsubscribeContext,                    "POST", "unsubscribeContextRequest",                     jsonUncrParseVector, jsonUncrInit, jsonUncrCheck,  jsonUncrPresent, jsonUncrRelease }
};



/* ****************************************************************************
*
* jsonRequestGet - 
*/
static JsonRequest* jsonRequestGet(RequestType requestType)
{
  for (unsigned int ix = 0; ix < sizeof(jsonRequest) / sizeof(jsonRequest[0]); ++ix)
  {
    if (requestType == jsonRequest[ix].type)
      return &jsonRequest[ix];
  }

  return NULL;
}


/* ****************************************************************************
*
* jsonTreat - 
*/
std::string jsonTreat(const char* content, ConnectionInfo* ciP, ParseData* parseDataP, RequestType request, std::string payloadWord, JsonRequest** reqPP)
{
  std::string   res   = "OK";
  JsonRequest*  reqP  = jsonRequestGet(request);

  LM_T(LmtParse, ("Treating a JSON request: '%s'", content));

  if (reqP == NULL)
  {
    std::string errorReply = restErrorReplyGet(ciP, ciP->outFormat, "", requestType(request), SccBadRequest, "no request treating object found", std::string("Sorry, no request treating object found for RequestType '") + requestType(request) + "'");
    LM_RE(errorReply, ("Sorry, no request treating object found for RequestType %d (%s)", request, requestType(request)));
  }

  if (reqP->parseVector == NULL)
  {
     std::string errorReply = restErrorReplyGet(ciP, ciP->outFormat, "", requestType(request), SccReceiverInternalError, "not implemented", "not implemented");
     LM_RE(errorReply, ("Sorry, not implemented"));
  }

  LM_T(LmtParse, ("Treating '%s' request", reqP->keyword.c_str()));

  reqP->init(parseDataP);

  try
  {
    jsonParse(content, reqP->keyword, reqP->parseVector, parseDataP);
  }
  catch (std::exception &e)
  {
    res = std::string("JSON parse error: ") + e.what();
    std::string errorReply  = restErrorReplyGet(ciP, ciP->outFormat, "", reqP->keyword, SccReceiverInternalError, "Parse Error", res);
    LM_E(("JSON Parse Error: '%s'", e.what()));
    LM_RE(errorReply, (res.c_str()));
  }

  reqP->present(parseDataP);

  res = reqP->check(parseDataP, ciP);
  reqP->present(parseDataP);

  if (reqPP != NULL)
    *reqPP = reqP;

  return res;
}
