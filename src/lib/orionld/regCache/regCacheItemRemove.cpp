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
#include <unistd.h>                                              // NULL
#include <regex.h>                                               // regfree

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjFree.h"                                        // kjFree
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/RegCacheItem.h"                          // RegCacheItem
#include "orionld/regCache/regCacheItemRegexRelease.h"           // regCacheItemRegexRelease
#include "orionld/regCache/regCacheItemRemove.h"                 // Own interface


// -----------------------------------------------------------------------------
//
// regCacheItemRemove -
//
// THE REGISTRATION CACHE IS MEANT FOR ENTITY UPDATES AND RETRIEVAL FASTER, NOT FOR MAINTAINING REGISTRATIONS.
//
// It's OK for this function to be slow.
// It will rarely be used.
//
// The function would be much faster if the Registration id was part of RegCacheItem, but ...
// that would require more RAM.
// Or even, if we used a hash-table for faster lookups.
//
// BUT, looking up an individual registration for patching, deletion or whatever is FAR FROM the
// main use of the registration cache.
//
bool regCacheItemRemove(RegCache* rcP, const char* regId)
{
  if (rcP == NULL)
    LM_RE(false, ("NULL rcP - that's a SW bug!"));

  RegCacheItem* rciP = rcP->regList;
  RegCacheItem* prev = NULL;

  while (rciP != NULL)
  {
    KjNode* idP = kjLookup(rciP->regTree, "id");

    if ((idP != NULL) && (strcmp(idP->value.s, regId) == 0))
    {
      // Remove rciP from rcP
      if (rciP == rcP->regList)  // First item is the item to remove - step over it
      {
        rcP->regList = rciP->next;
        if (rcP->regList == NULL)
          rcP->last = NULL;
      }
      else if (rciP->next == NULL)  // Last item is the item to remove
      {
        if (prev != NULL)
          prev->next = NULL;    // End the list right there
        else
          rcP->regList = NULL;  // First and last item is the same - list is emptied
      }
      else  // In the middle
        prev->next = rciP->next;  // Just step over it

      // Free the reg-cache item to be deleted (call regCacheItemRelease(rciP)?)
      if (rciP->regId != NULL)
        free(rciP->regId);

      kjFree(rciP->regTree);

      // In case we have any regex's, free them
      if (rciP->idPatternRegexList != NULL)
        regCacheItemRegexRelease(rciP);

      if (rciP->ipAndPort != NULL)
        free(rciP->ipAndPort);

      if ((rciP->hostAlias != NULL) && (rciP->hostAlias != rciP->ipAndPort))
        free(rciP->hostAlias);

      // And finally, free the entire struct
      free(rciP);

      return true;
    }

    prev = rciP;
    rciP = rciP->next;
  }

  return false;
}
