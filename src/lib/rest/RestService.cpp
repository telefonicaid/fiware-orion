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

#include "orionld/types/ApiVersion.h"                          // ApiVersion
#include "orionld/common/orionldState.h"                       // orionldState

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



/* *****************************************************************************
*
* restServiceVectorGet -
*
* FIXME P2: Create a vector of service vectors, for faster access.
*           E.g
* RestService** serviceVV[7];
* serviceVV[POST] = postServiceV;
* serviceVV[GET]  = getServiceV;
* etc.
*
* Then remove the switch to find the correct service vector, just do this:
*
* serviceV = restServiceVV[verb];
* 
*/
RestService* restServiceVectorGet(Verb verb)
{
  switch (verb)
  {
  case HTTP_POST:       return postServiceV;
  case HTTP_PUT:        return putServiceV;
  case HTTP_GET:        return getServiceV;
  case HTTP_PATCH:      return patchServiceV;
  case HTTP_DELETE:     return deleteServiceV;
  case HTTP_OPTIONS:    return (optionsServiceV == NULL)? restBadVerbV : optionsServiceV;
  default:              return restBadVerbV;
  }
}



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

  if (orionldState.in.contentType == MT_JSON)
  {
    if (orionldState.apiVersion == API_VERSION_NGSI_V2)
    {
      //
      // FIXME #3151: jsonRequestTreat should return 'bool' and accept an output parameter 'OrionError* oeP'.
      //              Same same for all underlying JSON APIv2 parsing functions
      //              Not sure the same thing can be done for 'jsonTreat' in the else-part, but this should AT LEAST
      //              be fixed for V2.
      //
      result = jsonRequestTreat(ciP, parseDataP, service->request, jsonReleaseP, compV);
    }
    else
    {
      result = jsonTreat(orionldState.in.payload, ciP, parseDataP, service->request, jsonPP);
    }
  }
  else if (orionldState.in.contentType == MT_TEXT)
  {
    result = textRequestTreat(ciP, parseDataP, service->request);
  }
  else
  {
    alarmMgr.badInput(orionldState.clientIp, "payload mime-type is not JSON");
    return "Bad Input";
  }

  if (result != "OK")
    restReply(ciP, result.c_str());

  return result;
}



/* ****************************************************************************
*
* tenantCheck -
*
* This function used to be 'static', but as it is now used by MetricsMgr::serviceValid
* it has been made 'extern'.
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
    snprintf(numV2, sizeof(numV2), "%zu", strlen(name));

    std::string details = std::string("a tenant name can be max ") + numV1 + " characters long. Length: " + numV2;
    alarmMgr.badInput(orionldState.clientIp, details);

    return "bad length - a tenant name can be max " SERVICE_NAME_MAX_LEN_STRING " characters long";
  }

  while (*name != 0)
  {
    if ((!isalnum(*name)) && (*name != '_'))
    {
      std::string details = std::string("bad character in tenant name - only underscore and alphanumeric characters are allowed. Offending character: ") + *name;

      alarmMgr.badInput(orionldState.clientIp, details);
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
  if (orionldState.in.entityTypeDoesNotExist == true)
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
  if (orionldState.in.entityTypeExists == true)
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
  Restriction* restrictionP = NULL;

  if (ciP->restServiceP->request == DiscoverContextAvailability)
  {
    restrictionP = &parseDataP->dcar.res.restriction;
  }
  else if (ciP->restServiceP->request == SubscribeContextAvailability)
  {
    restrictionP = &parseDataP->scar.res.restriction;
  }
  else if (ciP->restServiceP->request == UpdateContextAvailabilitySubscription)
  {
    restrictionP = &parseDataP->ucas.res.restriction;
  }
  else if (ciP->restServiceP->request == QueryContext)
  {
    restrictionP = &parseDataP->qcr.res.restriction;
  }
  else if (ciP->restServiceP->request == SubscribeContext)
  {
    restrictionP = &parseDataP->scr.res.restriction;
  }
  else if (ciP->restServiceP->request == UpdateContextSubscription)
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

  if ((apiVersion == API_VERSION_NGSI_V2) && (compV[1] == "entities"))
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
std::string restService(ConnectionInfo* ciP, RestService* serviceV)
{
  JsonRequest*              jsonReqP   = NULL;
  ParseData                 parseData;
  JsonDelayedRelease        jsonRelease;

  if ((orionldState.urlPath == NULL) || (orionldState.urlPath[0] == 0) || ((orionldState.urlPath[0] == '/') && (orionldState.urlPath[1] == 0)))
  {
    OrionError  error(SccBadRequest, "The Orion Context Broker is a REST service, not a 'web page'");
    std::string response = error.render();

    orionldState.httpStatusCode = SccBadRequest;
    alarmMgr.badInput(orionldState.clientIp, "The Orion Context Broker is a REST service, not a 'web page'");
    restReply(ciP, response.c_str());

    return std::string("Empty URL");
  }

  if (!compCheck(ciP->urlComponents, ciP->urlCompV))
  {
    OrionError oe;

    if (compErrorDetect(orionldState.apiVersion, ciP->urlComponents, ciP->urlCompV, &oe))
    {
      alarmMgr.badInput(orionldState.clientIp, oe.details);
      orionldState.httpStatusCode = SccBadRequest;
      restReply(ciP, oe.smartRender(orionldState.apiVersion).c_str());
      return "URL PATH component error";
    }
  }

  //
  // Check the payload, if any
  //
  if ((orionldState.in.payload != NULL) && (orionldState.in.payloadSize != 0) && (orionldState.in.payload[0] != 0))
  {
    std::string  response;
    const char*  spath = (ciP->servicePathV.size() > 0)? ciP->servicePathV[0].c_str() : "";

    metricsMgr.add(orionldState.tenantP->tenant, spath, METRIC_TRANS_IN_REQ_SIZE, orionldState.in.payloadSize);
    response = payloadParse(ciP, &parseData, ciP->restServiceP, &jsonReqP, &jsonRelease, ciP->urlCompV);

    if (response != "OK")
    {
      alarmMgr.badInput(orionldState.clientIp, response);
      restReply(ciP, response.c_str());

      if (jsonReqP != NULL)
        jsonReqP->release(&parseData);

      if (orionldState.apiVersion == API_VERSION_NGSI_V2)
      {
        delayedRelease(&jsonRelease);
      }

      return response;
    }
  }

  if (orionldState.in.payloadSize == 0)
  {
    orionldState.in.contentType = MT_NONE;
  }
  statisticsUpdate(ciP->restServiceP->request, orionldState.in.contentType);

  // Tenant to connectionInfo
  lmTransactionSetService(orionldState.tenantP->tenant);

  //
  // A tenant string must not be longer than 50 characters and may only contain
  // underscores and alphanumeric characters.
  //
  std::string result;
  if ((orionldState.tenantName != NULL) && ((result = tenantCheck(orionldState.tenantName)) != "OK"))
  {
    OrionError  oe(SccBadRequest, result);

    std::string  response = oe.setStatusCodeAndSmartRender(orionldState.apiVersion, &orionldState.httpStatusCode);

    alarmMgr.badInput(orionldState.clientIp, result);

    restReply(ciP, response.c_str());

    if (jsonReqP != NULL)
    {
      jsonReqP->release(&parseData);
    }

    if (orionldState.apiVersion == API_VERSION_NGSI_V2)
    {
      delayedRelease(&jsonRelease);
    }

    return response;
  }

  commonFilters(ciP, &parseData, ciP->restServiceP);
  scopeFilter(ciP, &parseData, ciP->restServiceP);

  //
  // If we have gotten this far the Input is OK.
  // Except for all the badVerb/badRequest, in the restBadVerbV vector.
  //
  // So, the 'Bad Input' alarm is cleared for this client.
  //
  alarmMgr.badInputReset(orionldState.clientIp);

  std::string response = ciP->restServiceP->treat(ciP, ciP->urlComponents, ciP->urlCompV, &parseData);

  filterRelease(&parseData, ciP->restServiceP->request);

  if (jsonReqP != NULL)
  {
    jsonReqP->release(&parseData);
  }

  if (orionldState.apiVersion == API_VERSION_NGSI_V2)
  {
    delayedRelease(&jsonRelease);
  }

  if (response == "DIE")
  {
    orionExitFunction(0, "Received a 'DIE' request on REST interface");
  }

  restReply(ciP, response.c_str());
  return response;
}



namespace orion
{
/* ****************************************************************************
*
* orion::requestServe -
*/
std::string requestServe(ConnectionInfo* ciP)
{
  if      ((orionldState.verb == HTTP_GET)     && (getServiceV     != NULL))    return restService(ciP, getServiceV);
  else if ((orionldState.verb == HTTP_POST)    && (postServiceV    != NULL))    return restService(ciP, postServiceV);
  else if ((orionldState.verb == HTTP_PUT)     && (putServiceV     != NULL))    return restService(ciP, putServiceV);
  else if ((orionldState.verb == HTTP_PATCH)   && (patchServiceV   != NULL))    return restService(ciP, patchServiceV);
  else if ((orionldState.verb == HTTP_DELETE)  && (deleteServiceV  != NULL))    return restService(ciP, deleteServiceV);
  else if ((orionldState.verb == HTTP_OPTIONS) && (optionsServiceV != NULL))    return restService(ciP, optionsServiceV);
  else                                                             return restService(ciP, restBadVerbV);
}

}
