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

#ifdef ORIONLD
#include "orionld/context/orionldDefaultContext.h"
#endif



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
  OPT_DATE_MODIFIED,
  OPT_NO_ATTR_DETAIL,
  OPT_UPSERT
};



/* ****************************************************************************
*
* ConnectionInfo::ConnectionInfo - 
*/
ConnectionInfo::ConnectionInfo():
  connection             (NULL),
  verb                   (NOVERB),
  badVerb                (false),
  inMimeType             (JSON),
  outMimeType            (JSON),
  tenant                 (""),
  restServiceP           (NULL),
  payload                (NULL),
  payloadSize            (0),
  callNo                 (1),
  parseDataP             (NULL),
  port                   (0),
  ip                     (""),
  apiVersion             (V1),
  transactionStart       { 0, 0 },
  inCompoundValue        (false),
  compoundValueP         (NULL),
  compoundValueRoot      (NULL),
  httpStatusCode         (SccOk),
#ifdef ORIONLD
  serviceP                  (NULL),
  responsePayload           (NULL),
  responsePayloadAllocated  (false),
  urlPath                   (NULL),
  wildcard                  { NULL, NULL},
  kjsonP                    (NULL),
  requestTree               (NULL),
  responseTree              (NULL),
  contextP                  (NULL)
#endif
{
}



/* ****************************************************************************
*
* ConnectionInfo::ConnectionInfo - 
*/
ConnectionInfo::ConnectionInfo(MimeType _outMimeType):
  connection             (NULL),
  verb                   (NOVERB),
  badVerb                (false),
  inMimeType             (JSON),
  outMimeType            (_outMimeType),
  tenant                 (""),
  restServiceP           (NULL),
  payload                (NULL),
  payloadSize            (0),
  callNo                 (1),
  parseDataP             (NULL),
  port                   (0),
  ip                     (""),
  apiVersion             (V1),
  transactionStart       { 0, 0 },
  inCompoundValue        (false),
  compoundValueP         (NULL),
  compoundValueRoot      (NULL),
  httpStatusCode         (SccOk),
#ifdef ORIONLD
  serviceP                  (NULL),
  responsePayload           (NULL),
  responsePayloadAllocated  (false),
  urlPath                   (NULL),
  wildcard                  { NULL, NULL},
  kjsonP                    (NULL),
  requestTree               (NULL),
  responseTree              (NULL),
  contextP                  (NULL)
#endif
{
}



/* ****************************************************************************
*
* ConnectionInfo::ConnectionInfo - 
*/
ConnectionInfo::ConnectionInfo(std::string _url, std::string _method, std::string _version, MHD_Connection* _connection):
  connection             (_connection),
  verb                   (NOVERB),
  badVerb                (false),
  inMimeType             (JSON),
  outMimeType            (JSON),
  url                    (_url),
  method                 (_method),
  version                (_version),
  tenant                 (""),
  restServiceP           (NULL),
  payload                (NULL),
  payloadSize            (0),
  callNo                 (1),
  parseDataP             (NULL),
  port                   (0),
  ip                     (""),
  apiVersion             (V1),
  transactionStart       { 0, 0 },
  inCompoundValue        (false),
  compoundValueP         (NULL),
  compoundValueRoot      (NULL),
  httpStatusCode         (SccOk),
#ifdef ORIONLD
  serviceP                  (NULL),
  responsePayload           (NULL),
  responsePayloadAllocated  (false),
  urlPath                   (NULL),
  wildcard                  { NULL, NULL},
  kjsonP                    (NULL),
  requestTree               (NULL),
  responseTree              (NULL),
  contextP                  (NULL)
#endif
{
  if      (_method == "POST")    verb = POST;
  else if (_method == "PUT")     verb = PUT;
  else if (_method == "GET")     verb = GET;
  else if (_method == "DELETE")  verb = DELETE;
  else if (_method == "PATCH")   verb = PATCH;
  else if (_method == "OPTIONS") verb = OPTIONS;
  else
  {
    badVerb = true;
    verb    = NOVERB;
  }
}



ConnectionInfo::~ConnectionInfo()
{
  if (compoundValueRoot != NULL)
  {
    delete compoundValueRoot;
  }

  servicePathV.clear();
  httpHeaders.release();

#ifdef ORIONLD
  if (requestTree != NULL)
  {
    LM_T(LmtFree, ("Calling kjFree for ciP->requestTree at %p", requestTree));
    kjFree(requestTree);
    LM_T(LmtFree, ("kjFree'd ciP->requestTree"));
  }

  if (responseTree != NULL)
  {
    LM_T(LmtFree, ("Calling kjFree for ciP->responseTree at %p", responseTree));
    kjFree(responseTree);
    LM_T(LmtFree, ("kjFree'd ciP->responseTree"));
  }

  if ((contextP != NULL) && (contextP != &orionldDefaultContext))
  {
    // contextP->tree point part of to the request payload - already freed by the call to "kjFree(requestTree)"
    LM_T(LmtFree, ("Freeing context '%s' at: %p", contextP->url, contextP));
    free(contextP);
    LM_T(LmtFree, ("NOT Freed contextP at: %p", contextP));
  }

  LM_T(LmtFree, ("Freeing ciP->kjsonP at %p", kjsonP));
  free(kjsonP);
  LM_T(LmtFree, ("Freed ciP->kjsonP at %p", kjsonP));
#endif
}



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
