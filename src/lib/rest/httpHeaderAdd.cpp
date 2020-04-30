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
#include <vector>

#include "orionld/common/orionldState.h"                         // orionldState

#include "rest/ConnectionInfo.h"
#include "rest/httpHeaderAdd.h"



// -----------------------------------------------------------------------------
//
// httpHeaderAdd -
//
void httpHeaderAdd(ConnectionInfo* ciP, const char* key, const char* value)
{
  ciP->httpHeader.push_back(key);
  ciP->httpHeaderValue.push_back(value);
}



#ifdef ORIONLD
#include "orionld/context/orionldCoreContext.h" // orionldCoreContext

// -----------------------------------------------------------------------------
//
// httpHeaderLocationAdd -
//
void httpHeaderLocationAdd(ConnectionInfo* ciP, const char* uriPathWithSlash, const char* entityId)
{
  char location[512];

  snprintf(location, sizeof(location), "%s%s", uriPathWithSlash, entityId);

  ciP->httpHeader.push_back(HTTP_RESOURCE_LOCATION);
  ciP->httpHeaderValue.push_back(location);
}



// ----------------------------------------------------------------------------
//
// httpHeaderLinkAdd -
//
void httpHeaderLinkAdd(ConnectionInfo* ciP, const char* _url)
{
  char             link[256];
  char*            linkP = link;
  char*            url;
  unsigned int     urlLen;
  bool             freeLinkP = false;

  if (orionldState.linkHeaderAdded == true)
    return;

  // If no context URL is given, the Core Context is used
  if (_url == NULL)
    url = ORIONLD_CORE_CONTEXT_URL;
  else
  {
    url = (char*) _url;

    if (url[0] == '<')
    {
      url = &url[1];

      // Find closing '>' and terminate the string
      char* cP = url;

      while ((*cP != 0) && (*cP != '>'))
        ++cP;

      if (*cP == '>')
        *cP = 0;
    }
  }

  urlLen = strlen(url);
  if (urlLen > sizeof(link) + LINK_REL_AND_TYPE_SIZE + 3)
  {
    linkP = (char*) malloc(sizeof(link) + LINK_REL_AND_TYPE_SIZE + 3);
    if (linkP == NULL)
    {
      LM_E(("Out-of-memory allocating roome for HTTP Link Header"));
      return;
    }
    freeLinkP = true;
  }

  sprintf(linkP, "<%s>; %s", url, LINK_REL_AND_TYPE);

  ciP->httpHeader.push_back("Link");
  ciP->httpHeaderValue.push_back(linkP);

  if (freeLinkP == true)
    free(linkP);

  orionldState.linkHeaderAdded = true;
}
#endif
