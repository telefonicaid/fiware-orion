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
#include <string.h>                                              // strcmp

#include "orionld/types/OrionldContext.h"                        // OrionldContext



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
// orionldOriginToString -
//
const char* orionldOriginToString(OrionldContextOrigin origin)
{
  switch (origin)
  {
  case OrionldContextUnknownOrigin:       return "UnknownOrigin";
  case OrionldContextFromInline:          return "FromInline";
  case OrionldContextDownloaded:          return "Downloaded";
  case OrionldContextFileCached:          return "FileCached";
  case OrionldContextForNotifications:    return "ForNotifications";
  case OrionldContextForForwarding:       return "ForForwarding";
  case OrionldContextUserCreated:         return "UserCreated";
  case OrionldContextBuiltinCoreContext:  return "BuiltinCoreContext";
  }

  return "InvalidOrigin";
}



// -----------------------------------------------------------------------------
//
// orionldKindFromString -
//
OrionldContextKind orionldKindFromString(const char* kind)
{
  if      (strcmp(kind, "Hosted")            == 0)    return OrionldContextHosted;
  else if (strcmp(kind, "Cached")            == 0)    return OrionldContextCached;
  else if (strcmp(kind, "ImplicitlyCreated") == 0)    return OrionldContextImplicit;
  else if (strcmp(kind, "Unknown")           == 0)    return OrionldContextUnknownKind;

  return OrionldContextUnknownKind;
}



// -----------------------------------------------------------------------------
//
// orionldKindToString -
//
const char* orionldKindToString(OrionldContextKind kind)
{
  switch (kind)
  {
  case OrionldContextUnknownKind:  return "Unknown";
  case OrionldContextHosted:       return "Hosted";
  case OrionldContextCached:       return "Cached";
  case OrionldContextImplicit:     return "ImplicitlyCreated";
  }

  return "Invalid";
}
