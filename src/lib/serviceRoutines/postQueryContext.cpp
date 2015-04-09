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

#include "common/string.h"
#include "mongoBackend/mongoQueryContext.h"
#include "ngsi/ParseData.h"
#include "ngsi10/QueryContextResponse.h"
#include "ngsi10/QueryContextRequestVector.h"
#include "ngsi10/QueryContextResponse.h"
#include "ngsi10/QueryContextResponseVector.h"
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
* queryForward - 
*
* An entity/attribute has been found on some context provider.
* We need to forward the query request to the context provider, indicated in qcrsP->contextProvider
*
* 1. Parse the providing application to extract IP, port and URI-path
* 2. Render an XML-string of the request we want to forward
* 3. Send the request to the providing application (and await the response)
* 4. Parse the XML response and fill in a binary QueryContextResponse
* 5. Render QueryContextResponse from the providing application
*
*/
static void queryForward(ConnectionInfo* ciP, QueryContextRequest* qcrP, QueryContextResponse* qcrsP)
{
  std::string     ip;
  std::string     protocol;
  int             port;
  std::string     prefix;
  std::string     answer;


  //
  // 1. Parse the providing application to extract IP, port and URI-path
  //
  if (parseUrl(qcrP->contextProvider, ip, port, prefix, protocol) == false)
  {
    LM_W(("Bad Input (invalid providing application '%s')", qcrP->contextProvider.c_str()));
    //  Somehow, if we accepted this providing application, it is the brokers fault ...
    //  SccBadRequest should have been returned before, when it was registered!

    qcrsP->errorCode.fill(SccContextElementNotFound, "");
    return;
  }


  //
  // 2. Render an XML-string of the request we want to forward
  //
  std::string  payload = qcrP->render(QueryContext, XML, "");
  char*        cleanPayload;

  if ((cleanPayload = xmlPayloadClean(payload.c_str(), "<queryContextRequest>")) == NULL)
  {
    LM_E(("Runtime Error (error rendering forward-request)"));
    qcrsP->errorCode.fill(SccContextElementNotFound, "");
    return;
  }


  //
  // 3. Send the request to the Context Provider (and await the reply)
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
    qcrsP->errorCode.fill(SccContextElementNotFound, "");
    LM_E(("Runtime Error (error forwarding 'Query' to providing application)"));
    return;
  }


  //
  // 5. Parse the XML response and fill in a binary QueryContextResponse
  //
  std::string  s;
  std::string  errorMsg;

  cleanPayload = xmlPayloadClean(out.c_str(), "<queryContextResponse>");

  if ((cleanPayload == NULL) || (cleanPayload[0] == 0))
  {
    //
    // This is really an internal error in the Context Provider
    // It is not in the orion broker though, so 404 is returned
    //
    LM_W(("Other Error (context provider response to QueryContext is empty)"));
    qcrsP->errorCode.fill(SccContextElementNotFound, "invalid context provider response");
    return;
  }

  //
  // NOTE
  // When coming from a convenience operation, such as GET /v1/contextEntities/EID/attributes/attrName,
  // the verb/method in ciP is GET. However, the parsing function expects a POST, as if it came from a 
  // POST /v1/queryContext. 
  // So, here we change the verb/method for POST.
  //
  ParseData parseData;

  ciP->verb   = POST;
  ciP->method = "POST";

  s = xmlTreat(cleanPayload, ciP, &parseData, RtQueryContextResponse, "queryContextResponse", NULL, &errorMsg);
  if (s != "OK")
  {
    LM_W(("Internal Error (error parsing reply from prov app: %s)", errorMsg.c_str()));
    qcrsP->errorCode.fill(SccContextElementNotFound, "");
    return;
  }


  //
  // 6. Render QueryContextResponse from the providing application
  //
  char portV[16];
  snprintf(portV, sizeof(portV), "%d", port);

  // Fill in the response from the redirection into the response
  qcrsP->fill(&parseData.qcrs.res);
  qcrsP->present("", "queryForward, after filling qcrsP");

  //
  // 'Fixing' StatusCode
  //
  if (qcrsP->errorCode.code == SccNone)
  {
    qcrsP->errorCode.fill(SccOk);
  }

  if ((qcrsP->contextElementResponseVector.size() == 1) && (qcrsP->contextElementResponseVector[0]->statusCode.code == SccContextElementNotFound))
  {
    qcrsP->errorCode.fill(SccContextElementNotFound);
  }
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
  QueryContextResponse*       qcrsP = &parseDataP->qcrs.res;
  std::string                 answer;
  int                         noOfForwards = 0;
  QueryContextRequestVector   requestV;
  QueryContextResponseVector  responseV;

  qcrsP->errorCode.fill(SccOk);
  ciP->httpStatusCode = mongoQueryContext(&parseDataP->qcr.res, qcrsP, ciP->tenant, ciP->servicePathV, ciP->uriParam);
  qcrsP->present("", "postQueryContext");

  for (unsigned int ix = 0 ; ix < qcrsP->contextElementResponseVector.size(); ++ix)
  {
    ContextElementResponse* cerP       = qcrsP->contextElementResponseVector[ix]->clone();

    for (unsigned int aIx = 0 ; aIx < cerP->contextElement.contextAttributeVector.size(); ++aIx)
    {
      EntityId*            eP       = &cerP->contextElement.entityId;
      ContextAttribute*    aP       = cerP->contextElement.contextAttributeVector[aIx];
      QueryContextRequest* requestP = requestV.lookup(aP->providingApplication, eP);   // No need to lookup if providingApplication == "" ...
      
      //
      // An empty providingApplication means the attribute is local
      // In such a case, the response is already in our hand, we just need to forward it to responseV
      //
      if (aP->providingApplication == "")
      {
        if (aP->found == false)
        {
          continue;  // Non-found pairs of entity/attribute are thrown away
        }

        QueryContextResponse* qP = new QueryContextResponse(eP, aP);
        responseV.push_back(qP);
      }
      else if (requestP == NULL)
      {
        requestP = new QueryContextRequest(aP->providingApplication, eP, aP->name);
        requestV.push_back(requestP);
        ++noOfForwards;
      }
      else
      {
        requestP->attributeList.push_back_if_absent(aP->name);
        requestP->entityIdVector.push_back_if_absent(eP);
      }
    }
  }

  //
  // If no redirectioning is necessary, just return the result
  //
  if (noOfForwards == 0)
  {
    //
    // In case the response is empty, fill response with the entity we looked for
    //
    if (qcrsP->contextElementResponseVector.size() == 0)
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
      qcrsP->errorCode.fill(SccContextElementNotFound);
    }

    answer = qcrsP->render(ciP, QueryContext, "");
    return answer;
  }


  //
  // DEBUG - present the vector prepared for Context Provider Forwarding
  //
  requestV.present();


  //
  // Now, send 'noOfForwards' Query requests, each in a separate thread and
  // await all the responses.
  // Actually, if there is only ONE forward to be done then there is no reason to
  // do the forward in a separate shell. Better to do it inside the current thread.
  //
  // If providingApplication is empty then that part of the query has been performed already, locally.
  // 
  //
  for (unsigned int fIx = 0; fIx < requestV.size(); ++fIx)
  {
    if (requestV[fIx]->contextProvider == "")
    {
      //
      // No forward, we already have the result in responseV
      //
      continue;
    }

    QueryContextResponse* qP = new QueryContextResponse();
    queryForward(ciP, requestV[fIx], qP);
    responseV.push_back(qP);
  }

  responseV.present();

  //
  // Is qcrsAccumulatedP OK? Without doing anything special here ... ?
  //
  answer = responseV.render(ciP, "");

  // requestV.release();

  return answer;
}
