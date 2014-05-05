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
#include "ngsi/ParseData.h"



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
    LM_E(("Bad inFormat: %d", (int) ciP->inFormat));
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
static std::string tenantCheck(std::string tenant)
{
  if ((tenant == "ngsi9") || (tenant == "NGSI9") || (tenant == "ngsi10") || (tenant == "NGSI10") || 
      (tenant == "log") || (tenant == "version") || (tenant == "statistics") || (tenant == "leak") || (tenant == "exit"))
  {
    return "tenant name coincides with Orion reserved name";
  }

  char* name = (char*) tenant.c_str();

  if (strlen(name) > 20)
    return "bad length - a tenant name can be max 20 characters long";

  while (*name != 0)
  {
    if ((!isalnum(*name)) && (*name != '-') && (*name != '_'))
    {
      LM_E(("offending character: %c", *name));
      return "bad character in tenant name - only hyphen, underscore and alphanumeric characters are allowed";
    }

    ++name;
  }

  return "OK";
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
    restReply(ciP, response);
    return std::string("Empty URL");
  }

  ciP->httpStatusCode = SccOk;

  components = stringSplit(ciP->url, '/', compV);

  for (unsigned int ix = 0; serviceV[ix].treat != NULL; ++ix)
  {
    if ((serviceV[ix].components != 0) && (serviceV[ix].components != components))
      continue;

    if ((ciP->method != serviceV[ix].verb) && (serviceV[ix].verb != "*"))
      continue;

    strncpy(ciP->payloadWord, serviceV[ix].payloadWord.c_str(), sizeof(ciP->payloadWord));
    bool match = true;
    for (int compNo = 0; compNo < components; ++compNo)
    {
      if (serviceV[ix].compV[compNo] == "*")
        continue;

      if (strcasecmp(serviceV[ix].compV[compNo].c_str(), compV[compNo].c_str()) != 0)
      {
        match = false;
        break;
      }
    }

    if (match == false)
      continue;

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
          reqP->release(&parseData);
        if (jsonReqP != NULL)
          jsonReqP->release(&parseData);

        compV.clear();
        return response;
      }
    }

    LM_T(LmtService, ("Treating service %s %s", serviceV[ix].verb.c_str(), ciP->url.c_str())); // Sacred - used in 'heavyTest'
    statisticsUpdate(serviceV[ix].request, ciP->inFormat);

    // Tenant to connectionInfo
    if (serviceV[ix].compV[0] == "*")
    {
      LM_T(LmtTenant, ("URL tenant: '%s'", compV[0].c_str()));
      ciP->tenantFromUrl = compV[0];
    }

    if (multitenant == "url")
      ciP->tenant = ciP->tenantFromUrl;
    else if (multitenant == "header")
      ciP->tenant = ciP->tenantFromHttpHeader;
    else // multitenant == "off"
      ciP->tenant = "";

    //
    // A tenant string must not be longer that 20 characters and may only contain
    // hyphens, underscores and alphanumeric characters.
    //
    LM_M(("multitenant mode: '%s', Checking tenant: '%s'", multitenant.c_str(), ciP->tenant.c_str()));
    std::string result;
    if ((ciP->tenant != "") && ((result = tenantCheck(ciP->tenant)) != "OK"))
    {
      OrionError   error(SccBadRequest, "tenant format not accepted (a tenant string must not be longer that 20 characters and may only contain hyphens, underscores and alphanumeric characters)");
      std::string  response = error.render(ciP->outFormat, "");

      LM_E(("tenant name error: %s", result.c_str()));
      restReply(ciP, response);
      return response;
    }
    else
      LM_M(("tenant name '%s' is OK", ciP->tenant.c_str()));

    LM_T(LmtTenant, ("tenant: '%s'", ciP->tenant.c_str()));
    std::string response = serviceV[ix].treat(ciP, components, compV, &parseData);

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
      orionExitFunction(1, "Received a 'DIE' request");


    restReply(ciP, response);
    return response;
  }

  LM_E(("Service '%s' not recognized", ciP->url.c_str()));
  ciP->httpStatusCode = SccBadRequest;
  std::string answer = restErrorReplyGet(ciP, ciP->outFormat, "", ciP->payloadWord, SccBadRequest, std::string("Service not recognized: ") + ciP->url);
  restReply(ciP, answer);

  compV.clear();
  return answer;
}
