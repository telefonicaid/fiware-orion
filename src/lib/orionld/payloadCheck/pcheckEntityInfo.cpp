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
#include <regex.h>                                               // regcomp, regfree

extern "C"
{
#include "kjson/KjNode.h"                                       // KjNode
#include "kjson/kjBuilder.h"                                    // kjChildRemove
}

#include "logMsg/logMsg.h"                                      // LM_*
#include "logMsg/traceLevels.h"                                 // Lmt*

#include "orionld/common/CHECK.h"                               // STRING_CHECK, ...
#include "orionld/common/orionldState.h"                        // orionldState
#include "orionld/common/orionldError.h"                        // orionldError
#include "orionld/context/orionldContextItemExpand.h"           // orionldContextItemExpand
#include "orionld/context/orionldContextItemAlreadyExpanded.h"  // orionldContextItemAlreadyExpanded
#include "orionld/payloadCheck/pcheckEntityInfo.h"              // Own interface



// -----------------------------------------------------------------------------
//
// regexCheck -
//
static bool regexCheck(const char* pattern)
{
  regex_t regex;
  bool    r;

  r = regcomp(&regex, pattern, REG_EXTENDED);

  regfree(&regex);

  if (r != 0)
  {
    LM_W(("Invalid regex '%s' - error %d from regcomp", r));
    return false;
  }

  return true;
}



// ----------------------------------------------------------------------------
//
// pcheckEntityInfo -
//
bool pcheckEntityInfo(KjNode* entityInfoP, bool typeMandatory, bool idMandatory, const char** fieldPathV)
{
  KjNode* idP         = NULL;
  KjNode* idPatternP  = NULL;
  KjNode* typeP       = NULL;

  OBJECT_CHECK(entityInfoP, fieldPathV[1]);
  EMPTY_OBJECT_CHECK(entityInfoP, fieldPathV[1]);

  for (KjNode* entityItemP = entityInfoP->value.firstChildP; entityItemP != NULL; entityItemP = entityItemP->next)
  {
    if ((strcmp(entityItemP->name, "id") == 0) || (strcmp(entityItemP->name, "@id") == 0))
    {
      DUPLICATE_CHECK(idP, fieldPathV[2], entityItemP);
      STRING_CHECK(entityItemP, fieldPathV[2]);
      EMPTY_STRING_CHECK(entityItemP, fieldPathV[2]);
      URI_CHECK(entityItemP->value.s, fieldPathV[2], true);
    }
    else if (strcmp(entityItemP->name, "idPattern") == 0)
    {
      DUPLICATE_CHECK(idPatternP, fieldPathV[3], entityItemP);
      STRING_CHECK(entityItemP, fieldPathV[3]);
      EMPTY_STRING_CHECK(entityItemP, fieldPathV[3]);

      //
      // It is not ideal to do the regexcomp TWICE, but it makes the implementation a little simpler - less bugs
      // Really, how often are registrations created/patched?
      //
      if (regexCheck(idPatternP->value.s) == false)
      {
        orionldError(OrionldBadRequestData, "Invalid REGEX in EntityInfo::idPattern", idPatternP->value.s, 400);
        return false;
      }
    }
    else if ((strcmp(entityItemP->name, "type") == 0) || (strcmp(entityItemP->name, "@type") == 0))
    {
      DUPLICATE_CHECK(typeP, fieldPathV[4], entityItemP);
      STRING_CHECK(entityItemP, fieldPathV[4]);
      EMPTY_STRING_CHECK(entityItemP, fieldPathV[4]);
      URI_CHECK(entityItemP->value.s, fieldPathV[4], false);

      //
      // Expand, unless already expanded
      // If a ':' is found inside the first 10 chars, the value is assumed to be expanded ...
      //
      if (orionldContextItemAlreadyExpanded(entityItemP->value.s) == false)
        entityItemP->value.s = orionldContextItemExpand(orionldState.contextP, entityItemP->value.s, true, NULL);  // entity type
    }
    else
    {
      orionldError(OrionldBadRequestData, "Invalid field name in EntityInfo", entityItemP->name, 400);
      return false;
    }
  }

  // If both "id" and "idPattern" are present - "idPattern" is ignored (removed)
  if ((idP != NULL) && (idPatternP != NULL))
    kjChildRemove(entityInfoP, idPatternP);

  // Only if Fully NGSI-LD compliant
  if ((typeMandatory == true) && (typeP == NULL))
  {
    orionldError(OrionldBadRequestData, "Missing mandatory field", fieldPathV[4], 400);
    return false;
  }

  //
  // Only exclusive registrations use idMandatory=true, so, as we need a good error message,
  // it is set right here.
  //
  // I know, it's a bit weird, as this function is also used for subscriptions.
  // Not really a problem though.
  //
  if ((idMandatory == true) && (idP == NULL))
  {
    orionldError(OrionldBadRequestData, "Invalid exclusive registration", "information item without specifying entity id", 400);
    return false;
  }

  return true;
}
