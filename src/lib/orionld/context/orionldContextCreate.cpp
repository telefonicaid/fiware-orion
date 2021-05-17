/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
extern "C"
{
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/kjClone.h"                                       // kjClone
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/context/OrionldContext.h"                      // OrionldContext
#include "orionld/common/orionldState.h"                         // orionldState, kalloc



// -----------------------------------------------------------------------------
//
// orionldContextOriginName - FIXME: move to its own module
//
const char* orionldContextOriginName(OrionldContextOrigin origin)
{
  switch (origin)
  {
  case OrionldContextUnknownOrigin:     return "UnknownOrigin";
  case OrionldContextFromInline:        return "Inline";
  case OrionldContextDownloaded:        return "Downloaded";
  case OrionldContextFileCached:        return "FileCached";
  case OrionldContextForNotifications:  return "ForNotifications";
  case OrionldContextForForwarding:     return "ForForwarding";
  case OrionldContextUserCreated:       return "UserCreated";
  }

  return "Unknown Origin II";
}



// -----------------------------------------------------------------------------
//
// orionldOriginFromString - FIXME: move to its own module
//
OrionldContextOrigin orionldOriginFromString(const char* s)
{
  if      (strcmp(s, "UnknownOrigin")    == 0) return OrionldContextUnknownOrigin;
  else if (strcmp(s, "Inline")           == 0) return OrionldContextFromInline;
  else if (strcmp(s, "Downloaded")       == 0) return OrionldContextDownloaded;
  else if (strcmp(s, "FileCached")       == 0) return OrionldContextFileCached;
  else if (strcmp(s, "ForNotifications") == 0) return OrionldContextForNotifications;
  else if (strcmp(s, "ForForwarding")    == 0) return OrionldContextForForwarding;
  else if (strcmp(s, "UserCreated")      == 0) return OrionldContextUserCreated;

  return OrionldContextUnknownOrigin;
}



// -----------------------------------------------------------------------------
//
// orionldContextCreate -
//
OrionldContext* orionldContextCreate(const char* url, OrionldContextOrigin origin, const char* id, KjNode* tree, bool keyValues, bool toBeCloned)
{
  OrionldContext* contextP = (OrionldContext*) kaAlloc(&kalloc, sizeof(OrionldContext));

  if (contextP == NULL)
    LM_X(1, ("out of memory - trying to allocate a OrionldContext of %d bytes", sizeof(OrionldContext)));

  contextP->url       = (url == NULL)? (char*) "no URL" : kaStrdup(&kalloc, url);
  contextP->origin    = origin;
  contextP->parent    = NULL;
  contextP->id        = (id == NULL)? NULL : kaStrdup(&kalloc, id);
  contextP->tree      = kjClone(NULL, tree);
  contextP->keyValues = keyValues;
  contextP->lookups   = 0;

  return contextP;
}
