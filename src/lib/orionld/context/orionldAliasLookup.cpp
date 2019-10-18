/*
*
* Copyright 2018 FIWARE Foundation e.V.
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
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/context/OrionldContext.h"                    // OrionldContext
#include "orionld/context/orionldCoreContext.h"                // orionldDefaultUrl, orionldDefaultUrlLen
#include "orionld/context/orionldContextValueLookup.h"         // orionldContextValueLookup
#include "orionld/context/orionldAliasLookup.h"                // Own interface



// -----------------------------------------------------------------------------
//
// orionldAliasLookup -
//
char* orionldAliasLookup(OrionldContext* contextP, const char* longName, bool* valueMayBeContractedP)
{
  LM_T(LmtAlias, ("VAL: =================== Reverse alias-search for '%s'", longName));

  if (valueMayBeContractedP != NULL)
    *valueMayBeContractedP = false;

  // Is it the default URL ?
  if (orionldDefaultUrlLen != -1)
  {
    if (strncmp(longName, orionldDefaultUrl, orionldDefaultUrlLen) == 0)
    {
      LM_T(LmtAlias, ("VAL: Default URL detected. Returning: '%s'", &longName[orionldDefaultUrlLen]));
      return (char*) &longName[orionldDefaultUrlLen];
    }
  }

  LM_T(LmtAlias, ("VAL: Calling orionldContextValueLookup for long-name '%s'", longName));
  LM_T(LmtContextValueLookup, ("VAL:"));
  LM_T(LmtContextValueLookup, ("VAL: =============================== Calling orionldContextValueLookup for '%s'", longName));
  KjNode* aliasNodeP     = orionldContextValueLookup(contextP, longName);
  LM_T(LmtContextValueLookup, ("VAL: =================================================================================================="));

  if (aliasNodeP != NULL)
  {
    if (aliasNodeP->type == KjObject)
    {
      KjNode* idP = NULL;

      LM_T(LmtAlias, ("VAL: The alias node is an object (named '%s')", aliasNodeP->name));
      // The @id node stores the name - look it up
      for (KjNode* kNodeP = aliasNodeP->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
      {
        if (SCOMPARE4(kNodeP->name, '@', 'i', 'd', 0))
        {
          LM_T(LmtContextValueLookup, ("Found the @id: '%s' (for aliasNode '%s')", kNodeP->value.s, aliasNodeP->name));
          idP = kNodeP;
        }
        else if ((valueMayBeContractedP != NULL) && (SCOMPARE6(kNodeP->name, '@', 't', 'y', 'p', 'e', 0)))
        {
          LM_T(LmtContextValueLookup, ("VAL: Found the @type: '%s' (for aliasNode '%s')", kNodeP->value.s, aliasNodeP->name));
          if (strcmp(kNodeP->value.s, "@vocab") == 0)
          {
            LM_TMP(("VAL: @type == @vocab - value can be contracted"));
            *valueMayBeContractedP = true;
          }
        }
      }

      if (idP != NULL)
        return aliasNodeP->name;  // All OK, as "@id" was present inside the object

      //
      // No "@id" found - alias cannot be used.
      //
      LM_T(LmtAlias, ("VAL: Error in context (@id part is missing), keeping long name '%s'", longName));
      return (char*) longName;
    }
    else
    {
      char* alias = aliasNodeP->name;

      LM_T(LmtAlias, ("VAL: Found the alias: '%s' => '%s'", longName, alias));
      return alias;
    }
  }

  LM_T(LmtAlias, ("VAL: No alias found, keeping long name '%s'", longName));
  return (char*) longName;
}
