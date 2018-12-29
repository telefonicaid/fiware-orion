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
      return (char*) &longName[orionldDefaultUrlLen];
  }

  LM_T(LmtAlias, ("Calling orionldContextValueLookup for %s", longName));
  bool    useStringValue = false;
  KjNode* aliasNodeP     = orionldContextValueLookup(contextP, longName, &useStringValue);

  if (aliasNodeP != NULL)
  {
    char* alias = (useStringValue == false)? aliasNodeP->name : aliasNodeP->value.s;

    LM_T(LmtAlias, ("Found the alias: '%s' => '%s'", longName, alias));
    return alias;
  }

  LM_T(LmtAlias, ("No alias found, keeping long name '%s'", longName));
  return (char*) longName;
}
