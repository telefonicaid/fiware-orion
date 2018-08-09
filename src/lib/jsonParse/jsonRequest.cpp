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
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include <exception>
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/limits.h"
#include "common/errorMessages.h"
#include "alarmMgr/alarmMgr.h"
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
#include "jsonParse/jsonQueryContextResponse.h"
#include "jsonParse/jsonUpdateContextRequest.h"
#include "jsonParse/jsonUpdateContextResponse.h"
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


#define FUNCS(prefix) json##prefix##ParseVector, json##prefix##Init,    \
                      json##prefix##Check,       json##prefix##Release



/* ****************************************************************************
*
* jsonRequest -
*/
static JsonRequest jsonRequest[] =
{
  // NGSI9
  { RegisterContext,                       "POST", "registerContextRequest",                        FUNCS(Rcr)   },
  { DiscoverContextAvailability,           "POST", "discoverContextAvailabilityRequest",            FUNCS(Dcar)  },
  { SubscribeContextAvailability,          "POST", "subscribeContextAvailabilityRequest",           FUNCS(Scar)  },
  { UnsubscribeContextAvailability,        "POST", "unsubscribeContextAvailabilityRequest",         FUNCS(Ucar)  },
  { NotifyContextAvailability,             "POST", "notifyContextRequestAvailability",              FUNCS(Ncar)  },
  { UpdateContextAvailabilitySubscription, "POST", "updateContextAvailabilitySubscriptionRequest",  FUNCS(Ucas)  },

  // NGSI10
  { QueryContext,                          "POST", "queryContextRequest",                           FUNCS(Qcr)   },
  { UpdateContext,                         "POST", "updateContextRequest",                          FUNCS(Upcr)  },
  { SubscribeContext,                      "POST", "subscribeContextRequest",                       FUNCS(Scr)   },
  { NotifyContext,                         "POST", "notifyContextRequest",                          FUNCS(Ncr)   },
  { UnsubscribeContext,                    "POST", "unsubscribeContextRequest",                     FUNCS(Uncr)  },
  { UpdateContextSubscription,             "POST", "updateContextSubscriptionRequest",              FUNCS(Ucsr)  },

  // Convenience
  { ContextEntitiesByEntityId,             "POST", "registerProviderRequest",                       FUNCS(Rpr)   },
  { ContextEntitiesByEntityId,             "*",    "registerProviderRequest",                       FUNCS(Rpr)   },
  { EntityByIdAttributeByName,             "POST", "registerProviderRequest",                       FUNCS(Rpr)   },
  { EntityByIdAttributeByName,             "*",    "registerProviderRequest",                       FUNCS(Rpr)   },
  { ContextEntityAttributes,               "POST", "registerProviderRequest",                       FUNCS(Rpr)   },
  { ContextEntityAttributes,               "*",    "registerProviderRequest",                       FUNCS(Rpr)   },
  { ContextEntityTypeAttributeContainer,   "POST", "registerProviderRequest",                       FUNCS(Rpr)   },
  { ContextEntityTypeAttributeContainer,   "*",    "registerProviderRequest",                       FUNCS(Rpr)   },
  { ContextEntityTypeAttribute,            "POST", "registerProviderRequest",                       FUNCS(Rpr)   },
  { ContextEntityTypeAttribute,            "*",    "registerProviderRequest",                       FUNCS(Rpr)   },

  { IndividualContextEntity,               "PUT",  "updateContextElementRequest",                   FUNCS(Ucer)  },
  { IndividualContextEntity,               "POST", "appendContextElementRequest",                   FUNCS(Acer)  },
  { AllContextEntities,                    "POST", "appendContextElementRequest",                   FUNCS(Acer)  },

  { ContextEntityTypes,                    "POST", "registerProviderRequest",                       FUNCS(Rpr)   },
  { ContextEntityTypes,                    "*",    "registerProviderRequest",                       FUNCS(Rpr)   },
  { Ngsi9SubscriptionsConvOp,              "PUT",  "updateContextAvailabilitySubscriptionRequest",  FUNCS(Ucas)  },
  { IndividualContextEntityAttribute,      "POST", "updateContextAttributeRequest",                 FUNCS(Upcar) },
  { IndividualContextEntityAttribute,      "PUT",  "updateContextAttributeRequest",                 FUNCS(Upcar) },
  { IndividualContextEntityAttributes,     "POST", "appendContextElementRequest",                   FUNCS(Acer)  },
  { IndividualContextEntityAttributes,     "PUT",  "updateContextElementRequest",                   FUNCS(Ucer)  },
  { AttributeValueInstance,                "PUT",  "updateContextAttributeRequest",                 FUNCS(Upcar) },
  { Ngsi10SubscriptionsConvOp,             "PUT",  "updateContextSubscriptionRequest",              FUNCS(Ucsr)  },

  { AllEntitiesWithTypeAndId,              "PUT",  "updateContextElementRequest",                   FUNCS(Ucer)  },
  { AllEntitiesWithTypeAndId,              "POST", "appendContextElementRequest",                   FUNCS(Acer)  },
  { AllEntitiesWithTypeAndId,              "*",    "",                                              FUNCS(Ucer)  },

  { IndividualContextEntityAttributeWithTypeAndId, "POST", "updateContextAttributeRequest",         FUNCS(Upcar) },
  { IndividualContextEntityAttributeWithTypeAndId, "PUT",  "updateContextAttributeRequest",         FUNCS(Upcar) },

  { AttributeValueInstanceWithTypeAndId,           "PUT",  "updateContextAttributeRequest",         FUNCS(Upcar) },
  { AttributeValueInstanceWithTypeAndId,           "POST", "updateContextAttributeRequest",         FUNCS(Upcar) },

  { ContextEntitiesByEntityIdAndType,              "POST", "registerProviderRequest",               FUNCS(Rpr)   },
  { EntityByIdAttributeByNameIdAndType,            "POST", "registerProviderRequest",               FUNCS(Rpr)   },

  { RtQueryContextResponse,                        "POST", "queryContextResponse",                  FUNCS(Qcrs)  },
  { RtUpdateContextResponse,                       "POST", "updateContextResponse",                 FUNCS(Upcrs) }
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
      {
        LM_T(LmtHttpRequest, ("Found jsonRequest of type %d, method '%s' - index %d (%s)",
              request, method.c_str(), ix, jsonRequest[ix].parseVector[0].path.c_str()));
      }

      return &jsonRequest[ix];
    }
  }

  std::string details = std::string("no request found for RequestType '") + requestType(request) + "'', method '" + method + "'";
  alarmMgr.badInput(clientIp, details);

  return NULL;
}



/* ****************************************************************************
*
* jsonTreat -
*/
std::string jsonTreat
(
  const char*         content,
  ConnectionInfo*     ciP,
  ParseData*          parseDataP,
  RequestType         request,
  JsonRequest**       reqPP
)
{
  std::string   res   = "OK";
  JsonRequest*  reqP  = jsonRequestGet(request, ciP->method);

  //
  // FIXME P4 #1862:
  //
  // This check comes from the old XML days, as the the XML parsing library did an assert
  // and the broker died. We need to test what happen with the JSON library.
  // If JSON library is "safer" with that regards, the check should be removed.
  //
  // 'OK' is returned as there is no error to send a request without payload.
  //
  if ((content == NULL) || (*content == 0))
  {
    return "OK";
  }

  LM_T(LmtParse, ("Treating a JSON request: '%s'", content));

  ciP->parseDataP = parseDataP;

  if (reqP == NULL)
  {
    std::string  errorReply;
    char         reqTypeV[STRING_SIZE_FOR_INT];

    restErrorReplyGet(ciP, SccBadRequest, SERVICE_NOT_FOUND, &errorReply);
    snprintf(reqTypeV, sizeof(reqTypeV), "%d", request);

    std::string details = std::string("no request treating object found for RequestType ") + reqTypeV + " (" + requestType(request) + ")";
    alarmMgr.badInput(clientIp, details);

    return errorReply;
  }

  if (reqPP != NULL)
  {
    *reqPP = reqP;
  }

  LM_T(LmtParse, ("Treating '%s' request", reqP->keyword.c_str()));

  reqP->init(parseDataP);

  try
  {
    res = jsonParse(ciP, content, reqP->keyword, reqP->parseVector, parseDataP);
  }
  catch (const std::exception &e)
  {
    std::string details = std::string("JSON Parse Error: ") + e.what();
    std::string errorReply;

    restErrorReplyGet(ciP, SccBadRequest, "JSON Parse Error", &errorReply);
    alarmMgr.badInput(clientIp, details);
    return errorReply;
  }
  catch (...)
  {
    std::string errorReply;

    restErrorReplyGet(ciP, SccBadRequest, "JSON Generic Error", &errorReply);
    alarmMgr.badInput(clientIp, "JSON parse generic error");
    return errorReply;
  }

  if (res != "OK")
  {
    std::string details = std::string("JSON parse error: ") + res;
    std::string answer;
    
    alarmMgr.badInput(clientIp, details);
    ciP->httpStatusCode = SccBadRequest;
    restErrorReplyGet(ciP, ciP->httpStatusCode, res, &answer);
    return answer;
  }

  if (ciP->inCompoundValue == true)
  {
    orion::compoundValueEnd(ciP, parseDataP);
  }
  if ((lmTraceIsSet(LmtCompoundValueShow)) && (ciP->compoundValueP != NULL))
  {
    ciP->compoundValueP->shortShow("after parse: ");
  }

  res = reqP->check(parseDataP, ciP);
  if (res != "OK")
  {
    std::string details = reqP->keyword + ": " + res;
    alarmMgr.badInput(clientIp, details);
  }

  return res;
}
