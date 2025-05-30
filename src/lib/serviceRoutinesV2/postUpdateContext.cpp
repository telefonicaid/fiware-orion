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
#include "common/logTracing.h"
#include "alarmMgr/alarmMgr.h"

#include "jsonParseV2/parseEntitiesResponseV1.h"
#include "mongoBackend/mongoUpdateContext.h"
#include "ngsi/ParseData.h"
#include "ngsi/UpdateContextResponse.h"
#include "orionTypes/UpdateContextRequestVector.h"
#include "rest/ConnectionInfo.h"
#include "rest/httpRequestSend.h"
#include "rest/uriParamNames.h"
#include "serviceRoutinesV2/postUpdateContext.h"



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

    for (unsigned int aIx = 0 ; aIx < cerP->entity.attributeVector.size(); ++aIx)
    {
      ContextAttribute* aP  = cerP->entity.attributeVector[aIx];

      if (!aP->provider.http.url.empty())
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
* 6. 'Fix' OrionError
* 7. Freeing memory
*
*/
static bool updateForward
(
  ConnectionInfo*         ciP,
  UpdateContextRequest*   upcrP,
  const std::string&      regId,
  unsigned int            correlatorCounter,
  UpdateContextResponse*  upcrsP)
{
  std::string      ip;
  std::string      protocol;
  int              port;
  std::string      prefix;

  //
  // 1. Parse the providing application to extract IP, port and URI-path
  //
  if (parseUrl(upcrP->contextProvider, ip, port, prefix, protocol) == false)
  {
    std::string details = std::string("invalid providing application");

    alarmMgr.badInput(clientIp, details, upcrP->contextProvider);

    //
    //  Somehow, if we accepted this providing application, it is the brokers fault ...
    //  SccBadRequest should have been returned before, when it was registered!
    //
    upcrsP->error.fill(SccContextElementNotFound, "");
    return false;
  }

  //
  // 2. Render the string of the request we want to forward
  //
  MimeType     outMimeType = ciP->outMimeType;
  std::string  payload;
  char*        cleanPayload;

  ciP->outMimeType  = JSON;

  std::string     verb;
  std::string     resource;
  std::string     op;
  std::string     tenant       = ciP->tenant;
  std::string     servicePath  = (ciP->httpHeaders.servicePathReceived == true)? ciP->httpHeaders.servicePath : "";
  std::string     mimeType     = "application/json";
  std::string     out;
  int             r;

  if (upcrP->legacyProviderFormat)
  {
    TIMED_RENDER(payload = upcrP->toJsonV1());

    op = "/updateContext";

    // Note we don't include log deprecation here as it would cause double-loging
    // (this flow involves Registration::fromBson() which already logs that)
  }
  else
  {
    TIMED_RENDER(payload = upcrP->toJson());

    op = "/op/update";
#if 0
    // FIXME #3485: this part is not removed by the moment, in the case it may be useful in the
    // context of issue #3485

    Entity* eP = upcrP->entityVector[0];

    eP->renderId = false;

    TIMED_RENDER(payload = eP->toJson(NGSI_V2_NORMALIZED));

    resource = prefix + "/entities/" + eP->id + "/attrs";
    verb     = "PATCH";

    if (!eP->type.empty())
    {
      // Add ?type=<TYPE> to 'resource'
      resource += "?type=" + eP->type;
    }
#endif
  }

  verb     = "POST";
  resource = prefix + op;

  ciP->outMimeType  = outMimeType;
  cleanPayload      = (char*) payload.c_str();
  std::string         url;
  char                portV[STRING_SIZE_FOR_INT];

  //
  // 3. Send the request to the Context Provider (and await the reply)
  //
  LM_T(LmtCPrForwardRequestPayload, ("forward updateContext request payload: %s", payload.c_str()));

  std::map<std::string, std::string>  noHeaders;
  long long                           statusCode;

  char suffix[STRING_SIZE_FOR_INT];
  snprintf(suffix, sizeof(suffix), "%u", correlatorCounter);
  std::string effectiveCorrelator = ciP->httpHeaders.correlator + "; cbfwd=" + suffix;

  snprintf(portV, sizeof(portV), "%d", port);
  url = ip + ":" + portV + resource;

  r = httpRequestSend(NULL,
                      "regId: " + regId,
                      fromIp,   // thread variable
                      ip,
                      port,
                      protocol,
                      verb,
                      tenant,
                      servicePath,
                      ciP->httpHeaders.xauthToken,
                      resource,
                      mimeType,
                      cleanPayload,
                      effectiveCorrelator,
                      "",
                      &out,
                      &statusCode,
                      noHeaders,
                      mimeType);

  if (r != 0)
  {
    upcrsP->error.fill(SccContextElementNotFound, "error forwarding update");
    logInfoFwdRequest(regId.c_str(), verb.c_str(), (upcrP->contextProvider + op).c_str(), payload.c_str(), "", out.c_str());
    alarmMgr.forwardingError(url, "forwarding failure for sender-thread: " + out);
    return false;
  }
  else
  {
    alarmMgr.forwardingErrorReset(url);
  }
  
  LM_T(LmtCPrForwardResponsePayload, ("forward updateContext response payload: %s", out.c_str()));

  cleanPayload = jsonPayloadClean(out.c_str());

  //
  // If NGSIv1 (legacyProviderFormat):
  //   4. Parse the response and fill in a binary UpdateContextResponse
  //   5. Fill in the response from the redirection into the response of this function
  //   6. 'Fix' OrionError
  //   7. Free up memory
  //
  // If NGSIv2:
  //   4. Look for "204 No Content" in the response of the forwarded request
  //   5. If found: OK, else, error
  //
  if (upcrP->legacyProviderFormat)
  {
    //
    // 4. Parse the response and fill in a binary UpdateContextResponse
    //
    bool result;
    EntityVector  entities;
    OrionError    oe;

    if ((cleanPayload == NULL) || (cleanPayload[0] == 0))
    {
      //
      // This is really an internal error in the Context Provider
      // It is not in the orion broker though, so 404 is returned
      //
      alarmMgr.forwardingError(url, "context provider response to UpdateContext is empty");
      upcrsP->error.fill(SccContextElementNotFound, "invalid context provider response");
      return false;
    }

    logInfoFwdRequest(regId.c_str(), verb.c_str(), (upcrP->contextProvider + op).c_str(), payload.c_str(), cleanPayload, statusCode);

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

    parseData.upcrs.res.error.fill(SccOk);

    result = parseEntitiesResponseV1(ciP, cleanPayload, &entities, &oe);

    if (!result)
    {
      alarmMgr.forwardingError(url, "error parsing reply from context provider: " + oe.error + " (" + oe.description + ")");
      upcrsP->error.fill(SccContextElementNotFound, "");

      parseData.upcr.res.release();
      parseData.upcrs.res.release();
      entities.release();

      return false;
    }


    //
    // 5. Fill in the response from the redirection into the response of this function
    //
    upcrsP->fill(&parseData.upcrs.res);


    //
    // 6. 'Fix' OrionError
    //
    if (upcrsP->error.code == SccNone)
    {
      upcrsP->error.fill(SccOk);
    }

    if ((upcrsP->contextElementResponseVector.size() == 1) && (upcrsP->contextElementResponseVector[0]->error.code == SccContextElementNotFound))
    {
      upcrsP->error.fill(SccContextElementNotFound);
    }

    //
    // 7. Free up memory
    //
    parseData.upcr.res.release();
    parseData.upcrs.res.release();
    entities.release();

    return true;
  }
  else  // NGSIv2
  {
    // NGSIv2 forward - no payload to be received

    logInfoFwdRequest(regId.c_str(), verb.c_str(), (upcrP->contextProvider + op).c_str(), payload.c_str(), cleanPayload, statusCode);

    if (statusCode == SccNoContent)
    {
      upcrsP->fill(upcrP, SccOk);
      return true;
    }
    // SccInvalidModification is the case por partial updates
    if ((statusCode == SccContextElementNotFound) || (statusCode == SccInvalidModification))
    {
      upcrsP->fill(upcrP, SccContextElementNotFound);
      return true;
    }

    alarmMgr.forwardingError(url, "unexpected response from context provider: %s" + out);
    upcrsP->error.fill(SccReceiverInternalError);
    return false;
  }

  // Can't reach this point - no return-statement needed
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

    for (unsigned int aIx = 0; aIx < cerP->entity.attributeVector.size(); ++aIx)
    {
      if (cerP->entity.attributeVector[aIx]->found == true)
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
      if ((cerP->error.code == SccOk) || (cerP->error.code == SccNone))
      {
        cerP->error.fill(SccContextElementNotFound, cerP->entity.entityId.id);
      }
    }
    else if ((noOfFounds > 0) && (noOfNotFounds > 0))
    {
      // Adding a ContextElementResponse for the 'Not-Founds'

      ContextElementResponse* notFoundCerP = new ContextElementResponse();
      notFoundCerP->entity.fill(cerP->entity.entityId);

      //
      // Filling in OrionError (SccContextElementNotFound) for NotFound
      //
      notFoundCerP->error.fill(SccContextElementNotFound, cerP->entity.entityId.id);

      //
      // Setting OrionError to OK for Found
      //
      cerP->error.fill(SccOk);

      //
      // And, pushing to NotFound-vector
      //
      notFoundV.push_back(notFoundCerP);


      // Now moving the not-founds to notFoundCerP
      std::vector<ContextAttribute*>::iterator iter;
      for (iter = cerP->entity.attributeVector.vec.begin(); iter < cerP->entity.attributeVector.vec.end();)
      {
        if ((*iter)->found == false)
        {
          // 1. Push to notFoundCerP
          notFoundCerP->entity.attributeVector.push_back(*iter);

          // 2. remove from cerP
          iter = cerP->entity.attributeVector.vec.erase(iter);
        }
        else
        {
          ++iter;
        }
      }
    }

    //
    // Add EntityId::id to OrionError::details if 404, but only if OrionError::description is empty
    //
    if ((cerP->error.code == SccContextElementNotFound) && (cerP->error.description.empty()))
    {
      cerP->error.description = cerP->entity.entityId.id;
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
      if (upcrsP->error.code == SccOk)
      {
        upcrsP->error.fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_ENTITY);
      }
    }
  }
}



/* ****************************************************************************
*
* attributesToNotFound - mark all attributes with 'found=false'
*/
static void attributesToNotFound(UpdateContextRequest* upcrP)
{
  for (unsigned int ceIx = 0; ceIx < upcrP->entityVector.size(); ++ceIx)
  {
    Entity* eP = upcrP->entityVector[ceIx];

    for (unsigned int aIx = 0; aIx < eP->attributeVector.size(); ++aIx)
    {
      ContextAttribute* aP = eP->attributeVector[aIx];

      aP->found = false;
    }
  }
}



/* ****************************************************************************
*
* postUpdateContext -
*
* Internal functions used by serviceRoutinesV2 functions
*
* Payload In:  UpdateContextRequest
* Payload Out: UpdateContextResponse
*/
void postUpdateContext
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

  bool forcedUpdate = ciP->uriParamOptions[OPT_FORCEDUPDATE];
  bool overrideMetadata = ciP->uriParamOptions[OPT_OVERRIDEMETADATA];
  bool flowControl  = ciP->uriParamOptions[OPT_FLOW_CONTROL];
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
    upcrsP->error.fill(SccBadRequest, "more than one service path in context update request");
    alarmMgr.badInput(clientIp, "more than one service path for an update request");

    upcrP->release();

    return;
  }
  else if (ciP->servicePathV[0].empty())
  {
    ciP->servicePathV[0] = SERVICE_PATH_ROOT;
  }

  std::string res = servicePathCheck(ciP->servicePathV[0].c_str());
  if (res != "OK")
  {
    upcrsP->error.fill(SccBadRequest, res);
    upcrP->release();
    return;
  }


  //
  // 02. Send the request to mongoBackend/mongoUpdateContext
  //
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
                                                  forcedUpdate,
                                                  overrideMetadata,
                                                  ngsiV2Flavour,
                                                  flowControl));

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
    upcrP->release();
    return;
  }



  //
  // 04. mongoBackend doesn't give us the values of the attributes.
  //     So, here we have to do a search inside the initial UpdateContextRequest to fill in all the
  //     attribute-values in the output from mongoUpdateContext
  //
  for (unsigned int cerIx = 0; cerIx < upcrsP->contextElementResponseVector.size(); ++cerIx)
  {
    Entity* eP = &upcrsP->contextElementResponseVector[cerIx]->entity;

    for (unsigned int aIx = 0; aIx < eP->attributeVector.size(); ++aIx)
    {
      ContextAttribute* aP = upcrP->attributeLookup(eP, eP->attributeVector[aIx]->name);

      if (aP == NULL)
      {
        LM_E(("Runtime Error (attribute '%s' not found)", eP->attributeVector[aIx]->name.c_str()));
      }
      else
      {
        eP->attributeVector[aIx]->stringValue    = aP->stringValue;
        eP->attributeVector[aIx]->numberValue    = aP->numberValue;
        eP->attributeVector[aIx]->boolValue      = aP->boolValue;
        eP->attributeVector[aIx]->valueType      = aP->valueType;
        eP->attributeVector[aIx]->compoundValueP = aP->compoundValueP == NULL ? NULL : aP->compoundValueP->clone();
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

  std::vector<std::string>    regIdsV;

  response.error.fill(SccOk);
  for (unsigned int cerIx = 0; cerIx < upcrsP->contextElementResponseVector.size(); ++cerIx)
  {
    ContextElementResponse* cerP  = upcrsP->contextElementResponseVector[cerIx];

    for (unsigned int aIx = 0; aIx < cerP->entity.attributeVector.size(); ++aIx)
    {
      ContextAttribute* aP = cerP->entity.attributeVector[aIx];

      //
      // 0. If the attribute is 'not-found' - just add the attribute to the outgoing response
      //
      if (aP->found == false)
      {
        ContextAttribute ca(aP);
        response.notFoundPush(&cerP->entity, &ca, NULL);
        continue;
      }


      //
      // 1. If the attribute is found locally - just add the attribute to the outgoing response
      //
      if (aP->provider.http.url.empty())
      {
        ContextAttribute ca(aP);
        response.foundPush(&cerP->entity, &ca);
        continue;
      }


      //
      // 2. Lookup UpdateContextRequest in requestV according to provider.
      //    If not found, add one.
      UpdateContextRequest*  reqP = requestV.lookup(aP->provider.http.url);
      if (reqP == NULL)
      {
        reqP = new UpdateContextRequest(aP->provider.http.url, aP->provider.legacyForwardingMode, &cerP->entity);
        reqP->updateActionType = ActionTypeUpdate;
        requestV.push_back(reqP);
        regIdsV.push_back(aP->providerRegId);
      }

      //
      // 3. Lookup ContextElement in UpdateContextRequest according to EntityId.
      //    If not found, add one (to the EntityVector of the UpdateContextRequest).
      //
      Entity* eP = reqP->entityVector.lookup(cerP->entity.entityId.id, cerP->entity.entityId.type);
      if (eP == NULL)
      {
        eP = new Entity();
        eP->fill(cerP->entity.entityId);
        reqP->entityVector.push_back(eP);
      }


      //
      // 4. Add ContextAttribute to the correct ContextElement in the correct UpdateContextRequest
      //
      eP->attributeVector.push_back(new ContextAttribute(aP));
    }
  }


  //
  // Now we are ready to forward the Updates
  //


  //
  // Calling each of the Context Providers, merging their results into the
  // total response 'response'
  //
  bool forwardOk = true;

  // Note that queryForward() (due to internal calls to httpRequestSend())
  // change coordid= and transid= so we need to preserve them and restore once forwarding loop has ended
  // FIXME P5: maybe this is not the right place to store&recover old transaction.
  // What about inside httpRequestSend?

  std::string prevCoorId  = correlationIdGet();
  std::string prevTransId = transactionIdGetAsString();

  if (requestV.size() > 0)
  {
    logInfoFwdStart(ciP->method.c_str(), ciP->uriForLogs.c_str());
  }

  for (unsigned int ix = 0; ix < requestV.size() && ix < cprForwardLimit; ++ix)
  {
    if (requestV[ix]->contextProvider.empty())
    {
      LM_E(("Runtime Error (empty context provider string)"));
      continue;
    }

    UpdateContextResponse upcrs;
    bool                  b;

    b = updateForward(ciP, requestV[ix], regIdsV[ix], ix + 1, &upcrs);

    if (b == false)
    {
      forwardOk = false;
    }

    //
    // Add the result from the forwarded update to the total response in 'response'
    //
    response.merge(&upcrs);
  }

  correlatorIdSet(prevCoorId.c_str());
  transactionIdSet(prevTransId.c_str());

  //
  // Adjust OrionError response in the case of partial updates. This may happen in CPr forwarding
  // scenarios. Note that mongoBackend logic "splits" successfull updates and failing updates in
  // two different CER (maybe using the same entity)
  //
  std::string failing = "";
  unsigned int failures = 0;

  for (unsigned int ix = 0; ix < response.contextElementResponseVector.size(); ++ix)
  {
    ContextElementResponse *cerP = response.contextElementResponseVector[ix];

    if (cerP->error.code != SccOk)
    {
      failures++;

      std::string failingPerCer = "";
      for (unsigned int jx = 0; jx < cerP->entity.attributeVector.size(); ++jx)
      {
        failingPerCer += cerP->entity.attributeVector[jx]->name;
        if (jx != cerP->entity.attributeVector.size() - 1)
        {
          failingPerCer += ", ";
        }
      }

      failing += cerP->entity.entityId.id + "-" + cerP->entity.entityId.type + " : [" + failingPerCer + "], ";
    }
  }

  //
  // Note that we modify parseDataP->upcrs.res.error and not response.oe, as the former is the
  // one used by the calling postBatchUpdate() function at serviceRoutineV2 library
  //
  if ((forwardOk == true) && (failures == 0))
  {
    parseDataP->upcrs.res.error.fill(SccNone, "");
  }
  else if (failures == response.contextElementResponseVector.size())
  {
    parseDataP->upcrs.res.error.fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_ENTITY, ERROR_NOT_FOUND);
  }
  else if (failures > 0)
  {
    // Removing trailing ", "
    failing = failing.substr(0, failing.size() - 2);

    // If some CER (but not all) fail, then it is a partial update
    parseDataP->upcrs.res.error.fill(SccContextElementNotFound, "Some of the following attributes were not updated: { " + failing + " }", ERROR_PARTIAL_UPDATE);
  }
  else // failures == 0
  {
    // No failure, so invalidate any possible OrionError filled by mongoBackend on the mongoUpdateContext step
    parseDataP->upcrs.res.error.fill(SccNone, "");
  }

  //
  // Cleanup
  //
  upcrP->release();
  requestV.release();
  upcrsP->release();
  response.release();

  return;
}
