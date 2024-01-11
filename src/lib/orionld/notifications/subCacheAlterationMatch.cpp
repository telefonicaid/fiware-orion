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
#include "logMsg/traceLevels.h"                                // LmtWatchedAttributes

#include "common/sem.h"                                        // cacheSemTake, cacheSemGive
#include "cache/subCache.h"                                    // CachedSubscription, subCacheMatch, tenantMatch

#include "orionld/types/QNode.h"                               // QNode, qNodeType
#include "orionld/types/OrionldAlteration.h"                   // OrionldAlteration, OrionldAlterationMatch, orionldAlterationType
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/dotForEq.h"                           // dotForEq
#include "orionld/common/pathComponentsSplit.h"                // pathComponentsSplit
#include "orionld/common/eqForDot.h"                           // eqForDot
#include "orionld/common/dateTime.h"                           // dateTimeFromString
#include "orionld/q/qBuild.h"                                  // qBuild
#include "orionld/q/qPresent.h"                                // qPresent
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
      if (eiP->entityId == "")  // No entity ID
        return true;

      if (strcmp(eiP->entityId.c_str(), entityId) == 0)
        return true;
    }
  }

  LM_T(LmtSubCacheMatch, ("Sub '%s': no match due to Entity ID", subP->subscriptionId));
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
    EntityInfo* eiP   = subP->entityIdInfos[ix];
    const char* eType = eiP->entityType.c_str();

    if (strcmp(entityType, eType) == 0)
      return true;

    if ((eType[0] == '*') && (eType[1] == 0))
      return true;
  }

  LM_T(LmtSubCacheMatch, ("Sub '%s': no match due to Entity Type", subP->subscriptionId));
  return false;
}



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
static bool matchLookup(OrionldAlterationMatch* matchP, OrionldAlterationMatch* itemP)
{
#if 0
  // <DEBUG>
  LM_T(LmtSubCacheMatch, ("Match List:"));
  for (OrionldAlterationMatch* mP = matchP; mP != NULL; mP = mP->next)
  {
    if (matchP->altAttrP)
      LM_T(LmtSubCacheMatch, ("o %p: %s %s", mP, mP->subP->subscriptionId, mP->altAttrP->alterationType));
    else
      LM_T(LmtSubCacheMatch, ("o %p: %s (no attr)", mP, mP->subP->subscriptionId));
  }
  LM_T(LmtSubCacheMatch, ("Compare with:"));
  if (itemP->altAttrP)
    LM_T(LmtSubCacheMatch, ("o %p: %s %s", itemP, itemP->subP->subscriptionId, itemP->altAttrP->alterationType));
  else
    LM_T(LmtSubCacheMatch, ("o %p: %s (no attr)", itemP, itemP->subP->subscriptionId));
  // </DEBUG>
#endif

  // matchP is really the match-list. itemP is the one we're looking for
  while (matchP != NULL)
  {
    if (itemP->subP == matchP->subP)  // Same subscription - might be a match
    {
      if ((matchP->altAttrP == NULL) && (itemP->altAttrP == NULL))
      {
        //
        // If the altered entity is the same, this is a duplicate.
        // If the altered entity is different, then itemP's entity needs to be added to the datas array of matchP ...
        //
        if (strcmp(itemP->altP->entityId, matchP->altP->entityId) != 0)
          LM_W(("Different entity (%s vs %s) - need to add it to the notification for sub %s", itemP->altP->entityId, matchP->altP->entityId, matchP->subP->subscriptionId));
        // return true;
      }
      else if ((matchP->altAttrP != NULL) && (itemP->altAttrP != NULL))
      {
        OrionldAlterationType inListAlterationType = matchP->altAttrP->alterationType;
        OrionldAlterationType candidate            = itemP->altAttrP->alterationType;

        if (inListAlterationType == candidate)
        {
          return true;
        }
      }
    }

    matchP = matchP->next;
  }

  return false;
}



// -----------------------------------------------------------------------------
//
// matchListInsert -
//
// NOTE:  This function puts matches in reverse order.
//        However, that is fixed later - the list is reversed back.
//
static OrionldAlterationMatch* matchListInsert(OrionldAlterationMatch* matchList, OrionldAlterationMatch* itemP)
{
  OrionldAlterationMatch* matchP = matchList;
  OrionldAlterationMatch* prev   = NULL;

  // Find the same subscription, to have all alteration-matches ordered
  while (matchP != NULL)
  {
    if (matchP->subP == itemP->subP)
    {
      //
      // insert it in the list, right BEFORE matchP (matchP == first occurrence of the subscription)
      //
      if (prev == NULL)
      {
        itemP->next = matchList;
        matchList   = itemP;
      }
      else
      {
        itemP->next = prev->next;  // prev->next === matchP
        prev->next = itemP;
      }

      return matchList;
    }

    prev   = matchP;
    matchP = matchP->next;
  }


  // First alteration-match of a subscription - prepending it to the matchList
  itemP->next = matchList;
  return itemP;  // As new matchList
}



// -----------------------------------------------------------------------------
//
// matchToMatchList -
//
static OrionldAlterationMatch* matchToMatchList
(
  OrionldAlterationMatch*      matchList,
  CachedSubscription*          subP,
  OrionldAlteration*           altP,
  OrionldAttributeAlteration*  aaP,
  int*                         matchesP
)
{
  OrionldAlterationMatch* amP = (OrionldAlterationMatch*) kaAlloc(&orionldState.kalloc, sizeof(OrionldAlterationMatch));
  amP->altP     = altP;
  amP->altAttrP = aaP;
  amP->subP     = subP;

  if (matchList == NULL)
  {
    matchList  = amP;
    amP->next  = NULL;
    *matchesP += 1;
  }
  else
  {
    // Already there? - look up the existing subs in matchList to make sure we don't get any duplicates
    if (matchLookup(matchList, amP) == false)
    {
      matchList  = matchListInsert(matchList, amP);
      *matchesP += 1;
    }
  }

  return matchList;
}



// -----------------------------------------------------------------------------
//
// falseUpdate -
//
bool falseUpdate(KjNode* attrP, KjNode* dbAttrsP)
{
  char eqAttrName[512];

  strncpy(eqAttrName, attrP->name, sizeof(eqAttrName) - 1);
  dotForEq(eqAttrName);

  KjNode* dbAttrP = kjLookup(dbAttrsP, eqAttrName);

  if (dbAttrP == NULL)  // New attribute - didn't exist
  {
    // LM_T(LmtSubCacheMatch, ("FU: NO  - NOT a False Update as '%s' did not exist before", attrP->name));
    return false;
  }

  KjNode* dbAttrValueP = kjLookup(dbAttrP, "value");

  if (dbAttrValueP == NULL)  // DB ERROR but ... never mind (will never happen)
  {
    // LM_T(LmtSubCacheMatch, ("FU: NO  - NOT a False Update as '%s' presents a DB error - no value field in the DB", attrP->name));
    return false;
  }

  KjNode* attrValueP = kjLookup(attrP, "value");

  if (attrValueP == NULL)  // No change in attribute value - false update
  {
    // LM_T(LmtSubCacheMatch, ("FU: YES - False Update as '%s' has no 'value' field in the normalized input", attrP->name));
    return true;
  }

  if (dbAttrValueP->type != attrValueP->type)  // Change in JSON type - real update
  {
    // LM_T(LmtSubCacheMatch, ("FU: NO  - NOT a False Update as '%s' the type of the attribute value is altered", attrP->name));
    return false;
  }

  if ((attrValueP->type == KjInt)     && (attrValueP->value.i != dbAttrValueP->value.i))    return false;
  if ((attrValueP->type == KjFloat)   && (attrValueP->value.f != dbAttrValueP->value.f))    return false;
  if ((attrValueP->type == KjBoolean) && (attrValueP->value.b != dbAttrValueP->value.b))    return false;

  // FIXME: Object + Array
  // LM_T(LmtSubCacheMatch, ("FU: PERHAPS - as Object + Array modification checks are still to be implemented (for '%s')", attrP->name));
  // LM_T(LmtSubCacheMatch, ("FU: YES - False Update as no value change was detected for '%s'", attrP->name));

  return true;
}



// -----------------------------------------------------------------------------
//
// attributeMatch -
//
static OrionldAlterationMatch* attributeMatch(OrionldAlterationMatch* matchList, CachedSubscription* subP, OrionldAlteration* altP, int* matchesP)
{
  int matches = 0;

  //
  // FIXME:
  //   No update should ever have ZERO alteredAttributes - I implemented that for convenience, but, can't stay.
  //   For now, this code inside "if (altP->alteredAttributes == 0)" stays but is all a bit "chapuza".
  //
  //   MIGHT BE I let "Creation" have zero alteredAttributes, as all attributes are created, none are removed nor updated ...
  //
  if (altP->alteredAttributes == 0)  // E.g. complete replace of an entity - treating it as EntityModified (for now)
  {
    KjNode* dbAttrsP = NULL;

    if ((altP->dbEntityP != NULL) && (noNotifyFalseUpdate == true))
      dbAttrsP = kjLookup(altP->dbEntityP, "attrs");

    //
    // watchedAttributes
    //
    int  watchAttrs = subP->notifyConditionV.size();
    bool match      = (watchAttrs == 0);  // If no watchedAttributes, then it's a match

    if ((watchAttrs > 0) && (altP->inEntityP != NULL))
    {
      for (KjNode* attrP = altP->inEntityP->value.firstChildP; attrP != NULL; attrP = attrP->next)
      {
        if (strcmp(attrP->name, "id")   == 0) continue;
        if (strcmp(attrP->name, "type") == 0) continue;

        for (int ix = 0; ix < watchAttrs; ix++)
        {
          LM_T(LmtWatchedAttributes, ("Comparing modified '%s' with watched '%s'", attrP->name, subP->notifyConditionV[ix].c_str()));
          if (strcmp(attrP->name, subP->notifyConditionV[ix].c_str()) == 0)
          {
            if ((dbAttrsP == NULL) || (noNotifyFalseUpdate == false) || (falseUpdate(attrP, dbAttrsP) == false))
            {
              match = true;
              break;
            }
          }
        }

        if (match == true)
          break;
      }
    }

    //
    // If no watchedAttributes - make sure not all attributes were unchanged (if noNotifyFalseUpdate in ON)
    // Only interesting of match == true
    // And of course, if the entity already existed (dbAttrsP != NULL)
    //
    if ((match == true) && (watchAttrs == 0) && (dbAttrsP != NULL) && (noNotifyFalseUpdate == true) && (altP->inEntityP != NULL))
    {
      int changed = 0;

      for (KjNode* attrP = altP->inEntityP->value.firstChildP; attrP != NULL; attrP = attrP->next)
      {
        if (strcmp(attrP->name, "id")   == 0) continue;
        if (strcmp(attrP->name, "type") == 0) continue;

        if (falseUpdate(attrP, dbAttrsP) == false)
          ++changed;
      }

      if (changed == 0)
        match = false;
    }

    // FIXME: Would need also to check those attributes that were deleted (either directly or via a REPLACE)

    // Is the Alteration type ON for this subscription?
    if (match == true)
    {
      if (subP->triggers[EntityModified] == true)
        matchList = matchToMatchList(matchList, subP, altP, NULL, &matches);
    }
    else
      LM_T(LmtSubCacheMatch, ("Sub '%s' - no match due to Watched Attributes", subP->subscriptionId));
  }

  for (int aaIx = 0; aaIx < altP->alteredAttributes; aaIx++)
  {
    OrionldAttributeAlteration*  aaP        = &altP->alteredAttributeV[aaIx];
    int                          watchAttrs = subP->notifyConditionV.size();
    int                          nIx        = 0;

    while (nIx < watchAttrs)
    {
      if (strcmp(aaP->attrName, subP->notifyConditionV[nIx].c_str()) == 0)
        break;
      ++nIx;
    }

    if ((watchAttrs > 0) && (nIx == watchAttrs))  // No match found
    {
      LM_T(LmtSubCacheMatch, ("Sub '%s' - no match due to watchedAttributes", subP->subscriptionId));
      continue;
    }

    // Is the Alteration type ON for this subscription?
    if (subP->triggers[aaP->alterationType] == false)
    {
      LM_T(LmtSubCacheMatch, ("Sub '%s' - no match due to Trigger '%s'", subP->subscriptionId, orionldAlterationType(aaP->alterationType)));
      continue;
    }

    matchList = matchToMatchList(matchList, subP, altP, aaP, &matches);
  }

  if (matches == 0)
    LM_T(LmtSubCacheMatch, ("Sub '%s' - no match due to Watched Attribute List (or Trigger!)", subP->subscriptionId));
  else
    LM_T(LmtSubCacheMatch, ("Subscription '%s' is a MATCH", subP->subscriptionId));
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
  int components = dotCount(path) + 1;
  if (components > 20)
    LM_X(1, ("The current implementation of Orion-LD can only handle 20 levels of tree navigation"));

  char* compV[20];

  // pathComponentsSplit destroys the path, I need to work on a copy
  char* pathCopy = kaStrdup(&orionldState.kalloc, path);
  components = pathComponentsSplit(pathCopy, compV);

  //
  // - the first component is always the longName of the ATTRIBUTE
  // - the second is either "value", "object", "languageMap", or the longName of the SUB-ATTRIBUTE
  //
  // 'attrs' and 'md' don't exist in an API entity and must be nulled out here.
  //

  //
  // Is it a timestamp?   (if so, an ISO8601 string must be turned into a float/integer to be compared
  //
  char* lastComponent = compV[components - 1];
  if ((strcmp(lastComponent, "observedAt") == 0) || (strcmp(lastComponent, "modifiedAt") == 0) || (strcmp(lastComponent, "createdAt") == 0))
    *isTimestampP = true;
  else
    *isTimestampP = false;

  compV[components] = NULL;

  KjNode* result = kjNavigate(treeP, compV);
  if (result != NULL)
    return result;

  //
  // Nothing found
  //
  //   What if it's due to '.' vs '=' ...
  //   Yes, I know, this is messy, some order is needed
  //
  // FIXME: Fix this!
  //
  eqForDot(compV[0]);  // As it IS an Attribute
  eqForDot(compV[1]);  // As it MIGHT be a Sub-Attribute (and if not, it has no '=')

  result = kjNavigate(treeP, compV);
  if (result != NULL)
    return result;

  //
  // Could be a Relationship ...
  // Perhaps I should "bake in" the value|object|languageMap inside kjNavigate ...
  //
  if ((components == 2) && (strcmp(compV[1], "value") == 0))
  {
    compV[1] = (char*) "object";
    result = kjNavigate(treeP, compV);
    if (result != NULL)
      return result;

    // FIXME: Here I put the '=' back ... Even messier now :(
    dotForEq(compV[0]);
    result = kjNavigate(treeP, compV);
    if (result != NULL)
      return result;
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// qRangeCompare -
//
static bool qRangeCompare(KjNode* lhsNode, QNode* rhs, bool isTimestamp)
{
  QNode* low  = rhs->value.children;

  if (low == NULL)
    return false;

  QNode* high = low->next;

  if (high == NULL)
    return false;

  if (isTimestamp)
  {
    char errorString[256];

    double lhsTimestamp  = dateTimeFromString(lhsNode->value.s, errorString, sizeof(errorString));
    double lowTimestamp  = (low->type == QNodeFloatValue)? low->value.f :  dateTimeFromString(low->value.s, errorString, sizeof(errorString));
    double highTimestamp = dateTimeFromString(high->value.s, errorString, sizeof(errorString));

    if ((lhsTimestamp < 0) || (lowTimestamp < 0) || (highTimestamp < 0))
      LM_RE(false, ("Invalid ISO8601 timestamp: %s", errorString));

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
// intComparison
//
bool intComparison(long long lhs, QNode* rhs)
{
  if      (rhs->type == QNodeIntegerValue)  return lhs == rhs->value.i;
  else if (rhs->type == QNodeFloatValue)    return ((double) lhs) == rhs->value.f;

  return false;
}



// -----------------------------------------------------------------------------
//
// floatComparison
//
bool floatComparison(KjNode* lhsP, QNode* rhs, bool isTimestamp)
{
  if (isTimestamp == false)
  {
    double lhs = lhsP->value.f;

    if      (rhs->type == QNodeFloatValue)    return lhs == rhs->value.f;  // precision for float comparison??
    else if (rhs->type == QNodeIntegerValue)  return lhs == (double) rhs->value.i;
    else                                      return false;
  }

  if (rhs->type == QNodeStringValue)
  {
    char   errorString[256];
    double rhsTimestamp = dateTimeFromString(rhs->value.s, errorString, sizeof(errorString));

    if (lhsP->value.f == rhsTimestamp)
      return true;
  }

  return false;
}


// -----------------------------------------------------------------------------
//
// boolComparison
//
bool boolComparison(bool lhs, QNode* rhs)
{
  if ((lhs == true) && (rhs->type == QNodeTrueValue))
    return true;

  if ((lhs == false) && (rhs->type == QNodeFalseValue))
    return true;

  return false;
}



// -----------------------------------------------------------------------------
//
// stringComparison
//
bool stringComparison(KjNode* lhsP, QNode* rhs, bool isTimestamp)
{
  if (rhs->type != QNodeStringValue)
    return false;

  if (isTimestamp)  // Then LHS is a Float?
  {
    char   errorString[256];
    double rhsTimestamp = dateTimeFromString(rhs->value.s, errorString, sizeof(errorString));

    return (lhsP->value.f == rhsTimestamp);
  }

  if (rhs->type != QNodeStringValue)
    return false;

  return (strcmp(lhsP->value.s, rhs->value.s) == 0);
}



// -----------------------------------------------------------------------------
//
// qCommaListCompare -
//
bool qCommaListCompare(KjNode* lhsNode, QNode* rhs, bool isTimestamp)
{
  if (isTimestamp == true)  // The first in the list id a FLOAT, the rest need to be converted to float
  {
    double timestamp;
    char   errorString[256];

    if (lhsNode->type == KjFloat)
      timestamp = lhsNode->value.f;
    else if (lhsNode->type == KjString)
      timestamp = dateTimeFromString(lhsNode->value.s, errorString, sizeof(errorString));
    else
      return false;

    QNode* child1 = rhs->value.children;

    if (child1->type != QNodeFloatValue)
      LM_E(("CLIST: Internal Error (LHS is a timestamp but the first in the RHS list is not a FLOAT ..."));
    else if (child1->value.f == timestamp)
      return true;

    // Compare the rest of the children in the comma list (converting them to FLOAT)
    for (QNode* rhP = child1->next; rhP != NULL; rhP = rhP->next)
    {
      if (rhP->type == QNodeFloatValue)
      {
        if (rhP->value.f == timestamp)
          return true;
      }
      else if (rhP->type == QNodeStringValue)
      {
        char   errorString[256];
        double rhsTimestamp = dateTimeFromString(rhP->value.s, errorString, sizeof(errorString));

        if (rhsTimestamp == timestamp)
          return true;
      }
    }

    return false;
  }

  for (QNode* rhP = rhs->value.children; rhP != NULL; rhP = rhP->next)
  {
    switch (lhsNode->type)
    {
    case KjInt:     if (intComparison(lhsNode->value.i,    rhP)              == true) return true; break;
    case KjFloat:   if (floatComparison(lhsNode,           rhP, isTimestamp) == true) return true; break;  // isTimestamp - then LHS is turned into a Float ... Right?
    case KjString:  if (stringComparison(lhsNode,          rhP, isTimestamp) == true) return true; break;  //               Can't be both - need to test empirically
    case KjBoolean: if (boolComparison(lhsNode->value.b,   rhP)              == true) return true; break;
    default:
      break;
    }
  }

  return false;
}



// -----------------------------------------------------------------------------
//
// qEqCompare -
//
bool qEqCompare(KjNode* lhsNode, QNode* rhs, bool isTimestamp)
{
  //
  // Might be a timestamp ... (observedAt, modifiedAt, or createdAt)
  //
  if (isTimestamp == true)
  {
    // lhsNode must be a string, and a valid ISO8601 at that
    if (lhsNode->type != KjString)
      return false;

    char   errorString[256];
    double timestamp = dateTimeFromString(lhsNode->value.s, errorString, sizeof(errorString));

    if (rhs->type == QNodeIntegerValue)
    {
      long long ts = (long long) timestamp;
      if (rhs->value.i == ts)
        return true;

      return false;
    }
    else if (rhs->type == QNodeFloatValue)
    {
      if (rhs->value.f == timestamp)
        return true;

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

    if (rhs->type == QNodeIntegerValue)
    {
      if ((lhsNode->value.f - margin < rhs->value.i) && (lhsNode->value.f + margin > rhs->value.i))
        return true;
    }
    else if (rhs->type == QNodeFloatValue)
    {
      if ((lhsNode->value.f - margin < rhs->value.f) && (lhsNode->value.f + margin > rhs->value.f))
        return true;
    }
  }
  else if (lhsNode->type == KjString)
  {
    if (rhs->type == QNodeStringValue)
      return (strcmp(lhsNode->value.s, rhs->value.s) == 0);
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
bool qGtCompare(KjNode* lhsNode, QNode* rhs, bool isTimestamp)
{
  //
  // Might be a timestamp ... (observedAt, modifiedAt, or createdAt)
  //
  if (isTimestamp == true)
  {
    // lhsNode must be a string, and a valid ISO8601 at that
    if (lhsNode->type != KjString)
      return false;

    char   errorString[256];
    double timestamp = dateTimeFromString(lhsNode->value.s, errorString, sizeof(errorString));

    if (rhs->type == QNodeIntegerValue)
    {
      long long ts = (long long) timestamp;
      if (rhs->value.i < ts)
        return true;

      return false;
    }
    else if (rhs->type == QNodeFloatValue)
    {
      if (rhs->value.f < timestamp)
        return true;

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
      return (strcmp(lhsNode->value.s, rhs->value.s) > 0);  // "> 0" means first arg > second arg to strcmp
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
bool qLtCompare(KjNode* lhsNode, QNode* rhs, bool isTimestamp)
{
  //
  // Might be a timestamp ... (observedAt, modifiedAt, or createdAt)
  //
  if (isTimestamp == true)
  {
    // lhsNode must be a string, and a valid ISO8601 at that
    if (lhsNode->type != KjString)
      return false;

    char   errorString[256];
    double timestamp = dateTimeFromString(lhsNode->value.s, errorString, sizeof(errorString));

    if (rhs->type == QNodeIntegerValue)
    {
      long long ts = (long long) timestamp;
      if (rhs->value.i > ts)
        return true;

      return false;
    }
    else if (rhs->type == QNodeFloatValue)
    {
      if (rhs->value.f > timestamp)
        return true;

      return false;
    }
  }

  bool r = false;
  if (lhsNode->type == KjInt)
  {
    if      (rhs->type == QNodeIntegerValue) r = (lhsNode->value.i < rhs->value.i);
    else if (rhs->type == QNodeFloatValue)   r = (lhsNode->value.i < rhs->value.f);
  }
  else if (lhsNode->type == KjFloat)
  {
    if      (rhs->type == QNodeIntegerValue) r = (lhsNode->value.f < rhs->value.i);
    else if (rhs->type == QNodeFloatValue)   r = (lhsNode->value.f < rhs->value.f);
  }
  else if (lhsNode->type == KjString)
  {
    if (rhs->type == QNodeStringValue)
      r = (strcmp(lhsNode->value.s, rhs->value.s) < 0);  // "< 0" means first arg < second arg to strcmp
  }
  else if (lhsNode->type == KjBoolean)  // true > false ... ?
  {
    if (rhs->type == QNodeTrueValue)       r = false;
    if (rhs->type == QNodeFalseValue)      r = (lhsNode->value.b == true);
  }

  return r;
}



// -----------------------------------------------------------------------------
//
// qMatchCompare -
//
bool qMatchCompare(KjNode* lhsNode, QNode* rhs)
{
  //
  // Create the REGEX in subCache - QNode might need a new QNodeValue
  // Here: Use regexec
  //
  LM_W(("Not Implemented"));
  return false;
}



// -----------------------------------------------------------------------------
//
// qMatch - move to orionld/q/qMatch.h/cpp
//
bool qMatch(QNode* qP, OrionldAlteration* altP)
{
  if (qP->type == QNodeOr)
  {
    // If any of the children is a match, then it's a match
    int childNo = 0;
    for (QNode* childP = qP->value.children; childP != NULL; childP = childP->next)
    {
      if (qMatch(childP, altP) == true)
        return true;
      ++childNo;
    }
  }
  else if (qP->type == QNodeAnd)
  {
    // If ALL of the children are a match, then it's a match
    for (QNode* childP = qP->value.children; childP != NULL; childP = childP->next)
    {
      if (qMatch(childP, altP) == false)
        return false;
    }

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
    bool     isTimestamp = false;
    KjNode*  lhsNode     = kjNavigate2(altP->finalApiEntityP, lhs->value.v, &isTimestamp);

    //
    // If Left-Hand-Side does not exist - MATCH for op "NotExist" and No Match for all other operations
    //
    if (lhsNode == NULL)
      return (qP->type == QNodeNotExists)? true : false;

    if      (qP->type == QNodeNotExists)  return false;
    else if (qP->type == QNodeExists)     return true;
    else if (qP->type == QNodeEQ)
    {
      if      (rhs->type == QNodeRange)   return  qRangeCompare(lhsNode, rhs, isTimestamp);
      else if (rhs->type == QNodeComma)   return  qCommaListCompare(lhsNode, rhs, isTimestamp);
      else                                return  qEqCompare(lhsNode, rhs, isTimestamp);
    }
    else if (qP->type == QNodeNE)
    {
      if      (rhs->type == QNodeRange)   return !qRangeCompare(lhsNode, rhs, isTimestamp);
      else if (rhs->type == QNodeComma)   return !qCommaListCompare(lhsNode, rhs, isTimestamp);
      else                                return !qEqCompare(lhsNode, rhs, isTimestamp);
    }
    else if (qP->type == QNodeGT)         return  qGtCompare(lhsNode, rhs, isTimestamp);
    else if (qP->type == QNodeLT)         return  qLtCompare(lhsNode, rhs, isTimestamp);
    else if (qP->type == QNodeGE)         return !qLtCompare(lhsNode, rhs, isTimestamp);
    else if (qP->type == QNodeLE)         return !qGtCompare(lhsNode, rhs, isTimestamp);
    else if (qP->type == QNodeMatch)      return qMatchCompare(lhsNode, rhs);
    else if (qP->type == QNodeNoMatch)    return !qMatchCompare(lhsNode, rhs);
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

  //
  // Loop over each alteration, and check ALL SUBSCRIPTIONS in the cache for that alteration
  // For each matching subscription, add the alterations into 'matchList'
  //
  cacheSemTake(__FUNCTION__, "Looping over sub-cache");

  for (OrionldAlteration* altP = alterationList; altP != NULL; altP = altP->next)
  {
    for (CachedSubscription* subP = subCacheHeadGet(); subP != NULL; subP = subP->next)
    {
      if ((multitenancy == true) && (tenantMatch(subP->tenant, orionldState.tenantName) == false))
      {
        LM_T(LmtSubCacheMatch, ("Sub '%s' - no match due to tenant", subP->subscriptionId));
        continue;
      }

      if (subP->isActive == false)
      {
        LM_T(LmtSubCacheMatch, ("Sub '%s' - no match due to isActive == false", subP->subscriptionId));
        continue;
      }

      if (strcmp(subP->status.c_str(), "active") != 0)
      {
        LM_T(LmtSubCacheMatch, ("Sub '%s' - no match due to status == '%s' (!= 'active')", subP->subscriptionId, subP->status.c_str()));
        continue;
      }

      if ((subP->expirationTime > 0) && (subP->expirationTime < orionldState.requestTime))
      {
        LM_T(LmtSubCacheMatch, ("Sub '%s' - no match due to expiration (now:%f, expired:%f)", subP->subscriptionId, orionldState.requestTime, subP->expirationTime));
        subP->status   = "expired";
        subP->isActive = false;
        continue;
      }

      if ((subP->throttling > 0) && ((orionldState.requestTime - subP->lastNotificationTime) < subP->throttling))
      {
        LM_T(LmtSubCacheMatch, ("Sub '%s' - no match due to throttling", subP->subscriptionId));
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
        subP->qP = qBuild(subP->qText, NULL, NULL, NULL, false, false);

      if (subP->qP != NULL)
      {
        if (qMatch(subP->qP, altP) == false)
        {
          LM_T(LmtSubCacheMatch, ("Sub '%s' - no match due to ldq == '%s'", subP->subscriptionId, subP->qText));
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
  }
  cacheSemGive(__FUNCTION__, "Looping over sub-cache");

  *matchesP = matches;

  return matchList;
}
