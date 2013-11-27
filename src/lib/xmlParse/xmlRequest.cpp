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
  { IndividualContextEntityAttributes,     "POST", "appendContextElementRequest",                  acerParseVector,  acerInit,  acerRelease,  acerPresent,  acerCheck  },
  { IndividualContextEntityAttributes,     "PUT",  "updateContextElementRequest",                  ucerParseVector,  ucerInit,  ucerRelease,  ucerPresent,  ucerCheck  },
  { AppendContextElement,                  "POST", "appendContextElementRequest",                  acerParseVector,  acerInit,  acerRelease,  acerPresent,  acerCheck  },
  { UpdateContextAttribute,                "POST", "updateContextAttributeRequest",                upcarParseVector, upcarInit, upcarRelease, upcarPresent, upcarCheck },

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
        LM_V2(("Found xmlRequest of type %d, method '%s' - index %d (%s)", request, method.c_str(), ix, xmlRequest[ix].parseVector[0].path.c_str()));
      return &xmlRequest[ix];
    }
  }

  LM_E(("No request found for RequestType '%s', method '%s'", requestType(request), method.c_str()));
  return NULL;
}



/* ****************************************************************************
*
* xmlDocPrepare - 
*/
static xml_node<>* xmlDocPrepare(char* xml)
{
  xml_document<> doc;

  try
  {
    doc.parse<0>(xml);     // 0 means default parse flags
  }
  catch (parse_error e)
  {
    LM_RE(NULL, ("PARSE ERROR: %s", e.what()));
  }

  xml_node<>* father = doc.first_node();

  return father;
}



/* ****************************************************************************
*
* xmlTreat -
*/
std::string xmlTreat(const char* content, ConnectionInfo* ciP, ParseData* parseDataP, RequestType request, std::string payloadWord, XmlRequest** reqPP)
{
  xml_node<>*   father    = xmlDocPrepare((char*) content);
  std::string   res       = "OK";
  XmlRequest*   reqP      = xmlRequestGet(request, ciP->method);

  if (father == NULL)
  {
    std::string errorReply = restErrorReplyGet(ciP, ciP->outFormat, "", "unknown", SccBadRequest, "Parse Error", "");
    LM_RE(errorReply, ("Parse Error"));
  }

  if (reqP == NULL)
  {
    std::string errorReply = restErrorReplyGet(ciP, ciP->outFormat, "", requestType(request),
                                               SccBadRequest,
                                               "no request treating object found",
                                               std::string("Sorry, no request treating object found for RequestType '") + requestType(request) + "'");

    LM_RE(errorReply, ("Sorry, no request treating object found for RequestType %d (%s), method %s", request, requestType(request), ciP->method.c_str()));
  }


  //
  // Checking that the payload matches the URL
  //
  if (((ciP->verb == POST) || (ciP->verb == PUT)) && (payloadWord.length() != 0))
  {
    char* payloadStart = (char*) content;

    if (reqP->parseVector == NULL)
    {
      std::string errorReply = restErrorReplyGet(ciP, ciP->outFormat, "", reqP->keyword, SccNotImplemented, "Not Implemented", std::string("Sorry, the '") + reqP->keyword.c_str() + "' object is not implemented");
      LM_RE(errorReply, ("Not Implemented"));
    }

    // Skip '<?xml version="1.0" encoding="UTF-8"?> ' ?
    if (strncmp(payloadStart, "<?xml", 5) == 0)
    {
      char* oldPayloadStart = payloadStart;

      payloadStart = strstr(payloadStart, ">");
      if (payloadStart == NULL)
      {
        LM_E(("Found '<?xml' but NOT '>' in '%s'", oldPayloadStart));
        payloadStart = (char*) content;  // Going back ...
      }
      else
      {
        oldPayloadStart = payloadStart;
        payloadStart = strstr(payloadStart, "<");
         
        if (payloadStart == NULL)
        {
          LM_E(("Found '<?xml' and '>' but NOT '<' in '%s'", oldPayloadStart));
          payloadStart = (char*) content;  // Going back ...
        }
      }
    }

    if (*payloadStart == '<')
    {
       ++payloadStart; // Skip '<'
    }

    if (strncasecmp(payloadWord.c_str(), payloadStart, payloadWord.length()) != 0)
    {
      std::string  errorReply;

      errorReply  = restErrorReplyGet(ciP, ciP->outFormat, "", reqP->keyword, SccBadRequest, "Invalid payload", std::string("Expected '") + payloadWord + "' payload, got '" + payloadStart + "'");
      LM_RE(errorReply, ("Invalid payload: wanted: '%s', got '%s'", payloadWord.c_str(), payloadStart));
    }
  }

  if (reqP->init == NULL) // No payload
    return "OK";

  reqP->init(parseDataP);
  xmlParse(NULL, father, "", "", reqP->parseVector, parseDataP);

  std::string check = reqP->check(parseDataP, ciP);
  if (check != "OK")
     LM_E(("check(%s): %s", reqP->keyword.c_str(), check.c_str()));

  reqP->present(parseDataP);

  // -----------------------------
  //
  // Can't release here ...
  // reqP->release(parseDataP);
  //
  // pass request pointer to father that will know when the free can be executed.
  //
  if (reqPP != NULL)
  {
    LM_T(LmtMetadataDoubleFree, ("Saving pointer to '%s' to free later (hope it hasn't been freed already ...)", reqP->keyword.c_str()));
    *reqPP = reqP;
  }

  return check;
}
