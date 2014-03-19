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
        LM_T(LmtRestCompare, ("ix %d: component %d is not a match ('%s' != '%s')", ix, compNo, compV[compNo].c_str(), serviceV[ix].compV[compNo].c_str()));
        break;
      }
    }

    if (match == false)
      continue;

    if ((ciP->payload != NULL) && (ciP->payloadSize != 0) && (serviceV[ix].verb != "*"))
    {
      std::string response;

      LM_T(LmtParsedPayload, ("Parsing payload for URL '%s', method '%s', service vector index: %d", ciP->url.c_str(), ciP->method.c_str(), ix));
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
