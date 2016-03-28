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
#include <map>

#include "common/string.h"
#include "common/globals.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* validOptions - 
*
* Text definitions OPT_* found in common/globals.h
*/
static const char* validOptions[] = 
{
  OPT_COUNT,
  OPT_NORMALIZED,
  OPT_VALUES,
  OPT_KEY_VALUES,
  OPT_APPEND,
  OPT_UNIQUE_VALUES,
  OPT_DATE_CREATED,
  OPT_DATE_MODIFIED
};



/* ****************************************************************************
*
* isValidOption - 
*/
static bool isValidOption(std::string item)
{
  for (unsigned int ix = 0; ix < sizeof(validOptions) / sizeof(validOptions[0]); ++ix)
  {
    if (item == validOptions[ix])
    {
      return true;
    }
  }

  return false;
}


/* ****************************************************************************
*
* uriParamOptionsParse - parse the URI param 'options' into uriParamOptions map
*
* RETURN VALUE
*  0  on success
* <0  on error
*/
int uriParamOptionsParse(ConnectionInfo* ciP, const char* value)
{
  std::vector<std::string> vec;

  stringSplit(value, ',', vec);

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    if (!isValidOption(vec[ix]))
    {
      return -1;
    }

    ciP->uriParamOptions[vec[ix]] = true;
  }

  //
  // Check of invalid combinations
  //
  if (ciP->uriParamOptions[OPT_KEY_VALUES]    && ciP->uriParamOptions[OPT_VALUES])        return -1;
  if (ciP->uriParamOptions[OPT_KEY_VALUES]    && ciP->uriParamOptions[OPT_UNIQUE_VALUES]) return -1;
  if (ciP->uriParamOptions[OPT_UNIQUE_VALUES] && ciP->uriParamOptions[OPT_VALUES])        return -1;

  return 0;
}



static inline Verb strToVerb(const std::string &str)
{
  if (str == "POST")
    return POST;
  else if (str == "PUT")
    return PUT;
  else if (str == "GET")
    return GET;
  else if (str == "DELETE")
    return DELETE;
  else if (str == "PATCH")
    return PATCH;
  return NOVERB;
}

// ConnectionInfo class implementation

ConnectionInfo::ConnectionInfo(const std::string &_api, Format _format, bool _ws):
  connection             (NULL),
  verb                   (NOVERB),
  inFormat               (_format),
  outFormat              (_format),
  payload                (NULL),
  payloadSize            (0),
  callNo                 (1),
  parseDataP             (NULL),
  port                   (0),
  apiVersion             (_api),
  inCompoundValue        (false),
  compoundValueP         (NULL),
  compoundValueRoot      (NULL),
  httpStatusCode         (SccOk)
{
  memset(payloadWord, 0, sizeof(payloadWord));

  if (_ws)
  {
    version = "HTTP/1.1";
    servicePath = "/";

    httpHeaders.gotHeaders = true;
    httpHeaders.userAgent = "orionWS/0.1";
    httpHeaders.accept = "*/*";
    httpHeaders.contentLength = 0;
    httpHeaders.servicePath = "/";

    uriParam["details"] = "off";
    uriParam["limit"] = "20";
    uriParam["notifyFormat"] = "JSON";
    uriParam["offset"] = "0";
  }

}

ConnectionInfo::ConnectionInfo(Format _outFormat):
  connection             (NULL),
  verb                   (NOVERB),
  inFormat               (JSON),
  outFormat              (_outFormat),
  payload                (NULL),
  payloadSize            (0),
  callNo                 (1),
  parseDataP             (NULL),
  port                   (0),
  apiVersion             ("v1"),
  inCompoundValue        (false),
  compoundValueP         (NULL),
  compoundValueRoot      (NULL),
  httpStatusCode         (SccOk)
{
  memset(payloadWord, 0, sizeof(payloadWord));
}

ConnectionInfo::ConnectionInfo(const std::string &_url, const std::string &_method, const std::string &_version, MHD_Connection* _connection):
  connection             (_connection),
  verb                   (NOVERB),
  inFormat               (JSON),
  outFormat              (JSON),
  url                    (_url),
  method                 (_method),
  version                (_version),
  payload                (NULL),
  payloadSize            (0),
  callNo                 (1),
  parseDataP             (NULL),
  port                   (0),
  apiVersion             ("v1"),
  inCompoundValue        (false),
  compoundValueP         (NULL),
  compoundValueRoot      (NULL),
  httpStatusCode         (SccOk)
{

  memset(payloadWord, 0, sizeof(payloadWord));
  verb = strToVerb(_method);
}

ConnectionInfo::~ConnectionInfo()
{
  if (compoundValueRoot != NULL)
    delete compoundValueRoot;

  servicePathV.clear();
}


/* ****************************************************************************
*
* reset - Reset some class memeber to use in a websocket connection
*/
void ConnectionInfo::reset()
{
  version = "HTTP/1.1";
  servicePath = "/";

  httpHeaders.gotHeaders = true;
  httpHeaders.userAgent = "orionWS/0.1";
  httpHeaders.accept = "*/*";
  httpHeaders.contentLength = 0;
  httpHeaders.servicePath = "/";
  httpHeaders.tenant.clear();

  uriParam["details"] = "off";
  uriParam["limit"] = "20";
  uriParam["notifyFormat"] = "JSON";
  uriParam["offset"] = "0";

  tenant.clear();
  tenantFromHttpHeader.clear();
}


/* ****************************************************************************
*
* modify - Modify a ConnectionInfo using the given parameters
*/

void ConnectionInfo::modify(const std::string &_url, const std::string &_verb, const std::string &_payload)
{
  url = _url;
  method = _verb;

  verb = strToVerb(_verb);
  servicePathV.clear();

  tenant = httpHeaders.tenant;
  tenantFromHttpHeader = httpHeaders.tenant;
  servicePath = httpHeaders.servicePath;
  servicePathV.push_back(httpHeaders.servicePath);

  if (payload)
  {
    free(payload);
    payload = NULL;
    payloadSize = 0;
    httpHeaders.contentType.clear();
    httpHeaders.contentLength = 0;
  }

  if (!_payload.empty())
  {
    payload = strdup(_payload.c_str());
    payloadSize = _payload.size();
    httpHeaders.contentType = "application/json";
  }

  uriParam["details"] = "off";
  uriParam["limit"] = "20";
  uriParam["notifyFormat"] = "JSON";
  uriParam["offset"] = "0";

  inFormat = outFormat = JSON;

}


/* ****************************************************************************
*
* uriParamTypesParse - parse the URI param 'type' into uriParamTypes vector
*/
void uriParamTypesParse(ConnectionInfo* ciP, const char* value)
{
  std::vector<std::string> vec;

  stringSplit(value, ',', vec);

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    ciP->uriParamTypes.push_back(vec[ix]);
  }

}
