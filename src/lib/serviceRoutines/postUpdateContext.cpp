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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/string.h"
#include "common/defaultValues.h"
#include "common/globals.h"
#include "common/errorMessages.h"
#include "common/statistics.h"
#include "common/clockFunctions.h"
#include "alarmMgr/alarmMgr.h"

#include "jsonParse/jsonRequest.h"
#include "mongoBackend/mongoUpdateContext.h"
#include "ngsi/ParseData.h"
#include "ngsi10/UpdateContextResponse.h"
#include "orionTypes/UpdateContextRequestVector.h"
#include "rest/ConnectionInfo.h"
#include "rest/httpRequestSend.h"
#include "rest/uriParamNames.h"
#include "serviceRoutines/postUpdateContext.h"



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
* forwardsPending -
*/
static bool forwardsPending(UpdateContextResponse* upcrsP)
{
  for (unsigned int cerIx = 0; cerIx < upcrsP->contextElementResponseVector.size(); ++cerIx)
  {
    ContextElementResponse* cerP  = upcrsP->contextElementResponseVector[cerIx];

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
* updateForward -
*
* An entity/attribute has been found on some context provider.
* We need to forward the update request to the context provider, indicated in upcrsP->contextProvider
*
* 1. Parse the providing application to extract IP, port and URI-path
* 2. Render the string of the request we want to forward
* 3. Send the request to the providing application (and await the response)
* 4. Parse the response and fill in a binary UpdateContextResponse
* 5. Fill in the response from the redirection into the response of this function
* 6. 'Fix' StatusCode
* 7. Freeing memory
*
*/
static void updateForward(ConnectionInfo* ciP, UpdateContextRequest* upcrP, UpdateContextResponse* upcrsP)
{
  std::string      ip;
  std::string      protocol;
  int              port;
  std::string      prefix;

  bool asJsonObject = (ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object" && ciP->outMimeType == JSON);

  //
  // 1. Parse the providing application to extract IP, port and URI-path
  //
  if (parseUrl(upcrP->contextProvider, ip, port, prefix, protocol) == false)
  {
    std::string details = std::string("invalid providing application '") + upcrP->contextProvider + "'";

    alarmMgr.badInput(clientIp, details);

    //
    //  Somehow, if we accepted this providing application, it is the brokers fault ...
    //  SccBadRequest should have been returned before, when it was registered!
    //
    upcrsP->errorCode.fill(SccContextElementNotFound, "");
    return;
  }


  //
  // 2. Render the string of the request we want to forward
  //
  MimeType     outMimeType = ciP->outMimeType;
  std::string  payload;
  char*        cleanPayload;

  ciP->outMimeType  = JSON;

  //
  // FIXME: Forwards are done using NGSIv1 only, for now
  //        This will hopefully change soon ...
  //        Once we implement forwards in NGSIv2, this render() should be like this:
  //        TIMED_RENDER(payload = upcrP->render(ciP->apiVersion, asJsonObject, ""));
  //
  TIMED_RENDER(payload = upcrP->render(asJsonObject));

  ciP->outMimeType  = outMimeType;
  cleanPayload      = (char*) payload.c_str();

  //
  // 3. Send the request to the Context Provider (and await the reply)
  // FIXME P7: Should Rush be used?
  //
  std::string     verb         = "POST";
  std::string     resource     = prefix + "/updateContext";
  std::string     tenant       = ciP->tenant;
  std::string     servicePath  = (ciP->httpHeaders.servicePathReceived == true)? ciP->httpHeaders.servicePath : "";
  std::string     mimeType     = "application/json";
  std::string     out;
  int             r;

  LM_T(LmtCPrForwardRequestPayload, ("forward updateContext request payload: %s", payload.c_str()));

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
                      cleanPayload,
                      ciP->httpHeaders.correlator,
                      "",
                      false,
                      true,
                      &out,
                      noHeaders,
                      mimeType);

  if (r != 0)
  {
    upcrsP->errorCode.fill(SccContextElementNotFound, "error forwarding update");
    LM_E(("Runtime Error (error forwarding 'Update' to providing application)"));
    return;
  }

  LM_T(LmtCPrForwardRequestPayload, ("forward updateContext response payload: %s", out.c_str()));


  //
  // 4. Parse the response and fill in a binary UpdateContextResponse
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
    LM_W(("Other Error (context provider response to UpdateContext is empty)"));
    upcrsP->errorCode.fill(SccContextElementNotFound, "invalid context provider response");
    return;
  }

  //
  // NOTE
  // When coming from a convenience operation, such as GET /v1/contextEntities/EID/attributes/attrName,
  // the verb/method in ciP is GET. However, the parsing function expects a POST, as if it came from a
  // POST /v1/updateContext.
  // So, here we change the verb/method for POST.
  //
  ParseData parseData;

  ciP->verb   = POST;
  ciP->method = "POST";

  parseData.upcrs.res.errorCode.fill(SccOk);

  s = jsonTreat(cleanPayload, ciP, &parseData, RtUpdateContextResponse, NULL);

  if (s != "OK")
  {
    LM_W(("Internal Error (error parsing reply from prov app: %s)", errorMsg.c_str()));
    upcrsP->errorCode.fill(SccContextElementNotFound, "");
    parseData.upcr.res.release();
    parseData.upcrs.res.release();
    return;
  }


  //
  // 5. Fill in the response from the redirection into the response of this function
  //
  upcrsP->fill(&parseData.upcrs.res);


  //
  // 6. 'Fix' StatusCode
  //
  if (upcrsP->errorCode.code == SccNone)
  {
    upcrsP->errorCode.fill(SccOk);
  }

  if ((upcrsP->contextElementResponseVector.size() == 1) && (upcrsP->contextElementResponseVector[0]->statusCode.code == SccContextElementNotFound))
  {
    upcrsP->errorCode.fill(SccContextElementNotFound);
  }


  //
  // 7. Freeing memory
  //
  parseData.upcr.res.release();
  parseData.upcrs.res.release();
}



/* ****************************************************************************
*
* foundAndNotFoundAttributeSeparation -
*
* Examine the response from mongo to find out what has really happened ...
*
*/
static void foundAndNotFoundAttributeSeparation(UpdateContextResponse* upcrsP, UpdateContextRequest* upcrP, ConnectionInfo* ciP)
{
  ContextElementResponseVector  notFoundV;

  for (unsigned int cerIx = 0; cerIx < upcrsP->contextElementResponseVector.size(); ++cerIx)
  {
    ContextElementResponse* cerP = upcrsP->contextElementResponseVector[cerIx];

    //
    // All attributes with found == false?
    //
    int noOfFounds    = 0;
    int noOfNotFounds = 0;

    for (unsigned int aIx = 0; aIx < cerP->contextElement.contextAttributeVector.size(); ++aIx)
    {
      if (cerP->contextElement.contextAttributeVector[aIx]->found == true)
      {
        ++noOfFounds;
      }
      else
      {
        ++noOfNotFounds;
      }
    }

    //
    // Now, if we have ONLY FOUNDS, then things stay the way they are, one response with '200 OK'
    // If we have ONLY NOT-FOUNDS, the we have one response with '404 Not Found'
    // If we have a mix, then we need to add a response for the not founds, with '404 Not Found'
    //
    if ((noOfFounds == 0) && (noOfNotFounds > 0))
    {
      if ((cerP->statusCode.code == SccOk) || (cerP->statusCode.code == SccNone))
      {
        cerP->statusCode.fill(SccContextElementNotFound, cerP->contextElement.entityId.id);
      }
    }
    else if ((noOfFounds > 0) && (noOfNotFounds > 0))
    {
      // Adding a ContextElementResponse for the 'Not-Founds'

      ContextElementResponse* notFoundCerP = new ContextElementResponse(&cerP->contextElement.entityId, NULL);

      //
      // Filling in StatusCode (SccContextElementNotFound) for NotFound
      //
      notFoundCerP->statusCode.fill(SccContextElementNotFound, cerP->contextElement.entityId.id);

      //
      // Setting StatusCode to OK for Found
      //
      cerP->statusCode.fill(SccOk);

      //
      // And, pushing to NotFound-vector
      //
      notFoundV.push_back(notFoundCerP);


      // Now moving the not-founds to notFoundCerP
      std::vector<ContextAttribute*>::iterator iter;
      for (iter = cerP->contextElement.contextAttributeVector.vec.begin(); iter < cerP->contextElement.contextAttributeVector.vec.end();)
      {
        if ((*iter)->found == false)
        {
          // 1. Push to notFoundCerP
          notFoundCerP->contextElement.contextAttributeVector.push_back(*iter);

          // 2. remove from cerP
          iter = cerP->contextElement.contextAttributeVector.vec.erase(iter);
        }
        else
        {
          ++iter;
        }
      }
    }

    //
    // Add EntityId::id to StatusCode::details if 404, but only if StatusCode::details is empty
    //
    if ((cerP->statusCode.code == SccContextElementNotFound) && (cerP->statusCode.details == ""))
    {
      cerP->statusCode.details = cerP->contextElement.entityId.id;
    }
  }

  //
  // Now add the contextElementResponses for 404 Not Found
  //
  if (notFoundV.size() != 0)
  {
    for (unsigned int ix = 0; ix < notFoundV.size(); ++ix)
    {
      upcrsP->contextElementResponseVector.push_back(notFoundV[ix]);
    }
  }


  //
  // If nothing at all in response vector, mark as not found (but not if DELETE request)
  //
  if (ciP->method != "DELETE")
  {
    if (upcrsP->contextElementResponseVector.size() == 0)
    {
      if (upcrsP->errorCode.code == SccOk)
      {
        upcrsP->errorCode.fill(SccContextElementNotFound, upcrP->contextElementVector[0]->entityId.id);
      }
    }
  }


  //
  // Add entityId::id to details if Not Found and only one element in response.
  // And, if 0 elements in response, take entityId::id from the request.
  //
  if (upcrsP->errorCode.code == SccContextElementNotFound)
  {
    if (upcrsP->contextElementResponseVector.size() == 1)
    {
      upcrsP->errorCode.details = upcrsP->contextElementResponseVector[0]->contextElement.entityId.id;
    }
    else if (upcrsP->contextElementResponseVector.size() == 0)
    {
      upcrsP->errorCode.details = upcrP->contextElementVector[0]->entityId.id;
    }
  }
}



/* ****************************************************************************
*
* attributesToNotFound - mark all attributes with 'found=false'
*/
static void attributesToNotFound(UpdateContextRequest* upcrP)
{
  for (unsigned int ceIx = 0; ceIx < upcrP->contextElementVector.size(); ++ceIx)
  {
    ContextElement* ceP = upcrP->contextElementVector[ceIx];

    for (unsigned int aIx = 0; aIx < ceP->contextAttributeVector.size(); ++aIx)
    {
      ContextAttribute* aP = ceP->contextAttributeVector[aIx];

      aP->found = false;
    }
  }
}



/* ****************************************************************************
*
* postUpdateContext -
*
* POST /v1/updateContext
* POST /ngsi10/updateContext
*
* Payload In:  UpdateContextRequest
* Payload Out: UpdateContextResponse
*/
std::string postUpdateContext
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP,
  Ngsiv2Flavour              ngsiV2Flavour
)
{
  UpdateContextResponse*  upcrsP = &parseDataP->upcrs.res;
  UpdateContextRequest*   upcrP  = &parseDataP->upcr.res;
  std::string             answer;

  bool asJsonObject = (ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object" && ciP->outMimeType == JSON);

  //
  // 01. Check service-path consistency
  //
  // If more than ONE service-path is input, an error is returned as response.
  // If ONE service-path is issued and that service path is "", then the default service-path is used.
  // Note that by construction servicePath cannot have 0 elements
  // After these checks, the service-path is checked to be 'correct'.
  //
  if (ciP->servicePathV.size() > 1)
  {
    upcrsP->errorCode.fill(SccBadRequest, "more than one service path in context update request");
    alarmMgr.badInput(clientIp, "more than one service path for an update request");

    TIMED_RENDER(answer = upcrsP->render(asJsonObject));
    upcrP->release();

    return answer;
  }
  else if (ciP->servicePathV[0] == "")
  {
    ciP->servicePathV[0] = SERVICE_PATH_ROOT;
  }

  std::string res = servicePathCheck(ciP->servicePathV[0].c_str());
  if (res != "OK")
  {
    upcrsP->errorCode.fill(SccBadRequest, res);

    TIMED_RENDER(answer = upcrsP->render(asJsonObject));

    upcrP->release();
    return answer;
  }


  //
  // 02. Send the request to mongoBackend/mongoUpdateContext
  //
  upcrsP->errorCode.fill(SccOk);
  attributesToNotFound(upcrP);

  HttpStatusCode httpStatusCode;
  TIMED_MONGO(httpStatusCode = mongoUpdateContext(upcrP,
                                                  upcrsP,
                                                  ciP->tenant,
                                                  ciP->servicePathV,
                                                  ciP->uriParam,
                                                  ciP->httpHeaders.xauthToken,
                                                  ciP->httpHeaders.correlator,
                                                  ciP->httpHeaders.ngsiv2AttrsFormat,
                                                  ciP->apiVersion,
                                                  ngsiV2Flavour));

  if (ciP->httpStatusCode != SccCreated)
  {
    ciP->httpStatusCode = httpStatusCode;
  }

  foundAndNotFoundAttributeSeparation(upcrsP, upcrP, ciP);



  //
  // 03. Normal case - no forwards
  //
  // If there is nothing to forward, just return the result
  //
  bool forwarding = forwardsPending(upcrsP);
  if (forwarding == false)
  {
    TIMED_RENDER(answer = upcrsP->render(asJsonObject));

    upcrP->release();
    return answer;
  }



  //
  // 04. mongoBackend doesn't give us the values of the attributes.
  //     So, here we have to do a search inside the initial UpdateContextRequest to fill in all the
  //     attribute-values in the output from mongoUpdateContext
  //
  for (unsigned int cerIx = 0; cerIx < upcrsP->contextElementResponseVector.size(); ++cerIx)
  {
    ContextElement* ceP = &upcrsP->contextElementResponseVector[cerIx]->contextElement;

    for (unsigned int aIx = 0; aIx < ceP->contextAttributeVector.size(); ++aIx)
    {
      ContextAttribute* aP = upcrP->attributeLookup(&ceP->entityId, ceP->contextAttributeVector[aIx]->name);

      if (aP == NULL)
      {
        LM_E(("Internal Error (attribute '%s' not found)", ceP->contextAttributeVector[aIx]->name.c_str()));
      }
      else
      {
        ceP->contextAttributeVector[aIx]->stringValue    = aP->stringValue;
        ceP->contextAttributeVector[aIx]->numberValue    = aP->numberValue;
        ceP->contextAttributeVector[aIx]->boolValue      = aP->boolValue;
        ceP->contextAttributeVector[aIx]->valueType      = aP->valueType;
        ceP->contextAttributeVector[aIx]->compoundValueP = aP->compoundValueP == NULL ? NULL : aP->compoundValueP->clone();
      }
    }
  }


  //
  // 05. Forwards necessary - sort parts in outgoing requestV
  //     requestV is a vector of UpdateContextRequests and each Context Provider
  //     will have a slot in the vector.
  //     When a ContextElementResponse is found in the output from mongoUpdateContext, a
  //     UpdateContextRequest is to be found/created and inside that UpdateContextRequest
  //     a ContextElement for the Entity of the ContextElementResponse.
  //
  //     Non-found parts go directly to 'response'.
  //
  UpdateContextRequestVector  requestV;
  UpdateContextResponse       response;

  response.errorCode.fill(SccOk);
  for (unsigned int cerIx = 0; cerIx < upcrsP->contextElementResponseVector.size(); ++cerIx)
  {
    ContextElementResponse* cerP  = upcrsP->contextElementResponseVector[cerIx];

    if (cerP->contextElement.contextAttributeVector.size() == 0)
    {
      //
      // If we find a contextElement without attributes here, then something is wrong
      //
      LM_E(("Orion Bug (empty contextAttributeVector for ContextElementResponse %d)", cerIx));
    }
    else
    {
      for (unsigned int aIx = 0; aIx < cerP->contextElement.contextAttributeVector.size(); ++aIx)
      {
        ContextAttribute* aP = cerP->contextElement.contextAttributeVector[aIx];

        //
        // 0. If the attribute is 'not-found' - just add the attribute to the outgoing response
        //
        if (aP->found == false)
        {
          ContextAttribute ca(aP);
          response.notFoundPush(&cerP->contextElement.entityId, &ca, NULL);
          continue;
        }


        //
        // 1. If the attribute is found locally - just add the attribute to the outgoing response
        //
        if (aP->providingApplication.get() == "")
        {
          ContextAttribute ca(aP);
          response.foundPush(&cerP->contextElement.entityId, &ca);
          continue;
        }


        //
        // 2. Lookup UpdateContextRequest in requestV according to providingApplication.
        //    If not found, add one.
        UpdateContextRequest*  reqP = requestV.lookup(aP->providingApplication.get());
        if (reqP == NULL)
        {
          reqP = new UpdateContextRequest(aP->providingApplication.get(), &cerP->contextElement.entityId);
          reqP->updateActionType = ActionTypeUpdate;
          requestV.push_back(reqP);
        }

        //
        // 3. Lookup ContextElement in UpdateContextRequest according to EntityId.
        //    If not found, add one (to the ContextElementVector of the UpdateContextRequest).
        //
        ContextElement* ceP = reqP->contextElementVector.lookup(&cerP->contextElement.entityId);
        if (ceP == NULL)
        {
          ceP = new ContextElement(&cerP->contextElement.entityId);
          reqP->contextElementVector.push_back(ceP);
        }


        //
        // 4. Add ContextAttribute to the correct ContextElement in the correct UpdateContextRequest
        //
        ceP->contextAttributeVector.push_back(new ContextAttribute(aP));
      }
    }
  }


  //
  // Now we are ready to forward the Updates
  //


  //
  // Calling each of the Context Providers, merging their results into the
  // total response 'response'
  //

  for (unsigned int ix = 0; ix < requestV.size() && ix < cprForwardLimit; ++ix)
  {
    if (requestV[ix]->contextProvider == "")
    {
      LM_E(("Internal Error (empty context provider string)"));
      continue;
    }

    UpdateContextResponse upcrs;

    updateForward(ciP, requestV[ix], &upcrs);

    //
    // Add the result from the forwarded update to the total response in 'response'
    //
    response.merge(&upcrs);
  }

  // Note this is a slight break in the separation of concerns among the different layers (i.e.
  // serviceRoutine/ logic should work in a "NGSIv1 isolated context"). However, it seems to be
  // a smart way of dealing with partial update situations
  if (ciP->apiVersion == V2)
  {
    // Adjust OrionError response in the case of partial updates. This may happen in CPr forwarding
    // scenarios. Note that mongoBackend logic "splits" successfull updates and failing updates in
    // two different CER (maybe using the same entity)

    std::string failing = "";
    unsigned int fails  = 0;

    for (unsigned int ix = 0; ix < response.contextElementResponseVector.size(); ++ix)
    {

      ContextElementResponse* cerP = response.contextElementResponseVector[ix];

      if (cerP->statusCode.code != SccOk)
      {
        fails++;

        std::string failingPerCer = "";
        for (unsigned int jx = 0; jx < cerP->contextElement.contextAttributeVector.size(); ++jx)
        {
          failingPerCer += cerP->contextElement.contextAttributeVector[jx]->name;
          if (jx != cerP->contextElement.contextAttributeVector.size() - 1)
          {
            failingPerCer +=", ";
          }
        }

        failing += cerP->contextElement.entityId.id + "-" + cerP->contextElement.entityId.type + " : [" + failingPerCer + "], ";
      }
    }

    // Note that we modify parseDataP->upcrs.res.oe and not response.oe, as the former is the
    // one used by the calling postBatchUpdate() function at serviceRoutineV2 library
    if (fails == response.contextElementResponseVector.size())
    {
      parseDataP->upcrs.res.oe.fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_ENTITY, ERROR_NOT_FOUND);
    }
    else if (fails > 0)
    {
      // Removing trailing ", "
      failing = failing.substr(0, failing.size() - 2);

      // If some CER (but not all) fails, then it is a partial update
      parseDataP->upcrs.res.oe.fill(SccContextElementNotFound, "Attributes that were not updated: { " + failing + " }", "PartialUpdate");
    }
    else  // fails == 0
    {
      // No failure, so invalidate any possible OrionError filled by mongoBackend on the mongoUpdateContext step
      parseDataP->upcrs.res.oe.fill(SccNone, "");
    }

  }
  else  // v1
  {
    // Note that v2 case doesn't use an actual response (so no need to waste time rendering it).
    // We render in the v1 case only
    TIMED_RENDER(answer = response.render(asJsonObject));
  }

  //
  // Cleanup
  //
  upcrP->release();
  requestV.release();
  upcrsP->release();
  upcrsP->fill(&response);
  response.release();

  return answer;
}
