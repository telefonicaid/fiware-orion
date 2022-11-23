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
#include <string.h>                                              // strcmp

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove, kjChildAdd
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/forwarding/fwdEntityMerge.h"                   // Own interface



// -----------------------------------------------------------------------------
//
// timestampMerge -
//
static void timestampMerge(KjNode* apiEntityP, KjNode* additionP, KjNode* currentTsP, KjNode* newTsP, const char* tsName, bool newReplaces)
{
  if (currentTsP != NULL)  // This should always be the case
  {
    bool newIsNewer = strcmp(currentTsP->value.s, newTsP->value.s) < 0;

    if (newReplaces == newIsNewer)
      currentTsP->value.s = newTsP->value.s;
  }
  else
  {
    kjChildRemove(additionP, newTsP);
    kjChildAdd(apiEntityP, newTsP);
  }
}



// -----------------------------------------------------------------------------
//
// newerAttribute -
//
static KjNode* newerAttribute(KjNode* currentP, KjNode* pretenderP)
{
  KjNode* currentObservedAt   = kjLookup(currentP,   "observedAt");
  KjNode* pretenderObservedAt = kjLookup(pretenderP, "observedAt");

  if ((currentObservedAt == NULL) && (pretenderObservedAt == NULL))
  {
    KjNode* currentModifiedAt   = kjLookup(currentP,   "modifiedAt");
    KjNode* pretenderModifiedAt = kjLookup(pretenderP, "modifiedAt");

    if ((currentModifiedAt != NULL) && (pretenderModifiedAt != NULL))
    {
      if (strcmp(currentModifiedAt->value.s, pretenderModifiedAt->value.s) >= 0)
        return currentP;
      else
        return pretenderP;
    }
    else if (currentModifiedAt != NULL)
      return currentP;
    else if (pretenderModifiedAt != NULL)
      return pretenderP;
    else
      return currentP;  // Just pick one ...
  }
  else if (currentObservedAt == NULL)
    return pretenderP;
  else if (pretenderObservedAt == NULL)
    return currentP;
  else  // both non-NULL
  {
    if (strcmp(currentObservedAt->value.s, pretenderObservedAt->value.s) >= 0)
      return currentP;
    else
      return pretenderP;
  }

  LM_W(("Not sure how we got here ... keeping the current - no replace"));
  return currentP;
}



// -----------------------------------------------------------------------------
//
// fwdEntityMerge -
//
bool fwdEntityMerge(KjNode* apiEntityP, KjNode* additionP, bool sysAttrs, bool auxiliary)
{
  KjNode* idP             = kjLookup(additionP,  "id");
  KjNode* typeP           = kjLookup(additionP,  "type");

  if (idP)
    kjChildRemove(additionP, idP);
  if (typeP)
    kjChildRemove(additionP, typeP);

  if (sysAttrs == true)  // sysAttrs for the final response
  {
    KjNode* addCreatedAtP   = kjLookup(additionP,  "createdAt");
    KjNode* addModifiedAtP  = kjLookup(additionP,  "modifiedAt");
    KjNode* createdAtP      = kjLookup(apiEntityP, "createdAt");
    KjNode* modifiedAtP     = kjLookup(apiEntityP, "modifiedAt");

    if (addCreatedAtP != NULL)
      timestampMerge(apiEntityP, additionP, createdAtP,  addCreatedAtP,  "createdAt",  false);   // false: replace if older
    if (addModifiedAtP != NULL)
      timestampMerge(apiEntityP, additionP, modifiedAtP, addModifiedAtP, "modifiedAt", true);    // true: replace if newer
  }

  KjNode* attrP   = additionP->value.firstChildP;
  KjNode* next;

  while (attrP != NULL)
  {
    next = attrP->next;

    KjNode* currentP = kjLookup(apiEntityP, attrP->name);

    if (currentP == NULL)
    {
      kjChildRemove(additionP, attrP);
      kjChildAdd(apiEntityP, attrP);
    }
    else if (auxiliary == false)  // two copies of the same attr ...  and NOT from an auxiliary registration
    {
      if (newerAttribute(currentP, attrP) == attrP)
      {
        kjChildRemove(apiEntityP, currentP);
        kjChildRemove(additionP, attrP);
        kjChildAdd(apiEntityP, attrP);
      }
    }

    attrP = next;
  }

  return true;
}
