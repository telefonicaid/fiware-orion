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
#include <string.h>                 // strstr

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi/ParseData.h"
#include "ngsi/Request.h"
#include "rest/restReply.h"
#include "rest/OrionError.h"
#include "xmlParse/xmlRegisterContextRequest.h"
#include "xmlParse/xmlRegisterContextResponse.h"
#include "xmlParse/xmlDiscoverContextAvailabilityRequest.h"
#include "xmlParse/xmlSubscribeContextAvailabilityRequest.h"
#include "xmlParse/xmlUnsubscribeContextAvailabilityRequest.h"
#include "xmlParse/xmlNotifyContextRequest.h"
#include "xmlParse/xmlNotifyContextAvailabilityRequest.h"
#include "xmlParse/xmlUpdateContextAvailabilitySubscriptionRequest.h"
#include "xmlParse/xmlQueryContextRequest.h"
#include "xmlParse/xmlSubscribeContextRequest.h"
#include "xmlParse/xmlUnsubscribeContextRequest.h"
#include "xmlParse/xmlUpdateContextSubscriptionRequest.h"
#include "xmlParse/xmlUpdateContextRequest.h"
#include "xmlParse/xmlRegisterProviderRequest.h"
#include "xmlParse/xmlUpdateContextElementRequest.h"
#include "xmlParse/xmlAppendContextElementRequest.h"
#include "xmlParse/xmlUpdateContextAttributeRequest.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlParse.h"
#include "xmlParse/xmlRequest.h"



/* ****************************************************************************
*
* xmlRequest - the vector used to parse the payload in XML
*
*  RequestType     type        - the type of request (6 ngsi9, 6 ngsi10)
*  std::string     method      - the method/verb of the request (POST, PUT, etc)
*  std::string     keyword     - first word in payload
*  XmlNode*        parseVector - important vector for the parsing of each element
*  RequestInit     init        - init function of the parsing
*  RequestRelease  release     - cleanup function called after the parsing is completed
*  RequestPresent  present     - debugging tool
*  RequestCheck    check       - function to verify the the payload is ngsi compliant
*
*/
static XmlRequest xmlRequest[] = 
{
  // NGSI9
  { RegisterContext,                       "POST", "registerContextRequest",                       rcrParseVector,   rcrInit,   rcrRelease,   rcrPresent,   rcrCheck   },
  { RegisterContext,                       "*",    "registerContextRequest",                       rcrParseVector,   rcrInit,   rcrRelease,   rcrPresent,   rcrCheck   },
  { DiscoverContextAvailability,           "POST", "discoverContextAvailabilityRequest",           dcarParseVector,  dcarInit,  dcarRelease,  dcarPresent,  dcarCheck  },
  { DiscoverContextAvailability,           "*",    "discoverContextAvailabilityRequest",           dcarParseVector,  dcarInit,  dcarRelease,  dcarPresent,  dcarCheck  },
  { SubscribeContextAvailability,          "POST", "subscribeContextAvailabilityRequest",          scarParseVector,  scarInit,  scarRelease,  scarPresent,  scarCheck  },
  { SubscribeContextAvailability,          "*",    "subscribeContextAvailabilityRequest",          scarParseVector,  scarInit,  scarRelease,  scarPresent,  scarCheck  },
  { UnsubscribeContextAvailability,        "POST", "unsubscribeContextAvailabilityRequest",        ucarParseVector,  ucarInit,  ucarRelease,  ucarPresent,  ucarCheck  },
  { UnsubscribeContextAvailability,        "*",    "unsubscribeContextAvailabilityRequest",        ucarParseVector,  ucarInit,  ucarRelease,  ucarPresent,  ucarCheck  },
  { UpdateContextAvailabilitySubscription, "POST", "updateContextAvailabilitySubscriptionRequest", ucasParseVector,  ucasInit,  ucasRelease,  ucasPresent,  ucasCheck  },
  { UpdateContextAvailabilitySubscription, "*",    "updateContextAvailabilitySubscriptionRequest", ucasParseVector,  ucasInit,  ucasRelease,  ucasPresent,  ucasCheck  },
  { NotifyContextAvailability,             "POST", "notifyContextAvailabilityRequest",             ncarParseVector,  ncarInit,  ncarRelease,  ncarPresent,  ncarCheck  },
  { NotifyContextAvailability,             "*",    "notifyContextAvailabilityRequest",             ncarParseVector,  ncarInit,  ncarRelease,  ncarPresent,  ncarCheck  },

  // NGSI10
  { UpdateContext,                         "POST", "updateContextRequest",                         upcrParseVector,  upcrInit,  upcrRelease,  upcrPresent,  upcrCheck  },
  { UpdateContext,                         "*",    "updateContextRequest",                         upcrParseVector,  upcrInit,  upcrRelease,  upcrPresent,  upcrCheck  },
  { QueryContext,                          "POST", "queryContextRequest",                          qcrParseVector,   qcrInit,   qcrRelease,   qcrPresent,   qcrCheck   },
  { QueryContext,                          "*",    "queryContextRequest",                          qcrParseVector,   qcrInit,   qcrRelease,   qcrPresent,   qcrCheck   },
  { SubscribeContext,                      "POST", "subscribeContextRequest",                      scrParseVector,   scrInit,   scrRelease,   scrPresent,   scrCheck   },
  { SubscribeContext,                      "*",    "subscribeContextRequest",                      scrParseVector,   scrInit,   scrRelease,   scrPresent,   scrCheck   },
  { UpdateContextSubscription,             "POST", "updateContextSubscriptionRequest",             ucsrParseVector,  ucsrInit,  ucsrRelease,  ucsrPresent,  ucsrCheck  },
  { UpdateContextSubscription,             "*",    "updateContextSubscriptionRequest",             ucsrParseVector,  ucsrInit,  ucsrRelease,  ucsrPresent,  ucsrCheck  },
  { UnsubscribeContext,                    "POST", "unsubscribeContextRequest",                    uncrParseVector,  uncrInit,  uncrRelease,  uncrPresent,  uncrCheck  },
  { UnsubscribeContext,                    "*",    "unsubscribeContextRequest",                    uncrParseVector,  uncrInit,  uncrRelease,  uncrPresent,  uncrCheck  },
  { NotifyContext,                         "POST", "notifyContextRequest",                         ncrParseVector,   ncrInit,   ncrRelease,   ncrPresent,   ncrCheck   },
  { NotifyContext,                         "*",    "notifyContextRequest",                         ncrParseVector,   ncrInit,   ncrRelease,   ncrPresent,   ncrCheck   },

  // Convenience
  { ContextEntitiesByEntityId,             "POST", "registerProviderRequest",                      rprParseVector,   rprInit,   rprRelease,   rprPresent,   rprCheck   },
  { ContextEntitiesByEntityId,             "*",    "registerProviderRequest",                      rprParseVector,   rprInit,   rprRelease,   rprPresent,   rprCheck   },
  { EntityByIdAttributeByName,             "POST", "registerProviderRequest",                      rprParseVector,   rprInit,   rprRelease,   rprPresent,   rprCheck   },
  { EntityByIdAttributeByName,             "*",    "registerProviderRequest",                      rprParseVector,   rprInit,   rprRelease,   rprPresent,   rprCheck   },
  { ContextEntityAttributes,               "POST", "registerProviderRequest",                      rprParseVector,   rprInit,   rprRelease,   rprPresent,   rprCheck   },
  { ContextEntityAttributes,               "*",    "registerProviderRequest",                      rprParseVector,   rprInit,   rprRelease,   rprPresent,   rprCheck   },
  { ContextEntityTypes,                    "POST", "registerProviderRequest",                      rprParseVector,   rprInit,   rprRelease,   rprPresent,   rprCheck   },
  { ContextEntityTypes,                    "*",    "registerProviderRequest",                      rprParseVector,   rprInit,   rprRelease,   rprPresent,   rprCheck   },
  { ContextEntityTypeAttributeContainer,   "POST", "registerProviderRequest",                      rprParseVector,   rprInit,   rprRelease,   rprPresent,   rprCheck   },
  { ContextEntityTypeAttributeContainer,   "*",    "registerProviderRequest",                      rprParseVector,   rprInit,   rprRelease,   rprPresent,   rprCheck   },
  { ContextEntityTypeAttribute,            "POST", "registerProviderRequest",                      rprParseVector,   rprInit,   rprRelease,   rprPresent,   rprCheck   },
  { ContextEntityTypeAttribute,            "*",    "registerProviderRequest",                      rprParseVector,   rprInit,   rprRelease,   rprPresent,   rprCheck   },

  { UpdateContextElement,                  "POST", "updateContextElementRequest",                  ucerParseVector,  ucerInit,  ucerRelease,  ucerPresent,  ucerCheck  },
  { IndividualContextEntity,               "PUT",  "updateContextElementRequest",                  ucerParseVector,  ucerInit,  ucerRelease,  ucerPresent,  ucerCheck  },
  { IndividualContextEntity,               "POST", "appendContextElementRequest",                  acerParseVector,  acerInit,  acerRelease,  acerPresent,  acerCheck  },

  { IndividualContextEntityAttribute,      "POST", "updateContextAttributeRequest",                upcarParseVector, upcarInit, upcarRelease, upcarPresent, upcarCheck },
  { IndividualContextEntityAttribute,      "PUT",  "updateContextAttributeRequest",                upcarParseVector, upcarInit, upcarRelease, upcarPresent, upcarCheck },
  { AttributeValueInstance,                "PUT",  "updateContextAttributeRequest",                upcarParseVector, upcarInit, upcarRelease, upcarPresent, upcarCheck },
  { IndividualContextEntityAttributes,     "POST", "appendContextElementRequest",                  acerParseVector,  acerInit,  acerRelease,  acerPresent,  acerCheck  },
  { IndividualContextEntityAttributes,     "PUT",  "updateContextElementRequest",                  ucerParseVector,  ucerInit,  ucerRelease,  ucerPresent,  ucerCheck  },
  { AppendContextElement,                  "POST", "appendContextElementRequest",                  acerParseVector,  acerInit,  acerRelease,  acerPresent,  acerCheck  },
  { UpdateContextAttribute,                "POST", "updateContextAttributeRequest",                upcarParseVector, upcarInit, upcarRelease, upcarPresent, upcarCheck },

  { SubscribeContext,                      "POST", "subscribeContextRequest",                      scrParseVector,   scrInit,   scrRelease,   scrPresent,   scrCheck   },
  { Ngsi10SubscriptionsConvOp,             "PUT",  "updateContextSubscriptionRequest",             ucsrParseVector,  ucsrInit,  ucsrRelease,  ucsrPresent,  ucsrCheck  },
  { Ngsi9SubscriptionsConvOp,              "POST", "subscribeContextAvailabilityRequest",          scarParseVector,  scarInit,  scarRelease,  scarPresent,  scarCheck  },
  { Ngsi9SubscriptionsConvOp,              "PUT",  "updateContextvailabilitySubscriptionRequest",  ucasParseVector,  ucasInit,  ucasRelease,  ucasPresent,  ucasCheck  },

  // Responses
  { RegisterResponse,                      "POST", "registerContextResponse",                      rcrsParseVector,  rcrsInit,  rcrsRelease,  rcrsPresent,  rcrsCheck  },

  // Without payload
  { LogRequest,                            "*", "", NULL, NULL, NULL, NULL, NULL },
  { VersionRequest,                        "*", "", NULL, NULL, NULL, NULL, NULL },
  { ExitRequest,                           "*", "", NULL, NULL, NULL, NULL, NULL },
  { InvalidRequest,                        "*", "", NULL, NULL, NULL, NULL, NULL },
};



/* ****************************************************************************
*
* xmlRequestGet - 
*/
static XmlRequest* xmlRequestGet(RequestType request, std::string method)
{
  for (unsigned int ix = 0; ix < sizeof(xmlRequest) / sizeof(xmlRequest[0]); ++ix)
  {
    if ((request == xmlRequest[ix].type) && ((xmlRequest[ix].method == method) || (xmlRequest[ix].method == "*")))
    {
      if (xmlRequest[ix].parseVector != NULL)
        LM_T(LmtHttpRequest, ("Found xmlRequest of type %d, method '%s' - index %d (%s)", request, method.c_str(), ix, xmlRequest[ix].parseVector[0].path.c_str()));
      return &xmlRequest[ix];
    }
  }

  LM_W(("Bad Input (no request found for RequestType '%s', method '%s')", requestType(request), method.c_str()));
  return NULL;
}



/* ****************************************************************************
*
* xmlTreat -
*/
std::string xmlTreat(const char* content, ConnectionInfo* ciP, ParseData* parseDataP, RequestType request, std::string payloadWord, XmlRequest** reqPP)
{
  xml_document<> doc;
  char*          xmlPayload = (char*) content;

  try
  {
    doc.parse<0>(xmlPayload);
  }
  catch (parse_error& e)
  {
    std::string errorReply = restErrorReplyGet(ciP, ciP->outFormat, "", "unknown", SccBadRequest, "XML Parse Error");
    LM_W(("Bad Input ('%s', '%s')", content, e.what()));
    return errorReply;
  }
  catch (...)
  {
    std::string errorReply = restErrorReplyGet(ciP, ciP->outFormat, "", "unknown", SccBadRequest, "XML Parse Error");
    LM_W(("Bad Input (%s)", content));
    return errorReply;
  }

  xml_node<>*   father = doc.first_node();
  XmlRequest*   reqP   = xmlRequestGet(request, ciP->method);

  ciP->parseDataP = parseDataP;

  if (father == NULL)
  {
    std::string errorReply = restErrorReplyGet(ciP, ciP->outFormat, "", "unknown", SccBadRequest, "XML Parse Error");
    LM_W(("Bad Input (XML parse error)"));
    return errorReply;
  }

  if (reqP == NULL)
  {
    std::string errorReply = restErrorReplyGet(ciP, ciP->outFormat, "", requestType(request),
                                               SccBadRequest,
                                               std::string("Sorry, no request treating object found for RequestType '") + requestType(request) + "', method '" + ciP->method + "'");

    LM_W(("Bad Input (no request treating object found for RequestType %d (%s), method %s)", request, requestType(request), ciP->method.c_str()));
    return errorReply;
  }


  if (reqPP != NULL)
    *reqPP = reqP;

  //
  // Checking that the payload matches the URL
  //
  if (((ciP->verb == POST) || (ciP->verb == PUT)) && (payloadWord.length() != 0))
  {
    std::string  errorReply;
    char*        payloadStart = (char*) content;

    // Skip '<?xml version="1.0" encoding="UTF-8"?> ' ?
    if (strncmp(payloadStart, "<?xml", 5) == 0)
    {
      ++payloadStart;
      payloadStart = strstr(payloadStart, "<");
    }

    // Skip '<'
    if (*payloadStart == '<')
       ++payloadStart;

    if (strncasecmp(payloadWord.c_str(), payloadStart, payloadWord.length()) != 0)
    {
      errorReply  = restErrorReplyGet(ciP, ciP->outFormat, "", reqP->keyword, SccBadRequest, std::string("Expected '") + payloadWord + "' payload, got '" + payloadStart + "'");
      LM_W(("Bad Input (invalid  payload: wanted: '%s', got '%s')", payloadWord.c_str(), payloadStart));
      return errorReply;
    }
  }

  if (reqP->init == NULL) // No payload treating function
    return "OK";

  reqP->init(parseDataP);
  ciP->httpStatusCode = SccOk;
  xmlParse(ciP, NULL, father, "", "", reqP->parseVector, parseDataP);
  if (ciP->httpStatusCode != SccOk)
  {
    LM_W(("Bad Input (XML parse error)"));
    return restErrorReplyGet(ciP, ciP->outFormat, "", payloadWord, ciP->httpStatusCode, ciP->answer);
  }

  LM_T(LmtParseCheck, ("Calling check for XML parsed tree (%s)", ciP->payloadWord));
  std::string check = reqP->check(parseDataP, ciP);
  if (check != "OK")
    LM_W(("Bad Input (%s: %s)", reqP->keyword.c_str(), check.c_str()));

  reqP->present(parseDataP);

  return check;
}
