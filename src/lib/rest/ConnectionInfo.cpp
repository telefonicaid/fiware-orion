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
#include "orionld/context/orionldCoreContext.h"
#include "orionld/context/orionldContextFree.h"
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
#ifdef ORIONLD
  , OPT_SYS_ATTRS
#endif
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
  contextP                  (NULL),
  contextToBeFreed          (false),
  prettyPrint               (false),
  prettyPrintSpaces         (2)
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
  contextP                  (NULL),
  contextToBeFreed          (false),
  prettyPrint               (false),
  prettyPrintSpaces         (2)
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
  contextP                  (NULL),
  contextToBeFreed          (false),
  prettyPrint               (false),
  prettyPrintSpaces         (2)
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



/* ****************************************************************************
*
* ConnectionInfo::~ConnectionInfo - 
*/
ConnectionInfo::~ConnectionInfo()
{
  if (compoundValueRoot != NULL)
  {
    delete compoundValueRoot;
    compoundValueRoot = NULL;
  }

  servicePathV.clear();
  httpHeaders.release();

#ifdef ORIONLD
  LM_TMP(("In ~ConnectionInfo"));
  if (requestTree != NULL)
  {
    LM_TMP(("Releasing requestTree at %p", requestTree));
    kjFree(requestTree);
    requestTree = NULL;
  }

  if (responseTree != NULL)
  {
    kjFree(responseTree);
    responseTree = NULL;
  }

  if ((contextP != NULL) && (contextP != &orionldCoreContext) && (contextToBeFreed == true))
  {
    LM_TMP(("Freeing REQUEST context '%s' at %p, tree at %p", contextP->url, contextP, contextP->tree));
    // The tree is part of the requestTree, so it's already freed
    free(contextP->url);
    free(contextP);
    contextP = NULL;
  }

  if (kjsonP != NULL)
  {
    LM_TMP(("Freeing kjsonP. iVec at %p", kjsonP->iVec));
    if (kjsonP->iVec != NULL)
      free(kjsonP->iVec);

    LM_TMP(("Freeing kjsonP"));
    if (kjsonP->iStrings != NULL)
      free(kjsonP->iStrings);
  
    LM_TMP(("Freeing kjsonP"));
    free(kjsonP);
    LM_TMP(("Freed kjsonP"));
    kjsonP = NULL;
  }
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
