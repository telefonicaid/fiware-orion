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
char* orionldAliasLookup(OrionldContext* contextP, const char* longName)
{
  LM_T(LmtAlias, ("=================== Reverse alias-search for '%s'", longName));

  // Is it the default URL ?
  if (orionldDefaultUrlLen != -1)
  {
    if (strncmp(longName, orionldDefaultUrl, orionldDefaultUrlLen) == 0)
    {
      LM_T(LmtAlias, ("Default URL detected. Returning: '%s'", &longName[orionldDefaultUrlLen]));
      return (char*) &longName[orionldDefaultUrlLen];
    }
  }

  LM_T(LmtAlias, ("Calling orionldContextValueLookup for long-name '%s'", longName));
  LM_T(LmtContextValueLookup, ("CTX:"));
  LM_T(LmtContextValueLookup, ("CTX: =============================== Calling orionldContextValueLookup for '%s'", longName));
  KjNode* aliasNodeP     = orionldContextValueLookup(contextP, longName);
  LM_T(LmtContextValueLookup, ("CTX: =================================================================================================="));

  if (aliasNodeP != NULL)
  {
    if (aliasNodeP->type == KjObject)
    {
      LM_T(LmtAlias, ("The alias node is an object (named '%s')", aliasNodeP->name));
      // The @id node stores the name - look it up
      for (KjNode* idNodeP = aliasNodeP->value.firstChildP; idNodeP != NULL; idNodeP = idNodeP->next)
      {
        if (SCOMPARE4(idNodeP->name, '@', 'i', 'd', 0))
        {
          LM_T(LmtContextValueLookup, ("Found the @id: '%s' (for aliasNode '%s')", idNodeP->value.s, aliasNodeP->name));
          return aliasNodeP->name;
        }
      }

      LM_T(LmtAlias, ("Error in context (@id part is missing), keeping long name '%s'", longName));
      return (char*) longName;
    }
    else
    {
      char* alias = aliasNodeP->name;

      LM_T(LmtAlias, ("Found the alias: '%s' => '%s'", longName, alias));
      return alias;
    }
  }

  LM_T(LmtAlias, ("No alias found, keeping long name '%s'", longName));
  return (char*) longName;
}
