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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "mongoBackend/mongoQueryContext.h"
#include "ngsi/ParseData.h"
#include "ngsi10/QueryContextRequest.h"
#include "ngsi10/QueryContextResponse.h"
#include "orionTypes/QueryContextRequestVector.h"
#include "orionTypes/QueryContextResponseVector.h"
#include "rest/ConnectionInfo.h"
#include "rest/HttpHeaders.h"
#include "rest/httpRequestSend.h"
#include "rest/uriParamNames.h"
#include "rest/OrionError.h"
#include "serviceRoutines/postQueryContext.h"
#include "jsonParse/jsonRequest.h"
#include "jsonParseV2/parseEntitiesResponse.h"



/* ****************************************************************************
*
* jsonPayloadClean -
*/
static char* jsonPayloadClean(const char* payload)
{
  //
  // After HTTP headers comes an empty line.
  // After this empty line comes the payload.
  // This function returns a pointer to the first byte after an empty line, if found
  // This "first byte" is the first byte of the payload
  //
  while (*payload != 0)
  {
    if (*payload == '\n')  // One newline is found
    {
      if (payload[1] == '\n')  // And the second one - we have found the start of the payload
        return (char*) &payload[2];
      if ((payload[1] == '\r') && (payload[2] == '\n'))  // "windows style newline with \r\n"
        return (char*) &payload[3];
    }

    ++payload;
  }

  return NULL;
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
static bool queryForward
(
  ConnectionInfo*        ciP,
  QueryContextRequest*   qcrP,
  const std::string&     regId,
  unsigned int           correlatorCounter,
  QueryContextResponse*  qcrsP
)
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
  // 2. Prepare the request to forward
  //    - If V1: Render the payload
  //    - If V2: Setup the URI params
  //
  std::string     payload;
  std::string     verb;
  std::string     resource;
  std::string     tenant       = ciP->tenant;
  std::string     servicePath  = (ciP->httpHeaders.servicePathReceived == true)? ciP->httpHeaders.servicePath : "";
  std::string     mimeType;
  std::string     op;

  if (qcrP->providerFormat == PfJson)
  {
    op        = "/queryContext";
    TIMED_RENDER(payload = qcrP->toJsonV1());
  }
  else
  {
    op        = "/op/query";
    TIMED_RENDER(payload = qcrP->toJson());
#if 0
    // FIXME #3485: this part is not removed by the moment, in the case it may be useful in the
    // context of issue #3485

    //
    // NGSIv2 forward: instead of payload, URI params are used
    //
    std::string  extraParams;

    verb      = "GET";
    resource  = prefix + "/entities";

    //
    // FIXME #3068: For requests where the type comes in the payload (batch op + NGSIv1), we'd need to add an else part
    //              to extract the type from the incoming payload
    //
    if (!ciP->uriParam["type"].empty())
    {
      extraParams += "&type=";
      extraParams += ciP->uriParam["type"];
    }

    if (qcrP->entityIdVector.size() > 0)
    {
      //
      // A few remarks about the list of entity ids:
      //   - If more than one ID is present, we just make a comma-separated list of them
      //   - We can't allow mixes between id and idPattern
      //   - There can only be ONE idPattern (lists aren't supported for idPatterns)
      //   - If an idPattern is present and equal to ".*", we can simply ignore it - .* matches ALL entity ids
      //
      if ((qcrP->entityIdVector.size() == 1) && (qcrP->entityIdVector[0]->isPattern == "true"))
      {
        //
        // This is the only case where isPattern is allowed to be true - ONE entity in qcrP->entityIdVector
        // In all other places (see below) isPattern set to TRUE is an error (that is silently ignored)
        //

        // If the idPattern is '.*', then no need to add it to the query
        if (qcrP->entityIdVector[0]->id != ".*")
        {
          extraParams += "&idPattern=";
          extraParams += qcrP->entityIdVector[0]->id;
        }
      }
      else
      {
        extraParams += "&id=";

        for (unsigned int ix = 0; ix < qcrP->entityIdVector.size(); ix++)
        {
          if (qcrP->entityIdVector[ix]->isPattern == "false")  // Silently ignored if "true"
          {
            if (ix != 0)
            {
              extraParams += ",";
            }

            extraParams += qcrP->entityIdVector[ix]->id;
          }
        }
      }
    }

    if (!ciP->uriParam["attrs"].empty())
    {
      extraParams += "&attrs=";
      extraParams += ciP->uriParam["attrs"];
    }
    else if (qcrP->attributeList.size() != 0)
    {
      extraParams += "&attrs=";
      extraParams += qcrP->attributeList[0];

      for (unsigned int ix = 1; ix < qcrP->attributeList.size(); ix++)
      {
        extraParams += ",";
        extraParams += qcrP->attributeList[ix];
      }
    }

    if (!extraParams.empty())
    {
      char* xParams = (char*) &(extraParams.c_str())[1];  // Remove first '&'

      resource += "?";
      resource += xParams;
    }
#endif
  }

  verb      = "POST";
  resource  = prefix + op;
  mimeType  = "application/json";

  //
  // 3. Send the request to the Context Provider (and await the reply)
  //
  std::string     out;
  int             r;

  LM_T(LmtCPrForwardRequestPayload, ("Forward Query: %s %s: %s", verb.c_str(), resource.c_str(), payload.c_str()));

  std::map<std::string, std::string>  noHeaders;
  long long                           statusCode; // not used by the moment

  char suffix[STRING_SIZE_FOR_INT];
  snprintf(suffix, sizeof(suffix), "%u", correlatorCounter);
  std::string effectiveCorrelator = ciP->httpHeaders.correlator + "; cbfwd=" + suffix;

  r = httpRequestSend(fromIp,  // thread variable
                      ip,
                      port,
                      protocol,
                      verb,
                      tenant,
                      servicePath,
                      ciP->httpHeaders.xauthToken,
                      resource,
                      mimeType,
                      payload,
                      effectiveCorrelator,
                      "",
                      &out,
                      &statusCode,
                      noHeaders,
                      mimeType);

  if (r != 0)
  {
    LM_E(("Runtime Error (error '%s' forwarding 'Query' to providing application)", out.c_str()));
    LM_I(("Request forwarded (regId: %s): %s %s%s, request payload (%d bytes): %s, response payload (0 bytes): , response code: %s",
          regId.c_str(), verb.c_str(), qcrP->contextProvider.c_str(), op.c_str(), payload.length(), payload.c_str(), out.c_str()));

    return false;
  }

  LM_T(LmtCPrForwardResponsePayload, ("forward queryContext response payload: %s", out.c_str()));

  //
  // 4. Parse the response and fill in a binary QueryContextResponse
  //
  ParseData parseData;
  char*     cleanPayload = jsonPayloadClean(out.c_str());

  if ((cleanPayload == NULL) || (cleanPayload[0] == 0))
  {
    //
    // This is really an internal error in the Context Provider
    // It is not in the orion broker though, so 404 is returned
    //
    LM_W(("Other Error (context provider response to QueryContext is empty)"));
    return false;
  }

  LM_I(("Request forwarded (regId: %s): %s %s%s, request payload (%d bytes): %s, response payload (%d bytes): %s, response code: %d",
        regId.c_str(), verb.c_str(), qcrP->contextProvider.c_str(), op.c_str(), payload.length(), payload.c_str(), strlen(cleanPayload), cleanPayload, statusCode));

  if (qcrP->providerFormat == PfJson)
  {
    std::string  s;
    std::string  errorMsg;

    //
    // NOTE
    // When coming from a convenience operation, such as GET /v1/contextEntities/EID/attributes/attrName,
    // the verb/method in ciP is GET. However, the parsing function expects a POST, as if it came from a
    // POST /v1/queryContext.
    // So, here we change the verb/method for POST.
    //
    ciP->verb   = POST;
    ciP->method = "POST";

    // Note that jsonTreat() is thought for client-to-CB interactions, thus it modifies ciP->httpStatusCode
    // Thus, we need to preserve it before (and recover after) in order a fail in the CB-to-CPr interaction doesn't
    // "corrupt" the status code in the client-to-CB interaction.
    // FIXME P5: not sure if I like this approach... very "hacking-style". Probably it would be better
    // to make JSON parsing logic (internal to jsonTreat()) independent of ciP (in fact, parsing process
    // hasn't anything to do with connection).
    HttpStatusCode sc = ciP->httpStatusCode;
    s = jsonTreat(cleanPayload, ciP, &parseData, RtQueryContextResponse, NULL);
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
  }
  else
  {
    bool                        b;
    Entities                    entities;
    OrionError                  oe;

    // Note that parseEntitiesResponse() is thought for client-to-CB interactions, so it takes into account
    // ciP->uriParamOptions[OPT_KEY_VALUES]. In this case, we never use keyValues in the CB-to-CPr so we
    // set to false and restore its original value later. In this case it seems it is not needed to preserve
    // ciP->httpStatusCode as in the similar case above
    // FIXME P5: not sure if I like this approach... very "hacking-style". Probably it would be better
    // to make JSON parsing logic (internal to parseEntitiesResponse()) independent of ciP and to pass the
    // keyValue directly as function parameter.
    bool previousKeyValues = ciP->uriParamOptions[OPT_KEY_VALUES];
    ciP->uriParamOptions[OPT_KEY_VALUES] = false;
    b = parseEntitiesResponse(ciP, cleanPayload, &entities, &oe);
    ciP->uriParamOptions[OPT_KEY_VALUES] = previousKeyValues;

    if (b == false)
    {
      LM_W(("Internal Error (error parsing reply from context provider: %s)", oe.details.c_str()));
      parseData.qcr.res.release();
      parseData.qcrs.res.release();
      return false;
    }

    //
    // 5. Fill in the response from the redirection into the response of this function
    //
    if (entities.size() > 0)
    {
      qcrsP->fill(entities);
    }
    else
    {
      qcrsP->errorCode.fill(SccContextElementNotFound);
    }
  }

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

    if (cerP->entity.providingApplicationList.size() != 0)
    {
      return true;
    }

    for (unsigned int aIx = 0 ; aIx < cerP->entity.attributeVector.size(); ++aIx)
    {
      ContextAttribute* aP  = cerP->entity.attributeVector[aIx];

      if (!aP->providingApplication.get().empty())
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
  std::vector<std::string>    regIdsV;
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

    TIMED_RENDER(answer = oe.toJsonV1());
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
    ciP->httpHeader.push_back(HTTP_FIWARE_TOTAL_COUNT);
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
    TIMED_RENDER(answer = qcrsP->toJsonV1(asJsonObject));

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
    ContextElementResponse*  cerP  = qcrsP->contextElementResponseVector[ix];
    EntityId                 en(cerP->entity.id, cerP->entity.type, cerP->entity.isPattern);

    //
    // If a Context Provider has been registered with an empty attribute list for
    // the EntityId in this ContextElement, then the ContextElement has this Context Provider
    // in the providingApplicationList after the call to mongoQueryContext.
    //
    // When there is a Context Provider in ContextElement::providingApplicationList, then the
    // request must be sent to that Context Provider also
    //
    for (unsigned int ix = 0; ix < cerP->entity.providingApplicationList.size(); ++ix)
    {
      QueryContextRequest* requestP;

      requestP = new QueryContextRequest(cerP->entity.providingApplicationList[ix].get(), &en, qcrP->attributeList, cerP->entity.providingApplicationList[ix].providerFormat);
      requestV.push_back(requestP);
    }

    //
    // What if the Attribute Vector of the ContextElementResponse is empty?
    // For now, just push it into localQcrsP, but only if its local, i.e. its contextElement.providingApplicationList is empty
    //
    if ((cerP->entity.attributeVector.size() == 0) && (cerP->entity.providingApplicationList.size() == 0))
    {
      localQcrsP->contextElementResponseVector.push_back(new ContextElementResponse(&en, NULL));
    }
    else
    {
      for (unsigned int aIx = 0; aIx < cerP->entity.attributeVector.size(); ++aIx)
      {
        ContextAttribute* aP  = cerP->entity.attributeVector[aIx];

        //
        // An empty providingApplication means the attribute is local
        // In such a case, the response is already in our hand, we just need to copy it to responseV
        //
        if (aP->providingApplication.get().empty())
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
          ContextElementResponse* contextElementResponseP = localQcrsP->contextElementResponseVector.lookup(&cerP->entity);

          if (contextElementResponseP == NULL)
          {
            contextElementResponseP = new ContextElementResponse(&en, aP);
            localQcrsP->contextElementResponseVector.push_back(contextElementResponseP);
          }
          else
          {
            contextElementResponseP->entity.attributeVector.push_back(new ContextAttribute(aP));
          }

          continue;
        }


        //
        // Not a local attribute - aP->providingApplication is not empty
        //
        QueryContextRequest* requestP = requestV.lookup(aP->providingApplication.get(), &en);

        if (requestP == NULL)
        {
          requestP = new QueryContextRequest(aP->providingApplication.get(), &en, aP->name, aP->providingApplication.providerFormat);
          requestV.push_back(requestP);
          regIdsV.push_back(aP->providingApplication.getRegId());
        }
        else
        {
          EntityId* entityP = new EntityId(&en);
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

  // Note that queryForward() (due to internal calls to httpRequestSend())
  // change coordid= and transid= so we need to preserve them and restore once fowarding loop has ended
  // FIXME P5: maybe this is not the right place to store&recover old transaction.
  // What about inside httpRequestSend?

  std::string prevCoorId  = correlationIdGet();
  std::string prevTransId = transactionIdGetAsString();

  for (unsigned int fIx = 0; fIx < requestV.size() && fIx < cprForwardLimit; ++fIx)
  {
    if (requestV[fIx]->contextProvider.empty())
    {
      LM_E(("Internal Error (empty context provider string)"));
      continue;
    }

    qP = new QueryContextResponse();
    qP->errorCode.fill(SccOk);

    if (queryForward(ciP, requestV[fIx], regIdsV[fIx], fIx + 1, qP) == true)
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

  correlatorIdSet(prevCoorId.c_str());
  transactionIdSet(prevTransId.c_str());

  std::string detailsString  = ciP->uriParam[URI_PARAM_PAGINATION_DETAILS];
  bool        details        = (strcasecmp("on", detailsString.c_str()) == 0)? true : false;

  TIMED_RENDER(answer = responseV.toJsonV1(asJsonObject, details, qcrsP->errorCode.details));


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
