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
#include "jsonParse/jsonNotifyContextAvailabilityRequest.h"
#include "jsonParse/jsonUnsubscribeContextAvailabilityRequest.h"
#include "jsonParse/jsonUpdateContextAvailabilitySubscriptionRequest.h"

#include "jsonParse/jsonQueryContextRequest.h"
#include "jsonParse/jsonUpdateContextRequest.h"
#include "jsonParse/jsonSubscribeContextRequest.h"
#include "jsonParse/jsonUnsubscribeContextRequest.h"
#include "jsonParse/jsonNotifyContextRequest.h"
#include "jsonParse/jsonUpdateContextSubscriptionRequest.h"

#include "jsonParse/jsonRegisterProviderRequest.h"
#include "jsonParse/jsonUpdateContextElementRequest.h"
#include "jsonParse/jsonAppendContextElementRequest.h"
#include "jsonParse/jsonUpdateContextAttributeRequest.h"

#include "parse/compoundValue.h"
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
  { UnsubscribeContextAvailability,        "POST", "unsubscribeContextAvailabilityRequest",         jsonUcarParseVector, jsonUcarInit, jsonUcarCheck,  jsonUcarPresent, jsonUcarRelease },
  { NotifyContextAvailability,             "POST", "notifyContextRequestAvailability",              jsonNcarParseVector, jsonNcarInit, jsonNcarCheck,  jsonNcarPresent, jsonNcarRelease },
  { UpdateContextAvailabilitySubscription, "POST", "updateContextAvailabilitySubscriptionRequest",  jsonUcasParseVector, jsonUcasInit, jsonUcasCheck,  jsonUcasPresent, jsonUcasRelease },

  // NGSI10
  { QueryContext,                          "POST", "queryContextRequest",                           jsonQcrParseVector,  jsonQcrInit,  jsonQcrCheck,   jsonQcrPresent,  jsonQcrRelease  },
  { UpdateContext,                         "POST", "updateContextRequest",                          jsonUpcrParseVector, jsonUpcrInit, jsonUpcrCheck,  jsonUpcrPresent, jsonUpcrRelease },
  { SubscribeContext,                      "POST", "subscribeContextRequest",                       jsonScrParseVector,  jsonScrInit,  jsonScrCheck,   jsonScrPresent,  jsonScrRelease  },
  { NotifyContext,                         "POST", "notifyContextRequest",                          jsonNcrParseVector,  jsonNcrInit,  jsonNcrCheck,   jsonNcrPresent,  jsonNcrRelease  },
  { UnsubscribeContext,                    "POST", "unsubscribeContextRequest",                     jsonUncrParseVector, jsonUncrInit, jsonUncrCheck,  jsonUncrPresent, jsonUncrRelease },
  { UpdateContextSubscription,             "POST", "updateContextSubscriptionRequest",              jsonUcsrParseVector, jsonUcsrInit, jsonUcsrCheck,  jsonUcsrPresent, jsonUcsrRelease },

  // Convenience
  { ContextEntityAttributes,               "POST", "registerProviderRequest",                       jsonRprParseVector,  jsonRprInit,  jsonRprCheck,   jsonRprPresent,  jsonRprRelease  },
  { ContextEntityAttributes,               "*",    "registerProviderRequest",                       jsonRprParseVector,  jsonRprInit,  jsonRprCheck,   jsonRprPresent,  jsonRprRelease  },
  { ContextEntityTypeAttributeContainer,   "POST", "registerProviderRequest",                       jsonRprParseVector,  jsonRprInit,  jsonRprCheck,   jsonRprPresent,  jsonRprRelease  },
  { ContextEntityTypeAttributeContainer,   "*",    "registerProviderRequest",                       jsonRprParseVector,  jsonRprInit,  jsonRprCheck,   jsonRprPresent,  jsonRprRelease  },
  { ContextEntitiesByEntityId,             "POST", "registerProviderRequest",                       jsonRprParseVector,  jsonRprInit,  jsonRprCheck,   jsonRprPresent,  jsonRprRelease  },
  { ContextEntitiesByEntityId,             "*",    "registerProviderRequest",                       jsonRprParseVector,  jsonRprInit,  jsonRprCheck,   jsonRprPresent,  jsonRprRelease  },
  { EntityByIdAttributeByName,             "POST", "registerProviderRequest",                       jsonRprParseVector,  jsonRprInit,  jsonRprCheck,   jsonRprPresent,  jsonRprRelease  },
  { EntityByIdAttributeByName,             "*",    "registerProviderRequest",                       jsonRprParseVector,  jsonRprInit,  jsonRprCheck,   jsonRprPresent,  jsonRprRelease  },
  { ContextEntityTypes,                    "POST", "registerProviderRequest",                       jsonRprParseVector,  jsonRprInit,  jsonRprCheck,   jsonRprPresent,  jsonRprRelease  },
  { ContextEntityTypes,                    "*",    "registerProviderRequest",                       jsonRprParseVector,  jsonRprInit,  jsonRprCheck,   jsonRprPresent,  jsonRprRelease  },
  { ContextEntityTypeAttribute,            "POST", "registerProviderRequest",                       jsonRprParseVector,  jsonRprInit,  jsonRprCheck,   jsonRprPresent,  jsonRprRelease  },
  { ContextEntityTypeAttribute,            "*",    "registerProviderRequest",                       jsonRprParseVector,  jsonRprInit,  jsonRprCheck,   jsonRprPresent,  jsonRprRelease  },
  { Ngsi9SubscriptionsConvOp,              "PUT",  "updateContextAvailabilitySubscriptionRequest",  jsonUcasParseVector, jsonUcasInit, jsonUcasCheck,  jsonUcasPresent, jsonUcasRelease },
  { IndividualContextEntity,               "PUT",  "updateContextElementRequest",                   jsonUcerParseVector, jsonUcerInit, jsonUcerCheck,  jsonUcerPresent, jsonUcerRelease },
  { IndividualContextEntity,               "POST", "appendContextElementRequest",                   jsonAcerParseVector, jsonAcerInit, jsonAcerCheck,  jsonAcerPresent, jsonAcerRelease },
  { IndividualContextEntityAttribute,      "POST", "updateContextAttributeRequest",                 jsonUpcarParseVector,jsonUpcarInit,jsonUpcarCheck, jsonUpcarPresent,jsonUpcarRelease},
  { IndividualContextEntityAttribute,      "PUT",  "updateContextAttributeRequest",                 jsonUpcarParseVector,jsonUpcarInit,jsonUpcarCheck, jsonUpcarPresent,jsonUpcarRelease},
  { IndividualContextEntityAttributes,     "POST", "appendContextElementRequest",                   jsonAcerParseVector, jsonAcerInit, jsonAcerCheck,  jsonAcerPresent, jsonAcerRelease },
  { IndividualContextEntityAttributes,     "PUT",  "updateContextElementRequest",                   jsonUcerParseVector, jsonUcerInit, jsonUcerCheck,  jsonUcerPresent, jsonUcerRelease },
  { AttributeValueInstance,                "PUT",  "updateContextAttributeRequest",                 jsonUpcarParseVector,jsonUpcarInit,jsonUpcarCheck, jsonUpcarPresent,jsonUpcarRelease},
  { Ngsi10SubscriptionsConvOp,             "PUT",  "updateContextSubscriptionRequest",              jsonUcsrParseVector, jsonUcsrInit, jsonUcsrCheck,  jsonUcsrPresent, jsonUcsrRelease }

};


/* ****************************************************************************
*
* jsonRequestGet - 
*/
static JsonRequest* jsonRequestGet(RequestType request, std::string method)
{
  for (unsigned int ix = 0; ix < sizeof(jsonRequest) / sizeof(jsonRequest[0]); ++ix)
  {
    if ((request == jsonRequest[ix].type) && (jsonRequest[ix].method == method))
    {
      if (jsonRequest[ix].parseVector != NULL)
        LM_V2(("Found jsonRequest of type %d, method '%s' - index %d (%s)", request, method.c_str(), ix, jsonRequest[ix].parseVector[0].path.c_str()));
      return &jsonRequest[ix];
    }
  }

  LM_E(("No request found for RequestType '%s', method '%s'", requestType(request), method.c_str()));
  return NULL;
}


/* ****************************************************************************
*
* jsonTreat - 
*/
std::string jsonTreat(const char* content, ConnectionInfo* ciP, ParseData* parseDataP, RequestType request, std::string payloadWord, JsonRequest** reqPP)
{
  std::string   res   = "OK";
  JsonRequest*  reqP  = jsonRequestGet(request, ciP->method);

  LM_T(LmtParse, ("Treating a JSON request: '%s'", content));

  ciP->parseDataP = parseDataP;

  if (reqP == NULL)
  {
    std::string errorReply = restErrorReplyGet(ciP, ciP->outFormat, "", requestType(request), SccBadRequest,
                                               std::string("Sorry, no request treating object found for RequestType '") + requestType(request) + "'");

    LM_RE(errorReply, ("Sorry, no request treating object found for RequestType %d (%s)", request, requestType(request)));
  }

  if (reqPP != NULL)
    *reqPP = reqP;

  LM_T(LmtParse, ("Treating '%s' request", reqP->keyword.c_str()));

  reqP->init(parseDataP);

  try
  {
    res = jsonParse(ciP, content, reqP->keyword, reqP->parseVector, parseDataP);
    if (ciP->inCompoundValue == true)
      orion::compoundValueEnd(ciP, "/end/caught/late", parseDataP);
    if ((lmTraceIsSet(LmtCompoundValueShow)) && (ciP->compoundValueP != NULL))
      ciP->compoundValueP->shortShow("after parse: ");
  }
  catch (std::exception &e)
  {
    std::string errorReply  = restErrorReplyGet(ciP, ciP->outFormat, "", reqP->keyword, SccBadRequest, std::string("JSON Parse Error: ") + e.what());
    LM_E(("JSON Parse Error: '%s'", e.what()));
    LM_RE(errorReply, (res.c_str()));
  }

  if (res != "OK")
  {
    LM_E(("JSON parse error: %s", res.c_str()));
    ciP->httpStatusCode = SccBadRequest;

    std::string answer = restErrorReplyGet(ciP, ciP->outFormat, "", payloadWord, ciP->httpStatusCode, res);
    return answer; 
  }

  reqP->present(parseDataP);

  LM_T(LmtParseCheck, ("Calling check for JSON parsed tree (%s)", ciP->payloadWord));
  res = reqP->check(parseDataP, ciP);
  reqP->present(parseDataP);

  return res;
}
