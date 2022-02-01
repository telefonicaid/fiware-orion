/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
#include <string.h>                                              // strstr, strlen
#include <unistd.h>                                              // NULL

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/types/OrionldAttributeType.h"                  // OrionldAttributeType
#include "orionld/common/attributeTypeFromUriParam.h"            // Own interface



// -----------------------------------------------------------------------------
//
// isHit -
//
inline bool isHit(char* list, const char* item, int itemLen)
{
  if (list == NULL)
    return false;

  char* hit = strstr(list, item);

  if (hit != NULL)  // Make sure it's really a hit
  {
    if ((hit[itemLen] == 0) || (hit[itemLen] == ','))
      return true;
  }

  return false;
}



// -----------------------------------------------------------------------------
//
// attributeTypeFromUriParam -
//
OrionldAttributeType attributeTypeFromUriParam(const char* attrShortName)
{
  int attrShortNameLen = strlen(attrShortName);

  if (isHit(orionldState.uriParams.relationships,      attrShortName, attrShortNameLen) == true)    return Relationship;
  if (isHit(orionldState.uriParams.geoproperties,      attrShortName, attrShortNameLen) == true)    return GeoProperty;
  if (isHit(orionldState.uriParams.languageproperties, attrShortName, attrShortNameLen) == true)    return LanguageProperty;

  return Property;
}
