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
#include "common/globals.h"
#include "common/statistics.h"
#include "common/clockFunctions.h"
#include "alarmMgr/alarmMgr.h"

#include "logMsg/traceLevels.h"

#include "mongoBackend/mongoQueryContext.h"
#include "ngsi/ParseData.h"
#include "ngsi10/QueryContextRequest.h"
#include "ngsi10/QueryContextResponse.h"
#include "orionTypes/QueryContextRequestVector.h"
#include "orionTypes/QueryContextResponseVector.h"
#include "rest/ConnectionInfo.h"
#include "rest/httpRequestSend.h"
#include "rest/uriParamNames.h"
#include "rest/OrionError.h"
#include "serviceRoutines/postQueryContext.h"
#include "jsonParse/jsonRequest.h"



/* ****************************************************************************
*
* jsonPayloadClean -
*/
static char* jsonPayloadClean(const char* payload)
{
  return (char*) strstr(payload, "{");
}



/* ****************************************************************************
*
* queryForward -
*
* An entity/attribute has been found on some context provider.
* We need to forward the query request to the context provider, indicated in qcrsP->contextProvider
*
* 1. Parse the providing application to extract IP, port and URI-path
* 2. Render the string of the request we want to forward
* 3. Send the request to the providing application (and await the response)
* 4. Parse the response and fill in a binary QueryContextResponse
* 5. Fill in the response from the redirection into the response of this function
* 6. 'Fix' StatusCode
* 7. Freeing memory
*
*/
static bool queryForward(ConnectionInfo* ciP, QueryContextRequest* qcrP, QueryContextResponse* qcrsP)
{
  std::string     ip;
  std::string     protocol;
  int             port;
  std::string     prefix;

  //
  // 1. Parse the providing application to extract IP, port and URI-path
  //
  if (parseUrl(qcrP->contextProvider, ip, port, prefix, protocol) == false)
  {
    std::string details = std::string("invalid providing application '") + qcrP->contextProvider + "'";

    alarmMgr.badInput(clientIp, details);

    //
    //  Somehow, if we accepted this providing application, it is the brokers fault ...
    //  SccBadRequest should have been returned before, when it was registered!
    //
    return false;
  }


  //
  // 2. Render the string of the request we want to forward
  //
  std::string  payload;
  TIMED_RENDER(payload = qcrP->render());

  char* cleanPayload = (char*) payload.c_str();;

  //
  // 3. Send the request to the Context Provider (and await the reply)
  // FIXME P7: Should Rush be used?
  //
  std::string     verb         = "POST";
  std::string     resource     = prefix + "/queryContext";
  std::string     tenant       = ciP->tenant;
  std::string     servicePath  = (ciP->httpHeaders.servicePathReceived == true)? ciP->httpHeaders.servicePath : "";
  std::string     mimeType     = "application/json";
  std::string     out;
  int             r;

  LM_T(LmtCPrForwardRequestPayload, ("forward queryContext request payload: %s", payload.c_str()));

  std::map<std::string, std::string> noHeaders;
  r = httpRequestSend(ip,
                      port,
                      protocol,
                      verb,
                      tenant,
                      servicePath,
                      ciP->httpHeaders.xauthToken,
                      resource,
                      mimeType,
                      payload,
                      ciP->httpHeaders.correlator,
                      "",
                      false,
                      true,
                      &out,
                      noHeaders,
                      mimeType);

  if (r != 0)
  {
    LM_W(("Runtime Error (error forwarding 'Query' to providing application)"));
    return false;
  }

  LM_T(LmtCPrForwardRequestPayload, ("forward queryContext response payload: %s", out.c_str()));


  //
  // 4. Parse the response and fill in a binary QueryContextResponse
  //
  std::string  s;
  std::string  errorMsg;


  cleanPayload = jsonPayloadClean(out.c_str());

  if ((cleanPayload == NULL) || (cleanPayload[0] == 0))
  {
    //
    // This is really an internal error in the Context Provider
    // It is not in the orion broker though, so 404 is returned
    //
    LM_W(("Other Error (context provider response to QueryContext is empty)"));
    return false;
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

  // Note that jsonTreat() is thought for client-to-CB interactions, thus it modifies ciP->httpStatusCode
  // Thus, we need to preserve it before (and recover after) in order a fail in the CB-to-CPr interaction doesn't
  // "corrupt" the status code in the client-to-CB interaction.
  // FIXME P5: not sure if I like this approach... very "hacking-style". Probably it would be better
  // to make JSON parsing logic (internal to jsonTreat()) independent of ciP (in fact, parsing process
  // hasn't anything to do with connection).
  HttpStatusCode sc = ciP->httpStatusCode;
  s = jsonTreat(cleanPayload, ciP, &parseData, RtQueryContextResponse, "queryContextResponse", NULL);
  ciP->httpStatusCode = sc;

  if (s != "OK")
  {
    LM_W(("Internal Error (error parsing reply from prov app: %s)", errorMsg.c_str()));
    parseData.qcr.res.release();
    parseData.qcrs.res.release();
    return false;
  }


  //
  // 5. Fill in the response from the redirection into the response of this function
  //
  qcrsP->fill(&parseData.qcrs.res);


  //
  // 6. 'Fix' StatusCode
  //
  if (qcrsP->errorCode.code == SccNone)
  {
    qcrsP->errorCode.fill(SccOk);
  }


  //
  // 7. Freeing memory
  //
  parseData.qcr.res.release();
  parseData.qcrs.res.release();

  return true;
}



/* ****************************************************************************
*
* forwardsPending -
*/
static bool forwardsPending(QueryContextResponse* qcrsP)
{
  for (unsigned int ix = 0 ; ix < qcrsP->contextElementResponseVector.size(); ++ix)
  {
    ContextElementResponse* cerP  = qcrsP->contextElementResponseVector[ix];

    if (cerP->contextElement.providingApplicationList.size() != 0)
    {
      return true;
    }

    for (unsigned int aIx = 0 ; aIx < cerP->contextElement.contextAttributeVector.size(); ++aIx)
    {
      ContextAttribute* aP  = cerP->contextElement.contextAttributeVector[aIx];

      if (aP->providingApplication.get() != "")
      {
        return true;
      }
    }
  }

  return false;
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
  // So, the digital response is passed back in parseDataP->qcrs.res.
  //
  QueryContextResponse*       qcrsP = &parseDataP->qcrs.res;
  QueryContextRequest*        qcrP  = &parseDataP->qcr.res;
  std::string                 answer;
  QueryContextRequestVector   requestV;
  QueryContextResponseVector  responseV;
  long long                   count = 0;
  long long*                  countP = NULL;

  bool asJsonObject = (ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object" && ciP->outMimeType == JSON);

  //
  // 00. Count or not count? That is the question ...
  //
  // For API version 1, if the URI parameter 'details' is set to 'on', then the total of local
  // entities is returned in the errorCode of the payload.
  //
  // In API version 2, this has changed completely. Here, the total count of local entities is returned
  // if the URI parameter 'count' is set to 'true', and it is returned in the HTTP header Fiware-Total-Count.
  //
  if ((ciP->apiVersion == V2) && (ciP->uriParamOptions["count"]))
  {
    countP = &count;
  }
  else if ((ciP->apiVersion == V1) && (ciP->uriParam["details"] == "on"))
  {
    countP = &count;
  }



  //
  // 01. Call mongoBackend/mongoQueryContext
  //
  qcrsP->errorCode.fill(SccOk);

  TIMED_MONGO(ciP->httpStatusCode = mongoQueryContext(qcrP,
                                                      qcrsP,
                                                      ciP->tenant,
                                                      ciP->servicePathV,
                                                      ciP->uriParam,
                                                      ciP->uriParamOptions,
                                                      countP,
                                                      ciP->apiVersion));

  if (qcrsP->errorCode.code == SccBadRequest)
  {
    // Bad Input detected by Mongo Backend - request ends here !
    OrionError oe(qcrsP->errorCode);

    TIMED_RENDER(answer = oe.render());
    qcrP->release();
    return answer;
  }


  //
  // If API version 2, add count, if asked for, in HTTP header Fiware-Total-Count
  //
  if ((ciP->apiVersion == V2) && (countP != NULL))
  {
    char cV[32];

    snprintf(cV, sizeof(cV), "%llu", *countP);
    ciP->httpHeader.push_back("Fiware-Total-Count");
    ciP->httpHeaderValue.push_back(cV);
  }



  //
  // 02. Normal case (no requests to be forwarded)
  //
  // If the result from mongoBackend is a 'simple' result without any forwarding needed (this is the
  // normal case), then there is no need to execute the special treatment that forwarding needs.
  // It's better not to mix the two very different treatments of the output from mongoBackend.
  //
  // Now, the request is 'simple' if all providingApplicationLists of the ContextElements are empty and
  // no ContextAttribute has any providingApplication.
  //
  if (forwardsPending(qcrsP) == false)
  {
    TIMED_RENDER(answer = qcrsP->render(ciP->apiVersion, asJsonObject));

    qcrP->release();
    return answer;
  }


  //
  // 03. Complex case (queries to be forwarded)
  //
  // In this loop, the output from mongoQueryContext is examined and the attributes are sorted
  // by their providing application, in requestV, later to be used to forward the queries to their
  // respective Context Providers.
  //
  // The local part of the query is already taken care of by mongoQueryContext and the result must be moved
  // to the response vector.
  // All the local response will be gathered in one single instance of QueryContextResponse.
  // As a "QueryContextResponse::ContextElementResponse::ContextElement" can contain only ONE entity,
  // we will need a separate ContextElementResponse for each distinct EntityId in the local response.
  // All attributes that belong to the same EntityId will be gathered in one unique instance of
  // ContextElementResponse - so, the correct ContextElementResponse must be looked up and if not found,
  // it must be created and added to the QueryContextResponse of the local response
  //
  QueryContextResponse* localQcrsP = new QueryContextResponse();

  for (unsigned int ix = 0 ; ix < qcrsP->contextElementResponseVector.size(); ++ix)
  {
    ContextElementResponse* cerP  = qcrsP->contextElementResponseVector[ix];
    EntityId*               eP    = &cerP->contextElement.entityId;

    //
    // If a Context Provider has been registered with an empty attribute list for
    // the EntityId in this ContextElement, then the ContextElement has this Context Provider
    // in the providingApplicationList after the call to mongoQueryContext.
    //
    // When there is a Context Provider in ContextElement::providingApplicationList, then the
    // request must be sent to that Context Provider also
    //
    for (unsigned int ix = 0; ix < cerP->contextElement.providingApplicationList.size(); ++ix)
    {
      QueryContextRequest* requestP;

      requestP = new QueryContextRequest(cerP->contextElement.providingApplicationList[ix].get(), eP, qcrP->attributeList);
      requestV.push_back(requestP);
    }

    //
    // What if the Attribute Vector of the ContextElementResponse is empty?
    // For now, just push it into localQcrsP, but only if its local, i.e. its contextElement.providingApplicationList is empty
    //
    if ((cerP->contextElement.contextAttributeVector.size() == 0) && (cerP->contextElement.providingApplicationList.size() == 0))
    {
      localQcrsP->contextElementResponseVector.push_back(new ContextElementResponse(eP, NULL));
    }
    else
    {
      for (unsigned int aIx = 0; aIx < cerP->contextElement.contextAttributeVector.size(); ++aIx)
      {
        ContextAttribute*    aP  = cerP->contextElement.contextAttributeVector[aIx];

        //
        // An empty providingApplication means the attribute is local
        // In such a case, the response is already in our hand, we just need to copy it to responseV
        //
        if (aP->providingApplication.get() == "")
        {
          if (aP->found == false)
          {
            continue;  // Non-found pairs of entity/attribute are thrown away
          }


          //
          // So, where can we put this attribute?
          // If we find a suitable existing contextElementResponse, we put it there,
          // otherwise, we have to create a new contextElementResponse.
          //
          ContextElementResponse* contextElementResponseP = localQcrsP->contextElementResponseVector.lookup(eP);

          if (contextElementResponseP == NULL)
          {
            contextElementResponseP = new ContextElementResponse(eP, aP);
            localQcrsP->contextElementResponseVector.push_back(contextElementResponseP);
          }
          else
          {
            contextElementResponseP->contextElement.contextAttributeVector.push_back(new ContextAttribute(aP));
          }

          continue;
        }


        //
        // Not a local attribute - aP->providingApplication is not empty
        //
        QueryContextRequest* requestP = requestV.lookup(aP->providingApplication.get(), eP);

        if (requestP == NULL)
        {
          requestP = new QueryContextRequest(aP->providingApplication.get(), eP, aP->name);
          requestV.push_back(requestP);
        }
        else
        {
          EntityId* entityP = new EntityId(eP);
          bool      pushed;

          requestP->attributeList.push_back_if_absent(aP->name);

          pushed = requestP->entityIdVector.push_back_if_absent(entityP);
          if (pushed == false)
          {
            entityP->release();
            delete entityP;
          }
        }
      }
    }
  }

  //
  // Any local results in localQcrsP?
  //
  // If so, localQcrsP must be pushed to the response vector 'responseV'.
  //
  if (localQcrsP->contextElementResponseVector.size() != 0)
  {
    responseV.push_back(localQcrsP);
  }
  else
  {
    delete localQcrsP;
  }


  //
  // Now, forward the Query requests, each in a separate thread (to be implemented) and
  // await all the responses.
  // Actually, if there is only ONE forward to be done then there is no reason to
  // do the forward in a separate shell. Better to do it inside the current thread.
  //
  // If providingApplication is empty then that part of the query has been performed already, locally.
  //
  //
  QueryContextResponse* qP;

  for (unsigned int fIx = 0; fIx < requestV.size() && fIx < cprForwardLimit; ++fIx)
  {
    if (requestV[fIx]->contextProvider == "")
    {
      LM_E(("Internal Error (empty context provider string)"));
      continue;
    }

    qP = new QueryContextResponse();
    qP->errorCode.fill(SccOk);

    if (queryForward(ciP, requestV[fIx], qP) == true)
    {
      //
      // Each ContextElementResponse of qP should be tested to see whether there
      // is already an existing ContextElementResponse in responseV
      //
      responseV.push_back(qP);
    }
    else
    {
      qP->errorCode.fill(SccContextElementNotFound, "invalid context provider response");
      responseV.push_back(qP);
    }
  }

  std::string detailsString  = ciP->uriParam[URI_PARAM_PAGINATION_DETAILS];
  bool        details        = (strcasecmp("on", detailsString.c_str()) == 0)? true : false;

  TIMED_RENDER(answer = responseV.render(ciP->apiVersion, asJsonObject, details, qcrsP->errorCode.details));


  //
  // Time to cleanup.
  // But before doing that ...
  // Some Convops calling this routine need the response in digital.
  // The response is returned in parseDataP->qcrs.res (qcrsP).
  // Right now, we have the response in responseV, so we have to migrate it
  // from a vector of QueryContextResponse into one single QueryContextResponse.
  // QueryContextResponseVector has a method 'populate' to do just that.
  // Before populating qcrsP with what's in responseV, qcrsP must be cleaned so that
  // we don't leak any memory.
  //
  qcrsP->release();
  responseV.populate(qcrsP);

  qcrP->release();
  requestV.release();
  responseV.release();

  return answer;
}
