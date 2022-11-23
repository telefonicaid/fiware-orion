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
extern "C"
{
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/types/StringArray.h"                           // StringArray
#include "orionld/kjTree/kjStringValueLookupInArray.h"           // kjStringValueLookupInArray
#include "orionld/regCache/RegCache.h"                           // RegCacheItem
#include "orionld/forwarding/regMatchAttributesForGet.h"         // Own interface



// -----------------------------------------------------------------------------
//
// kjArrayItems -
//
int kjArrayItems(KjNode* arrayP)
{
  int items = 0;

  for (KjNode* itemP = arrayP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    ++items;
  }

  return items;
}



// -----------------------------------------------------------------------------
//
// stringArrayRemoveItem -
//
static bool stringArrayRemoveItem(StringArray* saP, int ix)
{
  if ((saP == NULL) || (ix >= saP->items))
    return false;

  //
  // We remove the ix slot by replacing it with the LAST slot, which is shrunk away
  //
  saP->array[ix] = saP->array[saP->items - 1];
  saP->items -= 1;

  return true;
}



// -----------------------------------------------------------------------------
//
// stringArrayClone - FIXME: Move to common/stringArrayClone.h/cpp
//
StringArray* stringArrayClone(StringArray* attrV)
{
  StringArray* clone = (StringArray*) kaAlloc(&orionldState.kalloc, sizeof(StringArray));

  clone->items = attrV->items;
  clone->array = (char**) kaAlloc(&orionldState.kalloc, attrV->items * sizeof(char*));

  for (int ix = 0; ix < clone->items; ix++)
  {
    clone->array[ix] = kaStrdup(&orionldState.kalloc, attrV->array[ix]);
  }

  return clone;
}



// -----------------------------------------------------------------------------
//
// regMatchAttributesForGet -
//
// RETURN VALUE
//   Empty StringArray        All attributes match
//   Non-Empty StringArray    Some attributes match
//   NULL                     Nothing matches
//
StringArray* regMatchAttributesForGet
(
  RegCacheItem*  regP,
  KjNode*        propertyNamesP,
  KjNode*        relationshipNamesP,
  StringArray*   attrV,
  const char*    geoProp
)
{
  bool        allAttributes = (propertyNamesP == NULL) && (relationshipNamesP == NULL);

  if (allAttributes == true)
  {
    //
    // Note, attrV can be NULL (if no 'attrs' URL param has been used)
    // However, a return value of NULL means "Nothing matches" and that's exactly what we don't want here
    //
    // So, we'll need an empty StringArray - meaning EVERYTHING matches => don't include 'attrs' URI param in the forwarded request
    // That comes a few lines down (right after allocating the StringArray, see 'if (allAttributes == true)')
    //
    if (attrV != NULL)
      return stringArrayClone(attrV);
  }

  StringArray* sList = (StringArray*) kaAlloc(&orionldState.kalloc, sizeof(StringArray));
  int          items;

  if (allAttributes == true)  // We know that attrV == NULL in such case (see a few lines up)
  {
    // Everything matches - return an empty array
    sList->items = 0;
    sList->array = NULL;
    return sList;
  }
  else if (attrV != NULL)
    items = attrV->items;
  else
  {
    // Count items in propertyNamesP + relationshipNamesP
    int properties    = (propertyNamesP     != NULL)? kjArrayItems(propertyNamesP) : 0;
    int relationships = (relationshipNamesP != NULL)? kjArrayItems(relationshipNamesP) : 0;

    items = properties + relationships;
  }

  sList->items = items;
  sList->array = (char**) kaAlloc(&orionldState.kalloc, sizeof(char*) * items);

  if (attrV == NULL)
  {
    // Just copy all propertyNames + relationshipNames to sList
    int ix = 0;
    if (propertyNamesP != NULL)
    {
      for (KjNode* pName = propertyNamesP->value.firstChildP; pName != NULL; pName = pName->next)
      {
        sList->array[ix++] = pName->value.s;
      }
    }

    if (relationshipNamesP != NULL)
    {
      for (KjNode* rName = relationshipNamesP->value.firstChildP; rName != NULL; rName = rName->next)
      {
        sList->array[ix++] = rName->value.s;
      }
    }

    sList->items = ix;
  }
  else
  {
    int matches = 0;
    for (int ix = 0; ix < attrV->items; ix++)
    {
      bool match = false;

      if (propertyNamesP != NULL)
        match = (kjStringValueLookupInArray(propertyNamesP, attrV->array[ix]) != NULL);

      if ((match == false) && (relationshipNamesP != NULL))
        match = (kjStringValueLookupInArray(relationshipNamesP, attrV->array[ix]) != NULL);

      if (match == false)
        continue;

      sList->array[matches++]  = attrV->array[ix];

      if (regP->mode == RegModeExclusive)
        stringArrayRemoveItem(attrV, ix);
    }

    if (matches == 0)
      return NULL;

    sList->items = matches;
  }

  return sList;
}
