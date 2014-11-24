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
#include <string>

#include "xmlParse/xmlRequest.h"
#include "jsonParse/jsonRequest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/statistics.h"
#include "common/string.h"
#include "rest/ConnectionInfo.h"
#include "rest/OrionError.h"
#include "rest/RestService.h"
#include "rest/restReply.h"
#include "rest/rest.h"
#include "rest/uriParamNames.h"
#include "ngsi/ParseData.h"



/* ****************************************************************************
*
* Tenant name max length
*/
#define MAX_TENANT_NAME_LEN            50
#define MAX_TENANT_NAME_LEN_STRING    "50"



/* ****************************************************************************
*
* payloadParse - 
*/
std::string payloadParse(ConnectionInfo* ciP, ParseData* parseDataP, RestService* service, XmlRequest** reqPP, JsonRequest** jsonPP)
{
  std::string result = "NONE";

  LM_T(LmtParsedPayload, ("parsing data for service '%s'. Method: '%s'", requestType(service->request), ciP->method.c_str()));
  LM_T(LmtParsedPayload, ("outFormat: %s", formatToString(ciP->outFormat)));

  if (ciP->inFormat == XML)
  {
    LM_T(LmtParsedPayload, ("Calling xmlTreat for service request %d, payloadWord '%s'", service->request, service->payloadWord.c_str()));
    result = xmlTreat(ciP->payload, ciP, parseDataP, service->request, service->payloadWord, reqPP);
  }
  else if (ciP->inFormat == JSON)
    result = jsonTreat(ciP->payload, ciP, parseDataP, service->request, service->payloadWord, jsonPP);
  else
  {
    LM_W(("Bad Input (payload mime-type is neither JSON nor XML)"));
    return "Bad inFormat";
  }

  LM_T(LmtParsedPayload, ("result: '%s'", result.c_str()));
  LM_T(LmtParsedPayload, ("outFormat: %s", formatToString(ciP->outFormat)));

  if (result != "OK")
  {
    restReply(ciP, result);
  }

  return result;
}



/* ****************************************************************************
*
* tenantCheck - 
*/
static std::string tenantCheck(const std::string& tenant)
{
  char*        name    = (char*) tenant.c_str();

  if (strlen(name) > MAX_TENANT_NAME_LEN)
  {
    LM_W(("Bad Input (a tenant name can be max %d characters long. Length: %d)", MAX_TENANT_NAME_LEN, strlen(name)));
    return "bad length - a tenant name can be max " MAX_TENANT_NAME_LEN_STRING " characters long";
  }

  while (*name != 0)
  {
    if ((!isalnum(*name)) && (*name != '_'))
    {
      LM_W(("Bad Input (bad character in tenant name - only underscore and alphanumeric characters are allowed. Offending character: %c)", *name));
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
* restService - 
*/
std::string restService(ConnectionInfo* ciP, RestService* serviceV)
{
  std::vector<std::string>  compV;
  int                       components;
  XmlRequest*               reqP       = NULL;
  JsonRequest*              jsonReqP   = NULL;
  ParseData                 parseData;

  if ((ciP->url.length() == 0) || ((ciP->url.length() == 1) && (ciP->url.c_str()[0] == '/')))
  {
    OrionError  error(SccBadRequest, "The Orion Context Broker is a REST service, not a 'web page'");
    std::string response = error.render(ciP->outFormat, "");

    LM_W(("Bad Input (The Orion Context Broker is a REST service, not a 'web page')"));
    restReply(ciP, response);

    return std::string("Empty URL");
  }

  ciP->httpStatusCode = SccOk;

  components = stringSplit(ciP->url, '/', compV);

  for (unsigned int ix = 0; serviceV[ix].treat != NULL; ++ix)
  {
    if ((serviceV[ix].components != 0) && (serviceV[ix].components != components))
    {
      continue;
    }

    if ((ciP->method != serviceV[ix].verb) && (serviceV[ix].verb != "*"))
    {
      continue;
    }

    strncpy(ciP->payloadWord, serviceV[ix].payloadWord.c_str(), sizeof(ciP->payloadWord));
    bool match = true;
    for (int compNo = 0; compNo < components; ++compNo)
    {
      if (serviceV[ix].compV[compNo] == "*")
      {
        continue;
      }

      if (strcasecmp(serviceV[ix].compV[compNo].c_str(), compV[compNo].c_str()) != 0)
      {
        match = false;
        break;
      }
    }

    if (match == false)
    {
      continue;
    }

    if ((ciP->payload != NULL) && (ciP->payloadSize != 0) && (serviceV[ix].verb != "*"))
    {
      std::string response;

      LM_T(LmtParsedPayload, ("Parsing payload for URL '%s', method '%s', service vector index: %d", ciP->url.c_str(), ciP->method.c_str(), ix));
      ciP->parseDataP = &parseData;
      response = payloadParse(ciP, &parseData, &serviceV[ix], &reqP, &jsonReqP);
      LM_T(LmtParsedPayload, ("payloadParse returns '%s'", response.c_str()));

      if (response != "OK")
      {
        restReply(ciP, response);

        if (reqP != NULL)
        {
          reqP->release(&parseData);
        }
        if (jsonReqP != NULL)
        {
          jsonReqP->release(&parseData);
        }

        compV.clear();
        return response;
      }
    }

    LM_T(LmtService, ("Treating service %s %s", serviceV[ix].verb.c_str(), ciP->url.c_str())); // Sacred - used in 'heavyTest'
    statisticsUpdate(serviceV[ix].request, ciP->inFormat);

    // Tenant to connectionInfo
    ciP->tenant = ciP->tenantFromHttpHeader;

    //
    // A tenant string must not be longer than 50 characters and may only contain
    // underscores and alphanumeric characters.
    //
    std::string result;
    if ((ciP->tenant != "") && ((result = tenantCheck(ciP->tenant)) != "OK"))
    {
      OrionError  error(SccBadRequest,
                        "tenant name not accepted - a tenant string must not be longer than " MAX_TENANT_NAME_LEN_STRING " characters"
                        " and may only contain underscores and alphanumeric characters");

      std::string  response = error.render(ciP->outFormat, "");

      LM_W(("Bad Input (%s)", error.details.c_str()));
      restReply(ciP, response);

      if (reqP != NULL)
      {
        reqP->release(&parseData);
      }

      if (jsonReqP != NULL)
      {
        jsonReqP->release(&parseData);
      }

      compV.clear();
        
      return response;
    }

    LM_T(LmtTenant, ("tenant: '%s'", ciP->tenant.c_str()));
    commonFilters(ciP, &parseData, &serviceV[ix]);
    scopeFilter(ciP, &parseData, &serviceV[ix]);
    std::string response = serviceV[ix].treat(ciP, components, compV, &parseData);
    filterRelease(&parseData, serviceV[ix].request);

    if (reqP != NULL)
    {
      reqP->release(&parseData);
    }

    if (jsonReqP != NULL)
    {
      jsonReqP->release(&parseData);
    }

    compV.clear();

    if (response == "DIE")
    {
      orionExitFunction(0, "Received a 'DIE' request on REST interface");
    }

    restReply(ciP, response);
    return response;
  }

  LM_W(("Bad Input (service '%s' not recognized)", ciP->url.c_str()));
  ciP->httpStatusCode = SccBadRequest;
  std::string answer = restErrorReplyGet(ciP, ciP->outFormat, "", ciP->payloadWord, SccBadRequest, std::string("unrecognized request"));
  restReply(ciP, answer);

  compV.clear();
  return answer;
}
