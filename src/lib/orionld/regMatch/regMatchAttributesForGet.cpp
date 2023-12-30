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
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/StringArray.h"                           // StringArray, stringArrayClone
#include "orionld/types/RegCacheItem.h"                          // RegCacheItem
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/kjTree/kjStringValueLookupInArray.h"           // kjStringValueLookupInArray
#include "orionld/regMatch/regMatchAttributesForGet.h"           // Own interface



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
  StringArray*   attrListP,
  const char*    geoProp
)
{
  bool allAttributes = (propertyNamesP == NULL) && (relationshipNamesP == NULL);

  LM_T(LmtDistOpAttributes, ("Creating the union of attributes GET URL-Param vs Registered Attributes"));

  if (allAttributes == true)
  {
    //
    // Note, attrListP can be NULL (if no 'attrs' URL param has been used)
    // However, a return value of NULL means "Nothing matches" and that's exactly what we don't want here
    //
    // So, we'll need an empty StringArray - meaning EVERYTHING matches => don't include 'attrs' URI param in the forwarded request
    // That comes a few lines down (right after allocating the StringArray, see 'if (allAttributes == true)')
    //
    // Now, if the query is with an "attrs" parameter, we'll create a copy of it for this DistOp
    //
    if (attrListP != NULL)
    {
      LM_T(LmtDistOpAttributes, ("Keeping the URL-Param Attributes as the registration has no attributes specified"));
      return stringArrayClone(attrListP);
    }
  }

  StringArray* sList = (StringArray*) kaAlloc(&orionldState.kalloc, sizeof(StringArray));
  int          items = 0;

  if (allAttributes == true)  // We know that attrListP == NULL (otherwise it would have returned already a few lines up)
  {
    // Everything matches - return an empty array
    sList->items = 0;
    sList->array = NULL;
    LM_T(LmtDistOpAttributes, ("Using ALL Attributes as the registration has no attributes specified and the Query also not"));
    return sList;
  }
  else if ((attrListP != NULL) && (attrListP->items > 0))
    items = attrListP->items;
  else
  {
    // Count items in propertyNamesP + relationshipNamesP
    int properties    = (propertyNamesP     != NULL)? kjArrayItems(propertyNamesP) : 0;
    int relationships = (relationshipNamesP != NULL)? kjArrayItems(relationshipNamesP) : 0;

    items = properties + relationships;
  }

  sList->items = items;
  if (items > 0)
    sList->array = (char**) kaAlloc(&orionldState.kalloc, sizeof(char*) * items);

  if ((attrListP == NULL) || (attrListP->items == 0))
  {
    // Just copy all propertyNames + relationshipNames to sList
    int ix = 0;
    if (propertyNamesP != NULL)
    {
      for (KjNode* pName = propertyNamesP->value.firstChildP; pName != NULL; pName = pName->next)
      {
        LM_T(LmtDistOpAttributes, ("Adding '%s' to the attrList of the DistOp", pName->value.s));
        sList->array[ix++] = pName->value.s;
      }
    }

    if (relationshipNamesP != NULL)
    {
      for (KjNode* rName = relationshipNamesP->value.firstChildP; rName != NULL; rName = rName->next)
      {
        LM_T(LmtDistOpAttributes, ("Adding '%s' to the attrList of the DistOp", rName->value.s));
        sList->array[ix++] = rName->value.s;
      }
    }

    sList->items = ix;
  }
  else
  {
    int matches = 0;
    for (int ix = 0; ix < attrListP->items; ix++)
    {
      bool match = false;

      if (propertyNamesP != NULL)
        match = (kjStringValueLookupInArray(propertyNamesP, attrListP->array[ix]) != NULL);

      if ((match == false) && (relationshipNamesP != NULL))
        match = (kjStringValueLookupInArray(relationshipNamesP, attrListP->array[ix]) != NULL);

      if (match == false)
        continue;

      LM_T(LmtDistOpAttributes, ("Adding '%s' to the attrList of the DistOp", attrListP->array[ix]));
      sList->array[matches++]  = attrListP->array[ix];

      if (regP->mode == RegModeExclusive)
        stringArrayRemoveItem(attrListP, ix);
    }

    if (matches == 0)
      return NULL;

    sList->items = matches;
  }

  LM_T(LmtDistOpAttributes, ("Returning an attrList of %d items", sList->items));
  return sList;
}
