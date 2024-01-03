/*
*
* Copyright 2024 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
#include "logMsg/logMsg.h"

#include "orionld/types/OrionldHeader.h"                         // orionldHeaderAdd
#include "orionld/common/orionldState.h"                         // orionldState, coreContextUrl
#include "orionld/http/http.h"                                   // LINK_REL_AND_TYPE_SIZE
#include "orionld/http/httpHeaderLinkAdd.h"                      // Own interface



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
  if (urlLen + LINK_REL_AND_TYPE_SIZE + 5 > sizeof(link))
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
