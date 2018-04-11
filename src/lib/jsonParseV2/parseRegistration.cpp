/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "rapidjson/document.h"

#include "logMsg/logMsg.h"
#include "common/string.h"
#include "common/errorMessages.h"
#include "common/globals.h"
#include "rest/ConnectionInfo.h"
#include "ngsi/Request.h"
#include "apiTypesV2/Registration.h"
#include "parse/forbiddenChars.h"
#include "jsonParseV2/badInput.h"
#include "jsonParseV2/utilsParse.h"
#include "jsonParseV2/parseEntitiesVector.h"
#include "jsonParseV2/parseStringVector.h"
#include "jsonParseV2/jsonRequestTreat.h"



/* ****************************************************************************
*
* dataProvidedParse -
*/
static bool dataProvidedParse
(
  ConnectionInfo*          ciP,
  ngsiv2::DataProvided*    dataProvidedP,
  const rapidjson::Value&  dataProvided,
  std::string*             errorStringP
)
{
  if (!dataProvided.IsObject())
  {
    *errorStringP = "/dataProvided/ must be a JSON object";
    return false;
  }

  if (dataProvided.ObjectEmpty())
  {
    *errorStringP = "/dataProvided/ is empty";
    return false;
  }

  if (dataProvided.HasMember("entities"))
  {
    bool b = parseEntitiesVector(ciP, &dataProvidedP->entities, dataProvided["entities"], errorStringP);
    if (b == false)
    {
      return false;
    }
  }

  if (dataProvided.HasMember("attrs"))
  {
    if (!parseStringVector(&dataProvidedP->attributes, dataProvided["attrs"], "attrs", true, errorStringP))
    {
      return false;
    }
  }

  return true;
}



/* ****************************************************************************
*
* httpParse -
*/
static bool httpParse(ngsiv2::Http* httpP, const rapidjson::Value& jsonHttp, const std::string& fieldName, std::string* errorStringP)
{
  if (!jsonHttp.IsObject())
  {
    *errorStringP = "/" + fieldName + "/ must be a JSON object";
    return false;
  }

  if (!jsonHttp.HasMember("url"))
  {
    *errorStringP = "/" + fieldName + "/ must contain a field /url/";
    return false;
  }

  const rapidjson::Value& url = jsonHttp["url"];

  if (!url.IsString())
  {
    *errorStringP = "/url/ field of /" + fieldName + "/ must be a JSON string";
    return false;
  }

  httpP->url = url.GetString();

  if (httpP->url == "")
  {
    *errorStringP = "/url/ field of /" + fieldName + "/ cannot be the empty string";
    return false;
  }

  std::string  host;
  int          port;
  std::string  path;
  std::string  protocol;

  if (parseUrl(httpP->url, host, port, path, protocol) == false)
  {
    *errorStringP = "/url/ field of /" + fieldName + "/ is invalid";
    return false;
  }

  return true;
}



/* ****************************************************************************
*
* providerParse -
*/
static bool providerParse(ConnectionInfo* ciP, ngsiv2::Provider* providerP, const rapidjson::Value& provider, std::string* errorStringP)
{
  bool b;

  if (!provider.IsObject())
  {
    *errorStringP = "/provider/ must be a JSON object";
    return false;
  }

  if (provider.ObjectEmpty())
  {
    *errorStringP = "/provider/ empty";
    return false;
  }

  if (provider.HasMember("http"))
  {
    b = httpParse(&providerP->http, provider["http"], "http", errorStringP);
    if (b == false)
    {
      return false;
    }
  }
  else
  {
    *errorStringP = "/provider/ must contain a field /http/";
    return false;
  }

  if (provider.HasMember("supportedForwardingMode"))
  {
    const rapidjson::Value& supportedForwardingModeValue = provider["supportedForwardingMode"];
    std::string             supportedForwardingMode;

    if (!supportedForwardingModeValue.IsString())
    {
      *errorStringP = "/supportedForwardingMode/ must be a JSON string";
      return false;
    }

    supportedForwardingMode = supportedForwardingModeValue.GetString();

    //
    // Check Valid strings for supportedForwardingMode
    //
    if      (supportedForwardingMode == "all")      providerP->supportedForwardingMode = ngsiv2::ForwardAll;
    else if (supportedForwardingMode == "none")     providerP->supportedForwardingMode = ngsiv2::ForwardNone;
    else if (supportedForwardingMode == "query")    providerP->supportedForwardingMode = ngsiv2::ForwardQuery;
    else if (supportedForwardingMode == "update")   providerP->supportedForwardingMode = ngsiv2::ForwardUpdate;
    else
    {
      *errorStringP = "invalid value of /supportedForwardingMode/";
      return false;
    }
  }
  else
  {
    providerP->supportedForwardingMode = ngsiv2::ForwardAll;
  }

  if (provider.HasMember("legacyForwarding"))
  {
    const rapidjson::Value& legacyForwarding = provider["legacyForwarding"];

    if (legacyForwarding.IsBool())
    {
      providerP->legacyForwardingMode = legacyForwarding.GetBool();
    }
    else
    {
      *errorStringP = "the field /legacyForwarding/ must be a boolean";
      return false;
    }
  }
  else
  {
    providerP->legacyForwardingMode = false;
  }

  return true;
}



/* ****************************************************************************
*
* parseRegistration -
*/
std::string parseRegistration(ConnectionInfo* ciP, ngsiv2::Registration* regP)
{
  std::string          errorString;
  rapidjson::Document  document;

  document.Parse(ciP->payload);

  if (document.HasParseError())
  {
    return badInput(ciP, ERROR_DESC_PARSE);
  }

  if (!document.IsObject())
  {
    return badInput(ciP, ERROR_DESC_BAD_REQUEST_NOT_A_JSON_OBJECT);
  }

  if (document.ObjectEmpty())
  {
    return badInput(ciP, ERROR_DESC_BAD_REQUEST_EMPTY_PAYLOAD);
  }


  //
  // Extracting field "description"
  //
  Opt<std::string> description = getStringOpt(document, "description");

  if (!description.ok())
  {
    return badInput(ciP, "invalid description");
  }
  else if (description.given)
  {
    regP->description = description.value;
    if (regP->description.length() > MAX_DESCRIPTION_LENGTH)
    {
      return badInput(ciP, "max description length exceeded");
    }

    if (forbiddenChars(regP->description.c_str()))
    {
      return badInput(ciP, "forbidden characters in description");
    }
  }


  //
  // Extracting field "dataProvided"
  //
  if (!document.HasMember("dataProvided"))
  {
    return badInput(ciP, "the field /dataProvided/ is missing in payload");
  }

  const rapidjson::Value& dataProvided = document["dataProvided"];

  if (dataProvidedParse(ciP, &regP->dataProvided, dataProvided, &errorString) == false)
  {
    return badInput(ciP, errorString);
  }


  //
  // Extracting field "provider"
  //
  if (!document.HasMember("provider"))
  {
    return badInput(ciP, "the field /provider/ is missing in payload");
  }

  const rapidjson::Value& provider = document["provider"];

  if (providerParse(ciP, &regP->provider, provider, &errorString) == false)
  {
    return badInput(ciP, errorString);
  }


  //
  // Extracting field "expires"
  //
  Opt<std::string> expiresOpt   = getStringOpt(document, "expires");
  int64_t          expiresValue = PERMANENT_EXPIRES_DATETIME;

  if (!expiresOpt.ok())
  {
    return badInput(ciP, expiresOpt.error);
  }
  else if (expiresOpt.given)
  {
    std::string  expires = expiresOpt.value;

    if (!expires.empty())
    {
      expiresValue = parse8601Time(expires);
      if (expiresValue == -1)
      {
        return badInput(ciP, "the field /expires/ has an invalid format");
      }
    }
  }
  regP->expires = expiresValue;


  //
  // Extracting field "status"
  //
  Opt<std::string> status = getStringOpt(document, "status");

  if (!status.ok())
  {
    return badInput(ciP, "/status/ must be a string");
  }
  else if (status.given)
  {
    regP->status = status.value;
    if ((regP->status != "active") && (regP->status != "inactive"))
    {
      return badInput(ciP, "invalid value for /status/");
    }
  }

  return "OK";
}
