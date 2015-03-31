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
#include <string>
#include <vector>

#include "parseArgs/baStd.h"    // BA_FT for temporal debugging purposes

#include "common/string.h"
#include "mongoBackend/mongoQueryContext.h"
#include "ngsi/ParseData.h"
#include "ngsi10/QueryContextResponse.h"
#include "rest/ConnectionInfo.h"
#include "rest/clientSocketHttp.h"
#include "serviceRoutines/postQueryContext.h"
#include "xmlParse/xmlRequest.h"



/* ****************************************************************************
*
* xmlPayloadClean -
*/
static char* xmlPayloadClean(const char*  payload, const char* payloadWord)
{
  return (char*) strstr(payload, payloadWord);
}



/* ****************************************************************************
*
* postQueryContext -
*/
std::string postQueryContext
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  //
  // Convops calling this routine may need the response in digital
  // So, the digital response is passed back in parseDataP->qcrs.res
  //
  QueryContextResponse*  qcrP = &parseDataP->qcrs.res;
  std::string            answer;

  ciP->httpStatusCode = mongoQueryContext(&parseDataP->qcr.res, qcrP, ciP->tenant, ciP->servicePathV, ciP->uriParam);

  LM_M(("KZ: Got a reply from mongoQueryContext with %d contextElementResponses", qcrP->contextElementResponseVector.size()));
  for (unsigned int ix = 0 ; ix < qcrP->contextElementResponseVector.size(); ++ix)
  {
    ContextElementResponse* cerP = qcrP->contextElementResponseVector[ix];

    LM_M(("KZ: contextElement %d contains %d attributes", ix, cerP->contextElement.contextAttributeVector.size()));

    for (unsigned int aIx = 0 ; aIx < cerP->contextElement.contextAttributeVector.size(); ++aIx)
    {
      ContextAttribute* aP = cerP->contextElement.contextAttributeVector[aIx];

      LM_M(("KZ: Attribute %d in contextElement %d:", aIx, ix));
      LM_M(("KZ:   name:      '%s'", aP->name.c_str()));
      LM_M(("KZ:   type:      '%s'", aP->type.c_str()));
      LM_M(("KZ:   value:     '%s'", aP->value.c_str()));
      LM_M(("KZ:   found:     '%s'", BA_FT(aP->found)));
      LM_M(("KZ:   provApp:   '%s'", aP->providingApplication.c_str()));
    }
  }

  // If no redirectioning is necessary, just return the result
  if (qcrP->errorCode.code != SccFound)
  {
    //
    // In case the response is empty, fill response with the entity we looked for
    //
    if (qcrP->contextElementResponseVector.size() == 0)
    {
      //
      // FIXME P6: What do we do if we have more than one entity in incoming entityIdVector?
      //           Push each entityId in a separate contextElementResponse?
      //           Also, perhaps mongoQueryContext should implement this and not postQueryContext
      //           This FIXME is related to github issue #588 and (probably) #650.
      //           Also, optimizing this would be part of issue #768
      //


      //
      // It's better not to give any details here as we'd have to choose ONE of the entities in the vector ...
      // std::string details = "Entity id: /" + parseDataP->qcr.res.entityIdVector[0]->id + "/";
      //
      qcrP->errorCode.fill(SccContextElementNotFound);
    }

    answer = qcrP->render(ciP, QueryContext, "");
    return answer;
  }



  std::string     ip;
  std::string     protocol;
  int             port;
  std::string     prefix;

  //
  // An entity/attribute has been found on some context provider.
  // We need to forward the query request to the context provider, indicated in qcrP->errorCode.details.
  //
  // 1. Parse the providing application to extract IP, port and URI-path
  // 2. The exact same QueryContextRequest will be used for the query-forwarding
  // 3. Render an XML-string of the request we want to forward
  // 4. Send the request to the providing application (and await the response)
  // 5. Parse the XML response and fill in a binary QueryContextResponse
  // 6. Render QueryContextResponse from the providing application
  //


  //
  // 1. Parse the providing application to extract IP, port and URI-path
  //
  if (parseUrl(qcrP->errorCode.details, ip, port, prefix, protocol) == false)
  {
    QueryContextResponse qcrs;

    LM_W(("Bad Input (invalid proving application '%s')", qcrP->errorCode.details.c_str()));
    //  Somehow, if we accepted this providing application, it is the brokers fault ...
    //  SccBadRequest should have been returned before, when it was registered!

    qcrP->errorCode.fill(SccContextElementNotFound, "");
    answer = qcrP->render(ciP, QueryContext, "");
    return answer;
  }


  //
  // 2. The exact same QueryContextRequest will be used for the query-forwarding
  //
  QueryContextRequest* qcReqP = &parseDataP->qcr.res;


  //
  // 3. Render an XML-string of the request we want to forward
  //
  std::string payload = qcReqP->render(QueryContext, XML, "");
  char*       cleanPayload;

  if ((cleanPayload = xmlPayloadClean(payload.c_str(), "<queryContextRequest>")) == NULL)
  {
    QueryContextResponse qcrs;

    LM_E(("Runtime Error (error rendering forward-request)"));
    qcrs.errorCode.fill(SccContextElementNotFound, "");
    answer = qcrs.render(ciP, QueryContext, "");
    return answer;
  }


  //
  // 4. Send the request to the providing application (and await the reply)
  // FIXME P7: Should Rush be used?
  //
  std::string     out;
  std::string     verb         = "POST";
  std::string     resource     = prefix + "/queryContext";
  std::string     tenant       = ciP->tenant;
  std::string     servicePath  = (ciP->httpHeaders.servicePathReceived == true)? ciP->httpHeaders.servicePath : "";

  out = sendHttpSocket(ip,
                       port,
                       protocol,
                       verb,
                       tenant,
                       servicePath,
                       ciP->httpHeaders.xauthToken,
                       resource,
                       "application/xml",
                       payload,
                       false,
                       true);

  if ((out == "error") || (out == ""))
  {
    QueryContextResponse qcrs;

    qcrs.errorCode.fill(SccContextElementNotFound, "");
    answer = qcrs.render(ciP, QueryContext, "");
    LM_E(("Runtime Error (error forwarding 'Query' to providing application)"));
    return answer;
  }


  //
  // 5. Parse the XML response and fill in a binary QueryContextResponse
  //
  std::string  s;
  std::string  errorMsg;

  cleanPayload = xmlPayloadClean(out.c_str(), "<queryContextResponse>");

  if ((cleanPayload == NULL) || (cleanPayload[0] == 0))
  {
    QueryContextResponse qcrs;

    //
    // This is really an internal error in the Context Provider
    // It is not in the orion broker though, so 404 is returned
    //
    qcrs.errorCode.fill(SccContextElementNotFound, "invalid context provider response");

    LM_W(("Other Error (context provider response to QueryContext is empty)"));
    answer = qcrs.render(ciP, QueryContext, "");
    return answer;
  }

  //
  // NOTE
  // When coming from a convenience operation, such as GET /v1/contextEntities/EID/attributes/attrName,
  // the verb/method in ciP is GET. However, the parsing function expects a POST, as if it came from a 
  // POST /v1/queryContext. 
  // So, here we change the verb/method for POST.
  //
  ciP->verb   = POST;
  ciP->method = "POST";

  s = xmlTreat(cleanPayload, ciP, parseDataP, RtQueryContextResponse, "queryContextResponse", NULL, &errorMsg);
  if (s != "OK")
  {
    QueryContextResponse qcrs;

    qcrs.errorCode.fill(SccContextElementNotFound, "");
    LM_W(("Internal Error (error parsing reply from prov app: %s)", errorMsg.c_str()));
    answer = qcrs.render(ciP, QueryContext, "");
    return answer;
  }


  //
  // 6. Render QueryContextResponse from the providing application
  //
  char portV[16];
  snprintf(portV, sizeof(portV), "%d", port);

  // Fill in the response from the redirection into the response to the originator of this request
  QueryContextResponse* qcrsP = &parseDataP->qcrs.res;

  //
  // Returning 'redirected to' in StatusCode::details
  //
  if ((qcrsP->errorCode.code == SccOk) || (qcrsP->errorCode.details == ""))
  {
    qcrsP->errorCode.details = std::string("Redirected to context provider ") + ip + ":" + portV + prefix;
  }
  else if (qcrsP->errorCode.code == SccNone)
  {
    qcrsP->errorCode.fill(SccOk, std::string("Redirected to context provider ") + ip + ":" + portV + prefix);
  }

  if (qcrsP->contextElementResponseVector.size() > 0)
  {
    qcrsP->contextElementResponseVector[0]->statusCode.details = std::string("Redirected to context provider ") + ip + ":" + portV + prefix;
  }

  answer = qcrsP->render(ciP, QueryContext, "");
  return answer;
}
