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

#include "jsonParse/jsonRequest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/limits.h"
#include "common/globals.h"
#include "common/statistics.h"
#include "common/string.h"
#include "common/limits.h"
#include "common/errorMessages.h"

#include "alarmMgr/alarmMgr.h"
#include "metricsMgr/metricsMgr.h"
#include "ngsi/ParseData.h"
#include "mongoBackend/mongoSubCache.h"
#include "jsonParseV2/jsonRequestTreat.h"
#include "parse/textParse.h"
#include "serviceRoutines/badRequest.h"

#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "rest/restReply.h"
#include "rest/rest.h"
#include "rest/uriParamNames.h"
#include "rest/RestService.h"



/* ****************************************************************************
*
* service vectors - 
*/
static RestService*              getServiceV           = NULL;
static RestService*              putServiceV           = NULL;
static RestService*              postServiceV          = NULL;
static RestService*              patchServiceV         = NULL;
static RestService*              deleteServiceV        = NULL;
static RestService*              optionsServiceV       = NULL;
RestService*                     restBadVerbV          = NULL;



/* ****************************************************************************
*
* serviceVectorsSet
*/
void serviceVectorsSet
(
  RestService*        _getServiceV,
  RestService*        _putServiceV,
  RestService*        _postServiceV,
  RestService*        _patchServiceV,
  RestService*        _deleteServiceV,
  RestService*        _optionsServiceV,
  RestService*        _restBadVerbV
)
{
  getServiceV      = _getServiceV;
  putServiceV      = _putServiceV;
  postServiceV     = _postServiceV;
  patchServiceV    = _patchServiceV;
  deleteServiceV   = _deleteServiceV;
  optionsServiceV  = _optionsServiceV;
  restBadVerbV     = _restBadVerbV;
}

#include "serviceRoutinesV2/postRegistration.h"


/* ****************************************************************************
*
* delayedRelease -
*/
static void delayedRelease(JsonDelayedRelease* releaseP)
{
  if (releaseP->entity != NULL)
  {
    releaseP->entity->release();
    releaseP->entity = NULL;
  }

  if (releaseP->attribute != NULL)
  {
    releaseP->attribute->release();
    releaseP->attribute = NULL;
  }

  if (releaseP->scrP != NULL)
  {
    releaseP->scrP->release();
    releaseP->scrP = NULL;
  }

  if (releaseP->ucsrP != NULL)
  {
    releaseP->ucsrP->release();
    releaseP->ucsrP = NULL;
  }

  if (releaseP->subsP != NULL)
  {
    delete releaseP->subsP;
    releaseP->subsP = NULL;
  }
}



/* ****************************************************************************
*
* payloadParse -
*/
std::string payloadParse
(
  ConnectionInfo*            ciP,
  ParseData*                 parseDataP,
  RestService*               service,
  JsonRequest**              jsonPP,
  JsonDelayedRelease*        jsonReleaseP,
  std::vector<std::string>&  compV
)
{
  std::string result = "NONE";

  LM_T(LmtParsedPayload, ("parsing data for service '%s'. Method: '%s'", requestType(service->request), ciP->method.c_str()));
  LM_T(LmtParsedPayload, ("outMimeType: %s", mimeTypeToString(ciP->outMimeType)));

  ciP->requestType = service->request;

  if (ciP->inMimeType == JSON)
  {
    if (ciP->apiVersion == V2)
    {
      result = jsonRequestTreat(ciP, parseDataP, service->request, jsonReleaseP, compV);
    }
    else
    {
      result = jsonTreat(ciP->payload, ciP, parseDataP, service->request, service->payloadWord, jsonPP);
    }
  }
  else if (ciP->inMimeType == TEXT)
  {
    result = textRequestTreat(ciP, parseDataP, service->request);
  }
  else
  {
    alarmMgr.badInput(clientIp, "payload mime-type is not JSON");
    return "Bad inMimeType";
  }

  LM_T(LmtParsedPayload, ("result:      '%s'", result.c_str()));
  LM_T(LmtParsedPayload, ("outMimeType: '%s'", mimeTypeToString(ciP->outMimeType)));

  if (result != "OK")
  {
    restReply(ciP, result);
  }

  return result;
}



/* ****************************************************************************
*
* tenantCheck -
*
* This function used to be 'static', but as it is now used by MetricsMgr::serviceValid
* it has been mede 'extern'.
* This might change when github issue #2781 is looked into and if we stop using the
* function, is should go back to being 'static'.
*/
std::string tenantCheck(const std::string& tenant)
{
  char*  name = (char*) tenant.c_str();

  if (strlen(name) > SERVICE_NAME_MAX_LEN)
  {
    char numV1[STRING_SIZE_FOR_INT];
    char numV2[STRING_SIZE_FOR_LONG];

    snprintf(numV1, sizeof(numV1), "%d",  SERVICE_NAME_MAX_LEN);
    snprintf(numV2, sizeof(numV2), "%lu", strlen(name));

    std::string details = std::string("a tenant name can be max ") + numV1 + " characters long. Length: " + numV2;
    alarmMgr.badInput(clientIp, details);

    return "bad length - a tenant name can be max " SERVICE_NAME_MAX_LEN_STRING " characters long";
  }

  while (*name != 0)
  {
    if ((!isalnum(*name)) && (*name != '_'))
    {
      std::string details = std::string("bad character in tenant name - only underscore and alphanumeric characters are allowed. Offending character: ") + *name;

      alarmMgr.badInput(clientIp, details);
      return "bad character in tenant name - only underscore and alphanumeric characters are allowed";
    }

    ++name;
  }

  return "OK";
}



/* ****************************************************************************
*
* commonFilters -
*/
static void commonFilters
(
  ConnectionInfo*   ciP,
  ParseData*        parseDataP,
  RestService*      serviceP
)
{
  //
  // 1. ?!exist=entity::type
  //
  if (ciP->uriParam[URI_PARAM_NOT_EXIST] == SCOPE_VALUE_ENTITY_TYPE)
  {
    Restriction* restrictionP = NULL;

    //
    // Lookup the restriction of the correct parseData, where to add the new scope.
    //
    if (serviceP->request == EntityTypes)
    {
      restrictionP = &parseDataP->qcr.res.restriction;
    }
    else if (serviceP->request == AllContextEntities)
    {
      restrictionP = &parseDataP->qcr.res.restriction;
    }


    if (restrictionP == NULL)
    {
      // There are two possibilities to be here:
      //   1. A filter given for a request NOT SUPPORTING the filter
      //   2. The restrictionP-lookup is MISSING (not implemented)
      //
      // Either way, we just silently return.
      //
      return;
    }

    Scope* scopeP  = new Scope(SCOPE_FILTER_EXISTENCE, SCOPE_VALUE_ENTITY_TYPE);
    scopeP->oper   = SCOPE_OPERATOR_NOT;
    restrictionP->scopeVector.push_back(scopeP);
  }



  //
  // 2. ?exist=entity::type
  //
  if (ciP->uriParam[URI_PARAM_EXIST] == SCOPE_VALUE_ENTITY_TYPE)
  {
    Restriction* restrictionP = NULL;

    //
    // Lookup the restriction of the correct parseData, where to add the new scope.
    //
    if (serviceP->request == EntityTypes)
    {
      restrictionP = &parseDataP->qcr.res.restriction;
    }
    else if (serviceP->request == AllContextEntities)
    {
      restrictionP = &parseDataP->qcr.res.restriction;
    }

    if (restrictionP == NULL)
    {
      // There are two possibilities to be here:
      //   1. A filter given for a request NOT SUPPORTING the filter
      //   2. The restrictionP-lookup is MISSING (not implemented)
      //
      // Either way, we just silently return.
      //
      return;
    }

    Scope*  scopeP  = new Scope(SCOPE_FILTER_EXISTENCE, SCOPE_VALUE_ENTITY_TYPE);
    scopeP->oper    = "";
    restrictionP->scopeVector.push_back(scopeP);
  }
}



/* ****************************************************************************
*
* scopeFilter -
*/
static void scopeFilter
(
  ConnectionInfo*   ciP,
  ParseData*        parseDataP,
  RestService*      serviceP
)
{
  std::string  payloadWord  = ciP->payloadWord;
  Restriction* restrictionP = NULL;

  if (payloadWord == "discoverContextAvailabilityRequest")
  {
    restrictionP = &parseDataP->dcar.res.restriction;
  }
  else if (payloadWord == "subscribeContextAvailabilityRequest")
  {
    restrictionP = &parseDataP->scar.res.restriction;
  }
  else if (payloadWord == "updateContextAvailabilitySubscriptionRequest")
  {
    restrictionP = &parseDataP->ucas.res.restriction;
  }
  else if (payloadWord == "queryContextRequest")
  {
    restrictionP = &parseDataP->qcr.res.restriction;
  }
  else if (payloadWord == "subscribeContextRequest")
  {
    restrictionP = &parseDataP->scr.res.restriction;
  }
  else if (payloadWord == "updateContextSubscriptionRequest")
  {
    restrictionP = &parseDataP->ucsr.res.restriction;
  }
  else
  {
    return;
  }

  for (unsigned int ix = 0; ix < restrictionP->scopeVector.size(); ++ix)
  {
    Scope* scopeP = restrictionP->scopeVector[ix];

    if (scopeP->type == SCOPE_FILTER_NOT_EXISTENCE)
    {
      scopeP->type = SCOPE_FILTER_EXISTENCE;
      scopeP->oper = SCOPE_OPERATOR_NOT;
    }
  }
}



/* ****************************************************************************
*
* filterRelease -
*/
static void filterRelease(ParseData* parseDataP, RequestType request)
{
  Restriction* restrictionP = NULL;

  if (request == EntityTypes)
  {
    restrictionP = &parseDataP->qcr.res.restriction;
  }
  else if (request == AllContextEntities)
  {
    restrictionP = &parseDataP->qcr.res.restriction;
  }

  if (restrictionP != NULL)
    restrictionP->release();
}



/* ****************************************************************************
*
* compCheck -
*/
static bool compCheck(int components, const std::vector<std::string>& compV)
{
  for (int ix = 0; ix < components; ++ix)
  {
    if (compV[ix] == "")
    {
      return false;
    }
  }

  return true;
}


/* ****************************************************************************
*
* compErrorDetect -
*/
static bool compErrorDetect
(
  ApiVersion                       apiVersion,
  int                              components,
  const std::vector<std::string>&  compV,
  OrionError*                      oeP
)
{
  std::string  details;

  if ((apiVersion == V2) && (compV[1] == "entities"))
  {
    if ((components == 4) && (compV[3] == "attrs"))  // URL: /v2/entities/<entity-id>/attrs
    {
      std::string entityId = compV[2];

      if (entityId == "")
      {
        details = ERROR_DESC_BAD_REQUEST_EMPTY_ENTITY_ID;
      }
    }
    else if ((components == 5) && (compV[3] == "attrs"))  // URL: /v2/entities/<entity-id>/attrs/<attr-name>
    {
      std::string entityId = compV[2];
      std::string attrName = compV[4];

      if (entityId == "")
      {
        details = ERROR_DESC_BAD_REQUEST_EMPTY_ENTITY_ID;
      }
      else if (attrName == "")
      {
        details = ERROR_DESC_BAD_REQUEST_EMPTY_ATTR_NAME;
      }
    }
    else if ((components == 6) && (compV[3] == "attrs") && (compV[5] == "value")) // URL: /v2/entities/<entity-id>/attrs/<attr-name>/value
    {
      std::string entityId = compV[2];
      std::string attrName = compV[4];

      if (entityId == "")
      {
        details = ERROR_DESC_BAD_REQUEST_EMPTY_ENTITY_ID;
      }
      else if (attrName == "")
      {
        details = ERROR_DESC_BAD_REQUEST_EMPTY_ATTR_NAME;
      }
    }
  }

  if (details != "")
  {
    oeP->fill(SccBadRequest, details);
    return true;  // means: this was an error, make the broker stop this request
  }

  return false;  // No special error detected, let the broker continue with the request to detect the error later on
}



/* ****************************************************************************
*
* restService -
*
* This function is called with the appropriate RestService vector, depending on the VERB used in the request.
* If no matching service is found in this RestService vector, then a recursive call in made, using the "badVerb RestService vector",
* to see if we have a matching bad-verb-service-routine.
* If there is no badVerb RestService vector, then the "default error service routine "badRequest" is used.
* And lastly, if there is a badVerb RestService vector, but still no service routine is found, then we create a "service not recognized"
* response. See comments incrusted in the function as well.
*/
static std::string restService(ConnectionInfo* ciP, RestService* serviceV)
{
  std::vector<std::string>  compV;
  int                       components;
  JsonRequest*              jsonReqP   = NULL;
  ParseData                 parseData;
  JsonDelayedRelease        jsonRelease;

  if ((ciP->url.length() == 0) || ((ciP->url.length() == 1) && (ciP->url.c_str()[0] == '/')))
  {
    OrionError  error(SccBadRequest, "The Orion Context Broker is a REST service, not a 'web page'");
    std::string response = error.render();

    alarmMgr.badInput(clientIp, "The Orion Context Broker is a REST service, not a 'web page'");
    restReply(ciP, response);

    return std::string("Empty URL");
  }

  ciP->httpStatusCode = SccOk;


  //
  // Split URI PATH into components
  //
  components = stringSplit(ciP->url, '/', compV);
  if (!compCheck(components, compV))
  {
    OrionError oe;

    if (compErrorDetect(ciP->apiVersion, components, compV, &oe))
    {
      alarmMgr.badInput(clientIp, oe.details);
      ciP->httpStatusCode = SccBadRequest;
      restReply(ciP, oe.smartRender(ciP->apiVersion));
      return "URL PATH component error";
    }
  }

  //
  // Lookup the requested service
  //
  
  for (unsigned int ix = 0; serviceV[ix].treat != NULL; ++ix)
  {
    if ((serviceV[ix].components != 0) && (serviceV[ix].components != components))
    {
      continue;
    }

    strncpy(ciP->payloadWord, serviceV[ix].payloadWord.c_str(), sizeof(ciP->payloadWord));
    bool match = true;
    for (int compNo = 0; compNo < components; ++compNo)
    {
      const char* component = serviceV[ix].compV[compNo].c_str();
      
      if ((component[0] == '*') && (component[1] == 0))
      {
        continue;
      }

      if (ciP->apiVersion == V1)
      {
        if (strcasecmp(component, compV[compNo].c_str()) != 0)
        {
          match = false;
          break;
        }
      }
      else
      {
        if (strcmp(component, compV[compNo].c_str()) != 0)
        {
          match = false;
          break;
        }
      }
    }

    if (match == false)
    {
      continue;
    }


    //
    // If in restBadVerbV vector, no need to check the payload
    //
    if ((serviceV != restBadVerbV) && (ciP->payload != NULL) && (ciP->payloadSize != 0) && (ciP->payload[0] != 0))
    {
      std::string response;
      std::string spath = (ciP->servicePathV.size() > 0)? ciP->servicePathV[0] : "";

      LM_T(LmtParsedPayload, ("Parsing payload for URL '%s', method '%s', service vector index: %d", ciP->url.c_str(), ciP->method.c_str(), ix));
      ciP->parseDataP = &parseData;
      metricsMgr.add(ciP->httpHeaders.tenant, spath, METRIC_TRANS_IN_REQ_SIZE, ciP->payloadSize);
      LM_T(LmtPayload, ("Parsing payload '%s'", ciP->payload));
      response = payloadParse(ciP, &parseData, &serviceV[ix], &jsonReqP, &jsonRelease, compV);
      LM_T(LmtParsedPayload, ("payloadParse returns '%s'", response.c_str()));

      if (response != "OK")
      {
        alarmMgr.badInput(clientIp, response);
        restReply(ciP, response);

        if (jsonReqP != NULL)
        {
          jsonReqP->release(&parseData);
        }

        if (ciP->apiVersion == V2)
        {
          delayedRelease(&jsonRelease);
        }

        compV.clear();
        return response;
      }
    }

    LM_T(LmtService, ("Treating service %s %s", ciP->method.c_str(), ciP->url.c_str())); // Sacred - used in 'heavyTest'
    if (ciP->payloadSize == 0)
    {
      ciP->inMimeType = NOMIMETYPE;
    }
    statisticsUpdate(serviceV[ix].request, ciP->inMimeType);

    // Tenant to connectionInfo
    ciP->tenant = ciP->tenantFromHttpHeader;
    lmTransactionSetService(ciP->tenant.c_str());

    //
    // A tenant string must not be longer than 50 characters and may only contain
    // underscores and alphanumeric characters.
    //
    std::string result;
    if ((ciP->tenant != "") && ((result = tenantCheck(ciP->tenant)) != "OK"))
    {
      OrionError  oe(SccBadRequest, result);

      std::string  response = oe.setStatusCodeAndSmartRender(ciP->apiVersion, &(ciP->httpStatusCode));

      alarmMgr.badInput(clientIp, result);

      restReply(ciP, response);

      if (jsonReqP != NULL)
      {
        jsonReqP->release(&parseData);
      }

      if (ciP->apiVersion == V2)
      {
        delayedRelease(&jsonRelease);
      }

      compV.clear();

      return response;
    }

    LM_T(LmtTenant, ("tenant: '%s'", ciP->tenant.c_str()));
    commonFilters(ciP, &parseData, &serviceV[ix]);
    scopeFilter(ciP, &parseData, &serviceV[ix]);

    //
    // If we have gotten this far the Input is OK.
    // Except for all the badVerb/badRequest, in the restBadVerbV vector.
    //
    // So, the 'Bad Input' alarm is cleared for this client.
    //
    if (serviceV != restBadVerbV)
    {
      alarmMgr.badInputReset(clientIp);
    }

    std::string response = serviceV[ix].treat(ciP, components, compV, &parseData);

    filterRelease(&parseData, serviceV[ix].request);

    if (jsonReqP != NULL)
    {
      jsonReqP->release(&parseData);
    }

    if (ciP->apiVersion == V2)
    {
      delayedRelease(&jsonRelease);
    }

    compV.clear();

    if (response == "DIE")
    {
      orionExitFunction(0, "Received a 'DIE' request on REST interface");
    }

    restReply(ciP, response);
    return response;
  }

  //
  // No service routine found. Need to check bad-verb service vector.
  // If there is no bad-verb service vector (restBadVerbV == NULL), then
  // badRequest() is used as service routine ... 
  //
  if (restBadVerbV == NULL)
  {
    std::vector<std::string> cV;

    return badRequest(ciP, 0, cV, NULL);
  }

  //
  // ... but, if we have a non-NULL restBadVerbV, then we make a recursive call, using the
  // restBadVerbV service vector.  But, only if the current service vector is NOT the restBadVerbV,
  // of course. A situation like that would mean we are already in the recursive call and need to end
  // the recursion and return an error  ...
  //
  if (serviceV != restBadVerbV)
  {
    return restService(ciP, restBadVerbV);
  }

  //
  // ... and this here is the error that is returned. A 400 Bad Request with "service XXX not recognized" as payload
  //
  std::string details = std::string("service '") + ciP->url + "' not recognized";
  alarmMgr.badInput(clientIp, details);

  ciP->httpStatusCode = SccBadRequest;
  std::string answer = restErrorReplyGet(ciP, "", ciP->payloadWord, SccBadRequest, std::string("service not found"));
  restReply(ciP, answer);

  compV.clear();
  return answer;
}



namespace orion
{
/* ****************************************************************************
*
* orion::requestServe -
*/
std::string requestServe(ConnectionInfo* ciP)
{
  if      ((ciP->verb == GET)     && (getServiceV     != NULL))    return restService(ciP, getServiceV);
  else if ((ciP->verb == POST)    && (postServiceV    != NULL))    return restService(ciP, postServiceV);
  else if ((ciP->verb == PUT)     && (putServiceV     != NULL))    return restService(ciP, putServiceV);
  else if ((ciP->verb == PATCH)   && (patchServiceV   != NULL))    return restService(ciP, patchServiceV);
  else if ((ciP->verb == DELETE)  && (deleteServiceV  != NULL))    return restService(ciP, deleteServiceV);
  else if ((ciP->verb == OPTIONS) && (optionsServiceV != NULL))    return restService(ciP, optionsServiceV);
  else                                                             return restService(ciP, restBadVerbV);
}

}
