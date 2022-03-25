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
#include "kjson/KjNode.h"                                      // KjNode
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "cache/subCache.h"                                    // CachedSubscription, subCacheMatch, tenantMatch

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/types/OrionldAlteration.h"                   // OrionldAlteration, OrionldAlterationMatch, orionldAlterationType
#include "orionld/notifications/subCacheAlterationMatch.h"     // Own interface



// -----------------------------------------------------------------------------
//
// entityIdMatch -
//
static bool entityIdMatch(CachedSubscription* subP, const char* entityId, int eItems)
{
  for (int ix = 0; ix < eItems; ++ix)
  {
    EntityInfo* eiP = subP->entityIdInfos[ix];

    if (eiP->isPattern)
    {
      if (regexec(&eiP->entityIdPattern, entityId, 0, NULL, 0) == 0)
        return true;
    }
    else
    {
      if (strcmp(eiP->entityId.c_str(), entityId) == 0)
        return true;
    }
  }

  LM_TMP(("KZ: Sub '%s': no match due to Entity ID", subP->subscriptionId));
  return false;
}



// -----------------------------------------------------------------------------
//
// entityTypeMatch -
//
// Entity Type is mandatory in an NGSI-LD subscription. so can't be an empty string
// There is no pattern allowed for Entity Type in NGSI-LD (unlike NGSIv2)
//
static bool entityTypeMatch(CachedSubscription* subP, const char* entityType, int eItems)
{
  for (int ix = 0; ix < eItems; ++ix)
  {
    EntityInfo* eiP = subP->entityIdInfos[ix];

    if (strcmp(entityType, eiP->entityType.c_str()) == 0)
      return true;
  }

  LM_TMP(("KZ: Sub '%s': no match due to Entity Type", subP->subscriptionId));
  return false;
}



// -----------------------------------------------------------------------------
//
// attributeMatch -
//
static OrionldAlterationMatch* attributeMatch(OrionldAlterationMatch* matchList, CachedSubscription* subP, OrionldAlteration* altP, int* matchesP)
{
  int matches = 0;

  for (int aaIx = 0; aaIx < altP->alteredAttributes; aaIx++)
  {
    OrionldAttributeAlteration*  aaP        = &altP->alteredAttributeV[aaIx];
    int                          watchAttrs = subP->notifyConditionV.size();
    int                          nIx        = 0;

    while (nIx < watchAttrs)
    {
      // LM_TMP(("Comparing '%s' and '%s'", aaP->attrName, subP->notifyConditionV[nIx].c_str()));
      if (strcmp(aaP->attrName, subP->notifyConditionV[nIx].c_str()) == 0)
        break;
      ++nIx;
    }

    if ((watchAttrs > 0) && (nIx == watchAttrs))  // No match found
      continue;

    // Is the Alteration type ON for this subscription?
    if (subP->triggers[aaP->alterationType] == false)
    {
      // LM_TMP(("KZ: Sub '%s' - no match due to Trigger '%s'", subP->subscriptionId, orionldAlterationType(aaP->alterationType)));
      continue;
    }

    LM_TMP(("KZ: Alteration made it all the way:"));
    LM_TMP(("KZ:   - Alteration Type:  %s", orionldAlterationType(aaP->alterationType)));
    LM_TMP(("KZ:   - Attribute:        %s", aaP->attrName));

    // OK, let's add it to matchList!
    ++matches;

    OrionldAlterationMatch* amP = (OrionldAlterationMatch*) kaAlloc(&orionldState.kalloc, sizeof(OrionldAlterationMatch));
    amP->altP     = altP;
    amP->altAttrP = aaP;
    amP->subP     = subP;
    amP->next     = matchList;

    matchList = amP;
  }

  if (matches == 0)
    LM_TMP(("KZ: Sub '%s' - no match due to Watched Attribute List (or Trigger!)", subP->subscriptionId));

  *matchesP += matches;

  return matchList;
}



// -----------------------------------------------------------------------------
//
// subCacheAlterationMatch -
//
OrionldAlterationMatch* subCacheAlterationMatch(OrionldAlteration* alterationList, int* matchesP)
{
  OrionldAlterationMatch*  matchList = NULL;
  int                      matches   = 0;

  for (OrionldAlteration* altP = alterationList; altP != NULL; altP = altP->next)
  {
    for (CachedSubscription* subP = subCacheHeadGet(); subP != NULL; subP = subP->next)
    {
      if ((multitenancy == true) && (tenantMatch(subP->tenant, orionldState.tenantName) == false))
      {
        LM_TMP(("KZ: Sub '%s' - no match due to tenant", subP->subscriptionId));
        continue;
      }

      int eItems = subP->entityIdInfos.size();
      if (eItems > 0)
      {
        if (entityIdMatch(subP, altP->entityId, eItems) == false)
          continue;

        if (entityTypeMatch(subP, altP->entityType, eItems) == false)
          continue;
      }

      matchList = attributeMatch(matchList, subP, altP, &matches);  // Each call adds to matchList AND matches
    }
  }

  *matchesP = matches;

  return matchList;
}
