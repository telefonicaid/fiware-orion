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
#include "kbase/kMacros.h"                                     // K_FT
#include "kalloc/kaStrdup.h"                                   // kaStrdup
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "cache/subCache.h"                                    // CachedSubscription, subCacheMatch, tenantMatch
#include "common/globals.h"                                    // parse8601Time

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/q/QNode.h"                                   // QNode, qNodeType
#include "orionld/q/qBuild.h"                                  // qBuild
#include "orionld/q/qPresent.h"                                // qPresent
#include "orionld/common/pathComponentsSplit.h"                // pathComponentsSplit
#include "orionld/common/eqForDot.h"                           // eqForDot
#include "orionld/types/OrionldAlteration.h"                   // OrionldAlteration, OrionldAlterationMatch, orionldAlterationType
#include "orionld/kjTree/kjTreeLog.h"                          // kjTreeLog
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

  LM_TMP(("Sub '%s': no match due to Entity ID", subP->subscriptionId));
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

  LM_TMP(("Sub '%s': no match due to Entity Type", subP->subscriptionId));
  return false;
}


#if 0
// -----------------------------------------------------------------------------
//
// matchLookup -
//
// Look into all OrionldAttributeAlteration of all matches for the same subscription
// if the entity ID and the alterationType coincide, then it's a match
//
// PARAMETERS
//   * matchP             an item in the 'matchList' - those already programmed for notification
//   * itemP              the candidate
//
bool matchLookup(OrionldAlterationMatch* matchP, OrionldAlterationMatch* itemP)
{
  CachedSubscription* subP = matchP->subP;

  while ((matchP != NULL) && (matchP->subP == subP))  // matchP is an item in the 'matchList' - those already programmed for notification
  {
    if (strcmp(matchP->altP->entityId, itemP->altP->entityId) == 0)
    {
      // Same entity - is it the same reason too?
      OrionldAlterationType inListAlterationType = matchP->altAttrP->alterationType;
      OrionldAlterationType candidate            = itemP->altAttrP->alterationType;

      //
      // Match if
      //   - EntityDeleted            & EntityDeleted
      //   - AttributeDeleted         & AttributeDeleted
      //   - Any other AlterationType & Any other AlterationType
      //

      if      (inListAlterationType == EntityDeleted)    { if (candidate == EntityDeleted)    return true; }
      else if (inListAlterationType == AttributeDeleted) { if (candidate == AttributeDeleted) return true; }
      else
      {
        if ((candidate != EntityDeleted) && (candidate != AttributeDeleted))
          return true;
      }
    }

    matchP = matchP->next;
  }

  return false;
}
#endif


// -----------------------------------------------------------------------------
//
// matchListInsert -
//
static OrionldAlterationMatch* matchListInsert(OrionldAlterationMatch* matchList, OrionldAlterationMatch* itemP)
{
  OrionldAlterationMatch* matchP = matchList;

  // Find the same subscription, to have all alteration-matches ordered
  while (matchP != NULL)
  {
    if (matchP->subP == itemP->subP)
    {
      //
      // insert it in the list
      //
      itemP->next = matchP->next;
      matchP->next = itemP;

      return matchList;
    }

    matchP = matchP->next;
  }


  // First alteration-match of a subscription - prepending it to the matchList
  itemP->next = matchList;
  return itemP;
}



// -----------------------------------------------------------------------------
//
// matchToMatchList -
//
static OrionldAlterationMatch* matchToMatchList(OrionldAlterationMatch* matchList, CachedSubscription* subP, OrionldAlteration* altP, OrionldAttributeAlteration* aaP)
{
  OrionldAlterationMatch* amP = (OrionldAlterationMatch*) kaAlloc(&orionldState.kalloc, sizeof(OrionldAlterationMatch));
  amP->altP     = altP;
  amP->altAttrP = aaP;
  amP->subP     = subP;

  if (matchList == NULL)
  {
    matchList = amP;
    amP->next = NULL;
  }
  else
    matchList = matchListInsert(matchList, amP);  // Items in matchList are grouped by their subP

  return matchList;
}



// -----------------------------------------------------------------------------
//
// attributeMatch -
//
static OrionldAlterationMatch* attributeMatch(OrionldAlterationMatch* matchList, CachedSubscription* subP, OrionldAlteration* altP, int* matchesP)
{
  int matches = 0;

  if (altP->alteredAttributes == 0)  // E.g. complete replace of an entity - treating it as EntityModified (for now)
  {
    //
    // FIXME: what if an Entity is created with a single attribute P1, and there is a subscription with a "watchedAttributes": [ "P2" ]
    //        should a notification be sent or not?
    //        I just brouight this doubt to ETSI ISG CIM (2022-05-27)
    //
    LM_TMP(("SC: Altered Attributes == 0"));
    // Is the Alteration type ON for this subscription?
    if (subP->triggers[EntityModified] == true)
    {
      matchList = matchToMatchList(matchList, subP, altP, NULL);
      ++matches;
    }
    else
      LM_TMP(("Sub '%s' - no match due to Trigger '%s'", subP->subscriptionId, orionldAlterationType(EntityModified)));
  }

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
      LM_TMP(("Sub '%s' - no match due to Trigger '%s'", subP->subscriptionId, orionldAlterationType(aaP->alterationType)));
      continue;
    }

    matchList = matchToMatchList(matchList, subP, altP, aaP);
    ++matches;
  }

  if (matches == 0)
    LM_TMP(("Sub '%s' - no match due to Watched Attribute List (or Trigger!)", subP->subscriptionId));

  *matchesP += matches;

  return matchList;
}



// -----------------------------------------------------------------------------
//
// dotCount - count number of dots in a string (no of components in a path)
//
static int dotCount(char* s)
{
  int dots = 0;

  while (*s != 0)
  {
    if (*s == '.')
      ++dots;
    ++s;
  }

  return dots;
}



// -----------------------------------------------------------------------------
//
// kjNavigate - true Kj-Tree navigation
//
static KjNode* kjNavigate(KjNode* treeP, char** compV)
{
  KjNode* hitP = kjLookup(treeP, compV[0]);

  if (hitP == NULL)
    return NULL;

  if (compV[1] == NULL)
    return hitP;

  return kjNavigate(hitP, &compV[1]);
}



// -----------------------------------------------------------------------------
//
// kjNavigate2 - prepared for db-model, but also OK without
//
static KjNode* kjNavigate2(KjNode* treeP, char* path, bool* isTimestampP)
{
  int   components = dotCount(path) + 1;
  if (components > 20)
    LM_X(1, ("The current implementation of Orion-LD can only handle 20 levels of tree navigation"));

  char* compV[20];

  LM_TMP(("QM: path: '%s'", path));
  // pathComponentsSplit destroys the path, I need to work on a copy
  char* pathCopy = kaStrdup(&orionldState.kalloc, path);
  components = pathComponentsSplit(pathCopy, compV);
  LM_TMP(("QM: components: %d", components));

  //
  // - the first component is always the longName of the ATTRIBUTE
  // - the second is either "value", "object", "languageMap", or the longName of the SUB-ATTRIBUTE
  //
  // 'attrs' and 'md' don't exist in an API entity and must be nulled out here.
  //
  eqForDot(compV[0]);  // As it is an Attribute
  eqForDot(compV[1]);  // As it MIGHT be a Sub-Attribute (and if not, it has no '=')

  //
  // Is it a timestamp?   (if so, an ISO8601 string must be turned into a float/integer to be compared
  //
  char* lastComponent = compV[components - 1];
  if ((strcmp(lastComponent, "observedAt") == 0) || (strcmp(lastComponent, "modifiedAt") == 0) || (strcmp(lastComponent, "createdAt") == 0))
    *isTimestampP = true;
  else
    *isTimestampP = false;

  compV[components] = NULL;
  kjTreeLog(treeP, "QM: Tree to navigate");
  LM_TMP(("QM: compV[0] == '%s'", compV[0]));
  LM_TMP(("QM: compV[1] == '%s'", compV[1]));
  return kjNavigate(treeP, compV);
}



// -----------------------------------------------------------------------------
//
// qRangeCompare -
//
bool qRangeCompare(OrionldAlteration* altP, KjNode* lhsNode, QNode* rhs, bool isTimestamp)
{
  QNode* low  = rhs->value.children;

  if (low == NULL)
    return false;

  QNode* high = low->next;

  if (high == NULL)
    return false;

  LM_TMP(("QM: LHS        is of type '%s'", kjValueType(lhsNode->type)));
  LM_TMP(("QM: Low  range is of type '%s'", qNodeType(low->type)));
  LM_TMP(("QM: High range is of type '%s'", qNodeType(high->type)));
  LM_TMP(("QM: Is it a Timestamp?     %s",  K_FT(isTimestamp)));

  if (isTimestamp)
  {
    double lhsTimestamp  = parse8601Time(lhsNode->value.s);
    double lowTimestamp  = (low->type == QNodeFloatValue)? low->value.f :  parse8601Time(low->value.s);
    double highTimestamp = parse8601Time(high->value.s);

    LM_TMP(("QM: LHS  timestamp:  %f", lhsTimestamp));
    LM_TMP(("QM: Low  timestamp:  %f", lowTimestamp));
    LM_TMP(("QM: High timestamp:  %f", highTimestamp));

    if ((lhsTimestamp >= lowTimestamp) && (lhsTimestamp <= highTimestamp))
      return true;

    return false;
  }

  if (lhsNode->type == KjInt)
  {
    if (low->type == QNodeIntegerValue)
    {
      if (lhsNode->value.i < low->value.i)
        return false;
    }
    else if (low->type == QNodeFloatValue)
    {
      if ((double) lhsNode->value.i < low->value.f)
        return false;
    }
    else
      return false;  // type mismatch

    if (high->type == QNodeIntegerValue)
    {
      if (lhsNode->value.i > high->value.i)
        return false;
    }
    else if (high->type == QNodeFloatValue)
    {
      if ((double) lhsNode->value.i > high->value.f)
        return false;
    }
    else
      return false;  // type mismatch

    return true;
  }

  if (lhsNode->type == KjFloat)
  {
    if (low->type == QNodeFloatValue)
    {
      if (lhsNode->value.f < low->value.f)
        return false;
    }
    else if (low->type == QNodeIntegerValue)
    {
      if (lhsNode->value.f < (double) low->value.i)
        return false;
    }
    else
      return false;  // type mismatch

    if (high->type == QNodeFloatValue)
    {
      if (lhsNode->value.f > high->value.f)
        return false;
    }
    else if (high->type == QNodeIntegerValue)
    {
      if (lhsNode->value.f > (double) high->value.i)
        return false;
    }
    else
      return false;  // type mismatch

    return true;
  }

  if (lhsNode->type == KjString)
  {
    if (low->type == QNodeStringValue)
    {
      if (strcmp(lhsNode->value.s, low->value.s)  < 0)
        return false;
      if (strcmp(lhsNode->value.s, high->value.s) > 0)
        return false;

      return true;
    }

    return false;  // type mismatch
  }

  // Timestamp!

  return false;  // RANGES operate only on Numbers, Strings and Timestamps
}



// -----------------------------------------------------------------------------
//
// qCommaListCompare -
//
bool qCommaListCompare(OrionldAlteration* altP, KjNode* lhsNode, QNode* rhs, bool isTimestamp)
{
  return false;
}



// -----------------------------------------------------------------------------
//
// qEqCompare -
//
bool qEqCompare(OrionldAlteration* altP, KjNode* lhsNode, QNode* rhs, bool isTimestamp)
{
  if (altP->patchedEntity == NULL)
    LM_TMP(("QM: no patchTree ..."));
  else
    kjTreeLog(altP->patchedEntity, "QM: patchedEntity");

  LM_TMP(("QM: LHS is of type '%s'", kjValueType(lhsNode->type)));
  LM_TMP(("QM: RHS is of type '%s'", qNodeType(rhs->type)));

  //
  // Might be a timestamp ... (observedAt, modifiedAt, or createdAt)
  //
  if (isTimestamp == true)
  {
    LM_TMP(("QM: it's a timestamp"));

    // lhsNode must be a string, and a valid ISO8601 at that
    if (lhsNode->type != KjString)
    {
      LM_TMP(("QM: LHS '%s' is not a KjString", lhsNode->name));
      return false;
    }

    LM_TMP(("QM: parsing ISO8601 '%s'", lhsNode->value.s));
    double timestamp = parse8601Time(lhsNode->value.s);
    LM_TMP(("QM: parsed ISO8601, got %f", timestamp));

    if (rhs->type == QNodeIntegerValue)
    {
      long long ts = (long long) timestamp;
      if (rhs->value.i == ts)
      {
        LM_TMP(("QM: Matching timestamp (%lld == %lld)", rhs->value.i, ts));
        return true;
      }

      LM_TMP(("QM: Non-matching timestamp (%lld vs %lld)", rhs->value.i, ts));
      return false;
    }
    else if (rhs->type == QNodeFloatValue)
    {
      if (rhs->value.f == timestamp)
      {
        LM_TMP(("QM: Matching timestamp (%f == %f)", rhs->value.f, timestamp));
        return true;
      }

      LM_TMP(("QM: Non-matching timestamp (%f vs %f - might need some margin here ...)", rhs->value.f, timestamp));
      return false;
    }
  }

  if (lhsNode->type == KjInt)
  {
    if      (rhs->type == QNodeIntegerValue) return (lhsNode->value.i == rhs->value.i);
    else if (rhs->type == QNodeFloatValue)   return (lhsNode->value.i == rhs->value.f);
  }
  else if (lhsNode->type == KjFloat)
  {
    double margin = 0.000001;  // What margin should I use?

    LM_TMP(("QM: LHS is a FLOAT of %f", lhsNode->value.f));
    if (rhs->type == QNodeIntegerValue)
    {
      LM_TMP(("QM: RHS id an INTEGER of %d", rhs->value.i));
      if ((lhsNode->value.f - margin < rhs->value.i) && (lhsNode->value.f + margin > rhs->value.i))
        return true;
    }
    else if (rhs->type == QNodeFloatValue)
    {
      LM_TMP(("QM: RHS id a FLOAT of %d", rhs->value.f));
      if ((lhsNode->value.f - margin < rhs->value.f) && (lhsNode->value.f + margin > rhs->value.f))
        return true;
    }
  }
  else if (lhsNode->type == KjString)
  {
    if (rhs->type == QNodeStringValue)
    {
      LM_TMP(("QM: Comparing two strings: LHS: '%s' (at %p) and RHS: '%s' (at %p)", lhsNode->value.s, lhsNode->value.s, rhs->value.s, rhs->value.s));
      return (strcmp(lhsNode->value.s, rhs->value.s) == 0);
    }
  }
  else if (lhsNode->type == KjBoolean)
  {
    if (rhs->type == QNodeTrueValue)       return (lhsNode->value.b == true);
    if (rhs->type == QNodeFalseValue)      return (lhsNode->value.b == false);
  }

  return false;
}



// -----------------------------------------------------------------------------
//
// qGtCompare -
//
bool qGtCompare(OrionldAlteration* altP, KjNode* lhsNode, QNode* rhs, bool isTimestamp)
{
  if (altP->patchedEntity == NULL)
    LM_TMP(("QM: no patchTree ..."));
  else
    kjTreeLog(altP->patchedEntity, "QM: patchedEntity");

  LM_TMP(("QM: LHS is of type '%s'", kjValueType(lhsNode->type)));
  LM_TMP(("QM: RHS is of type '%s'", qNodeType(rhs->type)));

  //
  // Might be a timestamp ... (observedAt, modifiedAt, or createdAt)
  //
  if (isTimestamp == true)
  {
    LM_TMP(("QM: it's a timestamp"));

    // lhsNode must be a string, and a valid ISO8601 at that
    if (lhsNode->type != KjString)
    {
      LM_TMP(("QM: LHS '%s' is not a KjString", lhsNode->name));
      return false;
    }

    LM_TMP(("QM: parsing ISO8601 '%s'", lhsNode->value.s));
    double timestamp = parse8601Time(lhsNode->value.s);
    LM_TMP(("QM: parsed ISO8601, got %f", timestamp));

    if (rhs->type == QNodeIntegerValue)
    {
      long long ts = (long long) timestamp;
      if (rhs->value.i < ts)
      {
        LM_TMP(("QM: Matching timestamp (%lld < %lld)", rhs->value.i, ts));
        return true;
      }

      LM_TMP(("QM: Non-matching timestamp (%lld vs %lld)", rhs->value.i, ts));
      return false;
    }
    else if (rhs->type == QNodeFloatValue)
    {
      if (rhs->value.f < timestamp)
      {
        LM_TMP(("QM: Matching timestamp (%f < %f)", rhs->value.f, timestamp));
        return true;
      }

      LM_TMP(("QM: Non-matching timestamp (%f vs %f - might need some margin here ...)", rhs->value.f, timestamp));
      return false;
    }
  }

  if (lhsNode->type == KjInt)
  {
    if      (rhs->type == QNodeIntegerValue) return (lhsNode->value.i > rhs->value.i);
    else if (rhs->type == QNodeFloatValue)   return (lhsNode->value.i > rhs->value.f);
  }
  else if (lhsNode->type == KjFloat)
  {
    if      (rhs->type == QNodeIntegerValue) return (lhsNode->value.f > rhs->value.i);
    else if (rhs->type == QNodeFloatValue)   return (lhsNode->value.f > rhs->value.f);
  }
  else if (lhsNode->type == KjString)
  {
    if (rhs->type == QNodeStringValue)
    {
      LM_TMP(("QM: Comparing two strings: LHS: '%s' and RHS: '%s'", lhsNode->value.s, rhs->value.s));
      return (strcmp(lhsNode->value.s, rhs->value.s) > 0);  // "> 0" means first arg > second arg to strcmp
    }
  }
  else if (lhsNode->type == KjBoolean)  // true > false ... ?
  {
    if (rhs->type == QNodeTrueValue)       return false;
    if (rhs->type == QNodeFalseValue)      return (lhsNode->value.b == true);
  }

  return false;
}



// -----------------------------------------------------------------------------
//
// qLtCompare -
//
bool qLtCompare(OrionldAlteration* altP, KjNode* lhsNode, QNode* rhs, bool isTimestamp)
{
  if (altP->patchedEntity == NULL)
    LM_TMP(("QM: no patchTree ..."));
  else
    kjTreeLog(altP->patchedEntity, "QM: patchedEntity");

  LM_TMP(("QM: LHS is of type '%s'", kjValueType(lhsNode->type)));
  LM_TMP(("QM: RHS is of type '%s'", qNodeType(rhs->type)));

  //
  // Might be a timestamp ... (observedAt, modifiedAt, or createdAt)
  //
  if (isTimestamp == true)
  {
    LM_TMP(("QM: it's a timestamp"));

    // lhsNode must be a string, and a valid ISO8601 at that
    if (lhsNode->type != KjString)
    {
      LM_TMP(("QM: LHS '%s' is not a KjString", lhsNode->name));
      return false;
    }

    LM_TMP(("QM: parsing ISO8601 '%s'", lhsNode->value.s));
    double timestamp = parse8601Time(lhsNode->value.s);
    LM_TMP(("QM: parsed ISO8601, got %f", timestamp));

    if (rhs->type == QNodeIntegerValue)
    {
      long long ts = (long long) timestamp;
      if (rhs->value.i > ts)
      {
        LM_TMP(("QM: Matching timestamp (%lld > %lld)", rhs->value.i, ts));
        return true;
      }

      LM_TMP(("QM: Non-matching timestamp (%lld vs %lld)", rhs->value.i, ts));
      return false;
    }
    else if (rhs->type == QNodeFloatValue)
    {
      if (rhs->value.f > timestamp)
      {
        LM_TMP(("QM: Matching timestamp (%f > %f)", rhs->value.f, timestamp));
        return true;
      }

      LM_TMP(("QM: Non-matching timestamp (%f vs %f - might need some margin here ...)", rhs->value.f, timestamp));
      return false;
    }
  }

  if (lhsNode->type == KjInt)
  {
    if      (rhs->type == QNodeIntegerValue) return (lhsNode->value.i < rhs->value.i);
    else if (rhs->type == QNodeFloatValue)   return (lhsNode->value.i < rhs->value.f);
  }
  else if (lhsNode->type == KjFloat)
  {
    if      (rhs->type == QNodeIntegerValue) return (lhsNode->value.f < rhs->value.i);
    else if (rhs->type == QNodeFloatValue)   return (lhsNode->value.f < rhs->value.f);
  }
  else if (lhsNode->type == KjString)
  {
    if (rhs->type == QNodeStringValue)
    {
      LM_TMP(("QM: Comparing two strings: LHS: '%s' and RHS: '%s'", lhsNode->value.s, rhs->value.s));
      return (strcmp(lhsNode->value.s, rhs->value.s) < 0);  // "< 0" means first arg < second arg to strcmp
    }
  }
  else if (lhsNode->type == KjBoolean)  // true > false ... ?
  {
    if (rhs->type == QNodeTrueValue)       return false;
    if (rhs->type == QNodeFalseValue)      return (lhsNode->value.b == true);
  }

  return false;
}



// -----------------------------------------------------------------------------
//
// qMatchCompare -
//
bool qMatchCompare(OrionldAlteration* altP, KjNode* lhsNode, QNode* rhs)
{
  //
  // Create the REGEX in subCache - QNode might need a new QNodeValue
  // Here: Use regexec
  //
  LM_TMP(("QM: LHS is of type '%s' (must be String)", kjValueType(lhsNode->type)));
  LM_TMP(("QM: RHS is of type '%s' (must be String)", qNodeType(rhs->type)));

  LM_W(("Not Implemented"));
  return false;
}



// -----------------------------------------------------------------------------
//
// qMatch - move to orionld/q/qMatch.h/cpp
//
bool qMatch(QNode* qP, OrionldAlteration* altP)
{
  LM_TMP(("ALT: Operation: '%s'", qNodeType(qP->type)));

  if (qP->type == QNodeOr)
  {
    // <DEBUG>
    qPresent(qP, "DEBUG", "OR operation");
    int children = 0;
    LM_TMP(("Children of OR operation:"));
    for (QNode* childP = qP->value.children; childP != NULL; childP = childP->next)
    {
      LM_TMP(("o %d: %s", children, qNodeType(childP->type)));
      ++children;
    }
    // </DEBUG>

    // If any of the children is a match, then it's a match
    int childNo = 0;
    for (QNode* childP = qP->value.children; childP != NULL; childP = childP->next)
    {
      LM_TMP(("ALT: OR: checking child %d", childNo));
      if (qMatch(childP, altP) == true)
        return true;
      ++childNo;
    }
    LM_TMP(("ALT: OR: after checking %d children - no match", childNo));
  }
  else if (qP->type == QNodeAnd)
  {
    // If ALL of the children are a match, then it's a match
    for (QNode* childP = qP->value.children; childP != NULL; childP = childP->next)
    {
      LM_TMP(("ALT: Recursive call for AND item"));
      if (qMatch(childP, altP) == false)
      {
        LM_TMP(("ALT: The AND is a No-Match"));
        return false;
      }
    }

    LM_TMP(("ALT: The AND is a Match"));
    return true;
  }
  else
  {
    QNode* lhs = qP->value.children;        // variable-path
    QNode* rhs = qP->value.children->next;  // constant (MULL for QNodeExists & QNodeNotExists

    //
    // Not OR nor AND => LHS is an  attribute from the entity
    //
    // Well, or a sub-attribute, or a fragment of its value ...
    // Anyway, the attribute/sub-attribute must exist.
    // If it does not, then the result is always "false" (except for the case "q=!P1", of course :))
    //
    LM_TMP(("QM: lhs->value.v: %s", lhs->value.v));
    bool     isTimestamp = false;
    KjNode*  lhsNode     = kjNavigate2(altP->patchedEntity, lhs->value.v, &isTimestamp);
    LM_TMP(("QM: lhsNode at %p", lhsNode));

    //
    // If Left-Hand-Side does not exist - MATCH for op "NotExist" and No Match for all other operations
    //
    if (lhsNode == NULL)
      return (qP->type == QNodeNotExists)? true : false;

    if      (qP->type == QNodeNotExists)  return false;
    else if (qP->type == QNodeExists)     return true;
    else if (qP->type == QNodeEQ)
    {
      if      (rhs->type == QNodeRange)   return  qRangeCompare(altP, lhsNode, rhs, isTimestamp);
      else if (rhs->type == QNodeComma)   return  qCommaListCompare(altP, lhsNode, rhs, isTimestamp);
      else                                return  qEqCompare(altP, lhsNode, rhs, isTimestamp);
    }
    else if (qP->type == QNodeNE)
    {
      if      (rhs->type == QNodeRange)   return !qRangeCompare(altP, lhsNode, rhs, isTimestamp);
      else if (rhs->type == QNodeComma)   return !qCommaListCompare(altP, lhsNode, rhs, isTimestamp);
      else                                return !qEqCompare(altP, lhsNode, rhs, isTimestamp);
    }
    else if (qP->type == QNodeGT)         return  qGtCompare(altP, lhsNode, rhs, isTimestamp);
    else if (qP->type == QNodeLT)         return  qLtCompare(altP, lhsNode, rhs, isTimestamp);
    else if (qP->type == QNodeGE)         return !qLtCompare(altP, lhsNode, rhs, isTimestamp);
    else if (qP->type == QNodeLE)         return !qGtCompare(altP, lhsNode, rhs, isTimestamp);
    else if (qP->type == QNodeMatch)      return qMatchCompare(altP, lhsNode, rhs);
    else if (qP->type == QNodeNoMatch)    return !qMatchCompare(altP, lhsNode, rhs);
    else if (qP->type == QNodeComma)      return false;
    else if (qP->type == QNodeRange)      return false;
  }

  return false;
}



// -----------------------------------------------------------------------------
//
// subCacheAlterationMatch -
//
OrionldAlterationMatch* subCacheAlterationMatch(OrionldAlteration* alterationList, int* matchesP)
{
  OrionldAlterationMatch*  matchList = NULL;
  int                      matches   = 0;
  int                      altIx     = 0;  // DEBUG
  int                      subIx     = 0;  // DEBUG

  //
  // Loop over each alteration, and check ALL SUBSCRIPTIONS in the cache for that alteration
  // For each matching subscription, add the alterations into 'matchList'
  //
  for (OrionldAlteration* altP = alterationList; altP != NULL; altP = altP->next)
  {
    for (CachedSubscription* subP = subCacheHeadGet(); subP != NULL; subP = subP->next)
    {
      ++subIx;
      if ((multitenancy == true) && (tenantMatch(subP->tenant, orionldState.tenantName) == false))
      {
        LM_TMP(("Sub '%s' - no match due to tenant", subP->subscriptionId));
        continue;
      }

      if (subP->isActive == false)
      {
        LM_TMP(("Sub '%s' - no match due to isActive == false", subP->subscriptionId));
        continue;
      }

      if (strcmp(subP->status.c_str(), "active") != 0)
      {
        LM_TMP(("Sub '%s' - no match due to status == '%s' (!= 'active')", subP->subscriptionId, subP->status.c_str()));
        continue;
      }

      if ((subP->expirationTime > 0) && (subP->expirationTime < orionldState.requestTime))
      {
        LM_TMP(("Sub '%s' - no match due to expiration (now:%f, expired:%f)", subP->subscriptionId, orionldState.requestTime, subP->expirationTime));
        subP->status   = "expired";
        subP->isActive = false;
        continue;
      }

      if ((subP->throttling > 0) && ((orionldState.requestTime - subP->lastNotificationTime) < subP->throttling))
      {
        LM_TMP(("Sub '%s' - no match due to throttling", subP->subscriptionId));
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

      //
      // Might be we come from a sub-cache-refresh, and the subscription has a "q" but its "qP" hasn't been built
      // Only done if its an NGSI-LD operation AND if it's an NGSI-LD Subscription (ldContext has a value != "")
      //
      if ((subP->qP == NULL) && (subP->ldContext != "") && (subP->qText != NULL))
      {
        LM_TMP(("KZ: ********************** Building qP from: '%s'", subP->qText));
        subP->qP = qBuild(subP->qText, NULL, NULL, NULL, false);
      }

      if (subP->qP != NULL)
      {
        LM_TMP(("Calling qMatch for subP->qP and altP"));
        qPresent(subP->qP, "QM", "qP for qMatch");

        if (qMatch(subP->qP, altP) == false)
        {
          LM_TMP(("Sub '%s' - no match due to ldq == '%s'", subP->subscriptionId, subP->qText));
          continue;
        }
      }

      //
      // attributeMatch is too complex ...
      // I'd need simply a list of the attribute names that have been modified
      //
      // Also, I'd prefer to check for attributes before I check for 'q'
      //
      // 'geoQ' MUST come last as it requires a database query
      // (OR: somehow use GEOS library and fix it that way ...)
      //
      matchList = attributeMatch(matchList, subP, altP, &matches);  // Each call adds to matchList AND matches
    }

    ++altIx;
  }

  *matchesP = matches;

  return matchList;
}
