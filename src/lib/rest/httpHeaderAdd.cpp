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
#include <stdio.h>                                               // snprintf

#include "logMsg/logMsg.h"

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/types/OrionldHeader.h"                         // orionldHeaderAdd
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContext
#include "rest/httpHeaderAdd.h"                                  // Own interface



// -----------------------------------------------------------------------------
//
// httpHeaderLocationAdd -
//
void httpHeaderLocationAdd(const char* uriPathWithSlash, const char* entityId, const char* tenant)
{
  char location[512];

  if (entityId != NULL)
    snprintf(location, sizeof(location), "%s%s", uriPathWithSlash, entityId);
  else
    snprintf(location, sizeof(location), "%s", uriPathWithSlash);

  orionldHeaderAdd(&orionldState.out.headers, HttpLocation, location, 0);

  if (tenant != NULL)
    orionldHeaderAdd(&orionldState.out.headers, HttpTenant, tenant, 0);
}



// ----------------------------------------------------------------------------
//
// httpHeaderLinkAdd -
//
void httpHeaderLinkAdd(const char* _url)
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
    url = coreContextUrl;
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
  if (urlLen > sizeof(link) + LINK_REL_AND_TYPE_SIZE + 5)
  {
    linkP = (char*) malloc(urlLen + LINK_REL_AND_TYPE_SIZE + 5);
    if (linkP == NULL)
    {
      LM_E(("Out-of-memory allocating room for HTTP Link Header"));
      return;
    }
    freeLinkP = true;
  }

  sprintf(linkP, "<%s>; %s", url, LINK_REL_AND_TYPE);

  orionldHeaderAdd(&orionldState.out.headers, HttpLink, linkP, 0);

  if (freeLinkP == true)
    free(linkP);

  orionldState.linkHeaderAdded = true;
}
