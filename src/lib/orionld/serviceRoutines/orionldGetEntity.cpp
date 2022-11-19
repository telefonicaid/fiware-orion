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
#include <curl/curl.h>                                           // curl

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjParse.h"                                       // kjParse
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove, kjChildAdd, kjArray, ...
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/curlToBrokerStrerror.h"                 // curlToBrokerStrerror
#include "orionld/payloadCheck/pCheckUri.h"                      // pCheckUri
#include "orionld/legacyDriver/legacyGetEntity.h"                // legacyGetEntity
#include "orionld/mongoc/mongocEntityLookup.h"                   // mongocEntityLookup
#include "orionld/dbModel/dbModelToApiEntity.h"                  // dbModelToApiEntity
#include "orionld/kjTree/kjGeojsonEntityTransform.h"             // kjGeojsonEntityTransform
#include "orionld/kjTree/kjChildCount.h"                         // kjChildCount
#include "orionld/forwarding/ForwardPending.h"                   // ForwardPending
#include "orionld/forwarding/regMatchForEntityGet.h"             // regMatchForEntityGet
#include "orionld/forwarding/forwardingListsMerge.h"             // forwardingListsMerge
#include "orionld/forwarding/forwardRequestSend.h"               // forwardRequestSend
#include "orionld/forwarding/fwdPendingLookupByCurlHandle.h"     // fwdPendingLookupByCurlHandle
#include "orionld/serviceRoutines/orionldGetEntity.h"            // Own interface



// -----------------------------------------------------------------------------
//
// timestampMerge -
//
void timestampMerge(KjNode* apiEntityP, KjNode* additionP, KjNode* currentTsP, KjNode* newTsP, const char* tsName, bool newReplaces)
{
  if (currentTsP != NULL)  // This should always be the case
  {
    bool newIsNewer = strcmp(currentTsP->value.s, newTsP->value.s) < 0;

    if (newReplaces == newIsNewer)
    {
      LM(("Changing %s", tsName));
      currentTsP->value.s = newTsP->value.s;
    }
    else
      LM(("Keeping %s", tsName));
  }
  else
  {
    LM(("Adding %s (as it wasn't present!!!  - should never happen)", tsName));
    kjChildRemove(additionP, newTsP);
    kjChildAdd(apiEntityP, newTsP);
  }
}



// -----------------------------------------------------------------------------
//
// newerAttribute -
//
KjNode* newerAttribute(KjNode* currentP, KjNode* pretenderP)
{
  KjNode* currentObservedAt   = kjLookup(currentP,   "observedAt");
  KjNode* pretenderObservedAt = kjLookup(pretenderP, "observedAt");

  LM(("current   observedAt: '%s'", (currentObservedAt   != NULL)? currentObservedAt->value.s   : "none"));
  LM(("pretender observedAt: '%s'", (pretenderObservedAt != NULL)? pretenderObservedAt->value.s : "none"));

  if ((currentObservedAt == NULL) && (pretenderObservedAt == NULL))
  {
    KjNode* currentModifiedAt   = kjLookup(currentP,   "modifiedAt");
    KjNode* pretenderModifiedAt = kjLookup(pretenderP, "modifiedAt");

    LM(("current   modifiedAt: '%s'", (currentModifiedAt   != NULL)? currentModifiedAt->value.s   : "none"));
    LM(("pretender modifiedAt: '%s'", (pretenderModifiedAt != NULL)? pretenderModifiedAt->value.s : "none"));

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
    LM(("Comparing observedAt"));
    LM(("  Current:   %s", currentObservedAt->value.s));
    LM(("  Pretender: %s", pretenderObservedAt->value.s));
    if (strcmp(currentObservedAt->value.s, pretenderObservedAt->value.s) >= 0)
    {
      LM(("Current wins"));
      return currentP;
    }
    else
    {
      LM(("Pretender wins"));
      return pretenderP;
    }
  }

  LM_W(("Not sure how we got here ... keeping the current - no replace"));
  return currentP;
}



// -----------------------------------------------------------------------------
//
// entityMerge -
//
bool entityMerge(KjNode* apiEntityP, KjNode* additionP, bool sysAttrs, bool auxiliary)
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
    LM(("Treating attribute '%s' (already present at %p)", attrP->name, currentP));

    if (currentP == NULL)
    {
      LM(("First instance of '%s' - moving it from 'additionP' to 'entity'", attrP->name));
      kjChildRemove(additionP, attrP);
      kjChildAdd(apiEntityP, attrP);
    }
    else if (auxiliary == false)  // two copies of the same attr ...  and NOT from an auxiliary registration
    {
      LM(("Consecutive instance of '%s' (replace or ignore)", attrP->name));
      if (newerAttribute(currentP, attrP) == attrP)
      {
        LM(("Replacing the attribute '%s' as the alternative instance is newer", attrP->name));
        kjChildRemove(apiEntityP, currentP);
        kjChildRemove(additionP, attrP);
        kjChildAdd(apiEntityP, attrP);
      }
      else
        LM(("Keeping the attribute '%s' as the alternative instance is older", attrP->name));
    }

    attrP = next;
  }

  return true;
}



#if 0
static void attrsDebug(StringArray* attrV, const char* info)
{
  LM(("SLIST: %s", info));
  if (attrV != NULL)
  {
    if (attrV->items == 0)
    {
      LM(("SLIST:   EMPTY attrV"));
    }
    else
    {
      for (int ix = 0; ix < attrV->items; ix++)
      {
        LM(("SLIST:   Attr %d: %s", ix, attrV->array[ix]));
      }
    }
  }
  else
    LM(("SLIST:   NULL attrV"));
}

static void attrsDebug2(ForwardPending*  fwdPendingList)
{
  ForwardPending* fwdPendingP = fwdPendingList;

  LM(("SLIST: *************** Entire ForwardPending list ***************************"));
  while (fwdPendingP != NULL)
  {
    KjNode* regIdP = kjLookup(fwdPendingP->regP->regTree, "id");

    LM(("SLIST: -------------------------------------------------------------"));
    LM(("SLIST: Registration: %s", (regIdP != NULL)? regIdP->value.s : "unknown"));
    LM(("SLIST: FwdOperation: %s", fwdOperations[fwdPendingP->operation]));
    attrsDebug(fwdPendingP->attrList, "attrList");
    LM(("SLIST: -------------------------------------------------------------"));

    fwdPendingP = fwdPendingP->next;
  }
  LM(("SLIST: ******************************************************"));
}
#endif



// -----------------------------------------------------------------------------
//
// sysAttrsRemove -
//
void sysAttrsRemove(KjNode* container)
{
  KjNode* createdAtP  = kjLookup(container, "createdAt");
  KjNode* modifiedAtP = kjLookup(container, "modifiedAt");

  if (createdAtP != NULL)
    kjChildRemove(container, createdAtP);

  if (modifiedAtP != NULL)
    kjChildRemove(container, modifiedAtP);
}



// -----------------------------------------------------------------------------
//
// langFixNormalized - from kjTreeFromContextAttribute.cpp
//
// FIXME: move to its own module
//
extern void langFixNormalized(KjNode* attrP, KjNode* typeP, KjNode* languageMapP, const char* lang);



// -----------------------------------------------------------------------------
//
// ntonSubAttribute -
//
void ntonSubAttribute(KjNode* saP, char* lang, bool sysAttrs)
{
  LM(("Treating sub-attribute '%s'", saP->name));

  if (sysAttrs == false)
    sysAttrsRemove(saP);

  KjNode* typeP = kjLookup(saP, "type");
  if ((lang != NULL) && (typeP != NULL) && (strcmp(typeP->value.s, "LanguageProperty")))
  {
    KjNode* languageMapP = kjLookup(saP, "languageMap");
    langFixNormalized(saP, typeP, languageMapP, lang);
  }

  for (KjNode* fieldP = saP->value.firstChildP; fieldP != NULL; fieldP = fieldP->next)
  {
    LM(("Treating attribute field '%s'", fieldP->name));
    if (strcmp(fieldP->name, "type")        == 0)  continue;
    if (strcmp(fieldP->name, "value")       == 0)  continue;
    if (strcmp(fieldP->name, "object")      == 0)  continue;
    if (strcmp(fieldP->name, "languageMap") == 0)  continue;
    if (strcmp(fieldP->name, "createdAt")   == 0)  continue;
    if (strcmp(fieldP->name, "modifiedAt")  == 0)  continue;
    if (strcmp(fieldP->name, "observedAt")  == 0)  continue;
    if (strcmp(fieldP->name, "unitCode")    == 0)  continue;

    ntonSubAttribute(fieldP, lang, sysAttrs);
  }
}



// -----------------------------------------------------------------------------
//
// ntonAttribute -
//
void ntonAttribute(KjNode* attrP, char* lang, bool sysAttrs)
{
  LM(("Treating attribute '%s'", attrP->name));

  // if array, we're dealing with datasetId ...  - later!

  if (sysAttrs == false)
    sysAttrsRemove(attrP);

  KjNode* typeP = kjLookup(attrP, "type");
  if ((lang != NULL) && (typeP != NULL) && (strcmp(typeP->value.s, "LanguageProperty")))
  {
    KjNode* languageMapP = kjLookup(attrP, "languageMap");
    langFixNormalized(attrP, typeP, languageMapP, lang);
  }

  for (KjNode* fieldP = attrP->value.firstChildP; fieldP != NULL; fieldP = fieldP->next)
  {
    LM(("Treating attribute field '%s'", fieldP->name));
    if (strcmp(fieldP->name, "type")        == 0)  continue;
    if (strcmp(fieldP->name, "value")       == 0)  continue;
    if (strcmp(fieldP->name, "object")      == 0)  continue;
    if (strcmp(fieldP->name, "languageMap") == 0)  continue;
    if (strcmp(fieldP->name, "createdAt")   == 0)  continue;
    if (strcmp(fieldP->name, "modifiedAt")  == 0)  continue;
    if (strcmp(fieldP->name, "observedAt")  == 0)  continue;
    if (strcmp(fieldP->name, "unitCode")    == 0)  continue;

    ntonSubAttribute(fieldP, lang, sysAttrs);
  }
}



// -----------------------------------------------------------------------------
//
// ntonEntity - transform entity from normalized to fixed normalized
//
// Removing sysAttrs, fixing lang, ...
//
void ntonEntity(KjNode* apiEntityP, char* lang, bool sysAttrs)
{
  if (sysAttrs == false)
    sysAttrsRemove(apiEntityP);

  for (KjNode* fieldP = apiEntityP->value.firstChildP; fieldP != NULL; fieldP = fieldP->next)
  {
    if (strcmp(fieldP->name, "id")          == 0)  continue;
    if (strcmp(fieldP->name, "type")        == 0)  continue;
    if (strcmp(fieldP->name, "createdAt")   == 0)  continue;
    if (strcmp(fieldP->name, "modifiedAt")  == 0)  continue;

    // It's an attribute
    ntonAttribute(fieldP, lang, sysAttrs);
  }
}



// -----------------------------------------------------------------------------
//
// ntosEntity - normalized to simplified for an entity
//
// The combination Simplified+KeyValues isn't allowed, we'd never get this far
//
void ntosEntity(KjNode* apiEntityP, char* lang)
{
  KjNode* attrP = apiEntityP->value.firstChildP;
  KjNode* next;

  while (attrP != NULL)
  {
    next = attrP->next;

    if (strcmp(attrP->name, "id") == 0)
    {
      attrP = next;
      continue;
    }
    if (strcmp(attrP->name, "type") == 0)
    {
      attrP = next;
      continue;
    }

    if (strcmp(attrP->name, "createdAt") == 0)
    {
      kjChildRemove(apiEntityP, attrP);
      attrP = next;
      continue;
    }

    if (strcmp(attrP->name, "modifiedAt") == 0)
    {
      kjChildRemove(apiEntityP, attrP);
      attrP = next;
      continue;
    }

    //
    // It's an attribute
    //
    KjNode* valueP = kjLookup(attrP, "value");

    if (valueP == NULL)
      valueP = kjLookup(attrP, "object");

    if (valueP == NULL)
      valueP = kjLookup(attrP, "languageMap");

    if (valueP != NULL)
    {
      attrP->value     = valueP->value;
      attrP->type      = valueP->type;
      attrP->lastChild = valueP->lastChild;
    }
    else
      LM_E(("No attribute value (object/languageMap) found - this should never happen!!!"));

    attrP = next;
  }
}



// -----------------------------------------------------------------------------
//
// ntocEntity -
//
void ntocEntity(KjNode* apiEntityP, char* lang, bool sysAttrs)
{
  KjNode* attrP = apiEntityP->value.firstChildP;
  KjNode* next;

  while (attrP != NULL)
  {
    next = attrP->next;

    if (strcmp(attrP->name, "id") == 0)
    {
      attrP = next;
      continue;
    }
    if (strcmp(attrP->name, "type") == 0)
    {
      attrP = next;
      continue;
    }

    if (strcmp(attrP->name, "createdAt") == 0)
    {
      if (sysAttrs == false)
        kjChildRemove(apiEntityP, attrP);
      attrP = next;
      continue;
    }

    if (strcmp(attrP->name, "modifiedAt") == 0)
    {
      if (sysAttrs == false)
        kjChildRemove(apiEntityP, attrP);
      attrP = next;
      continue;
    }

    //
    // It's a regular attribute
    //
    // 1. Remove the attribute type
    // 2. If only "value" left - simplified
    //
    KjNode* typeP       = kjLookup(attrP, "type");
    KjNode* createdAtP  = kjLookup(attrP, "createdAt");
    KjNode* modifiedAtP = kjLookup(attrP, "modifiedAt");

    if (typeP != NULL)
      kjChildRemove(attrP, typeP);

    if (sysAttrs == false)
    {
      if (createdAtP != NULL)
        kjChildRemove(attrP, createdAtP);

      if (modifiedAtP != NULL)
        kjChildRemove(attrP, modifiedAtP);
    }

    KjNode* valueP     = kjLookup(attrP, "value");
    LM(("'Ere (attr '%s' of type '%s')", attrP->name, kjValueType(attrP->type)));
    int     attrFields = kjChildCount(attrP);
    LM(("'Ere"));

    if ((valueP != NULL) && (attrFields == 1))  // Simplified
    {
      attrP->value     = valueP->value;
      attrP->type      = valueP->type;
      attrP->lastChild = valueP->lastChild;
    }
    else  // Dig into sub-attrs - might want a recursive call here
    {
    }

    attrP = next;
  }
}



// ----------------------------------------------------------------------------
//
// orionldGetEntity -
//
// URL PARAMETERS
// - attrs
// - geometryProperty
// - lang
// - options=simplified/concise  (normalized is default)
//
bool orionldGetEntity(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))                      // If Legacy header - use old implementation
    return legacyGetEntity();

  const char* entityId = orionldState.wildcard[0];
  if (pCheckUri(entityId, "Entity ID in URL PATH", true) == false)
    return false;

  bool             distributed    = (forwarding == true) || (orionldState.uriParams.local == false);
  ForwardPending*  fwdPendingList = NULL;
  int              forwards       = 0;

  if (distributed)
  {
    StringArray*  attrV   = (orionldState.uriParams.attrs != NULL)? &orionldState.in.attrList : NULL;
    const char*   geoProp = orionldState.uriParams.geometryProperty;

    char dateHeader[70];
    snprintf(dateHeader, sizeof(dateHeader), "Date: %s", orionldState.requestTimeString);

    //
    // If attrV AND geoProp are set, then geoProp needs to be merged in into attrV
    // UNLESS it's not present already.
    //
    // If attrV is not used BUT geoProp is, then attrV needs to be an array on ONE ITEM: geoProp
    //
    // This must be done as local copies, as otherwiose we fuck it up for the query lin the local database
    //
    //
    // If 'attrs' is used, any matches in Exclusive/Redirect registrations must chop off attributes from 'attrs'
    //
    // We want EVERYTHING from Auxiliary regs (and we may throw it all away in the end),
    // so we create the ForwardPending list FIRST, before any attrs are removed
    //
    ForwardPending* auxiliarList  = regMatchForEntityGet(RegModeAuxiliary, FwdRetrieveEntity, entityId, attrV, geoProp);
    ForwardPending* exclusiveList = regMatchForEntityGet(RegModeExclusive, FwdRetrieveEntity, entityId, attrV, geoProp);
    ForwardPending* redirectList  = regMatchForEntityGet(RegModeRedirect,  FwdRetrieveEntity, entityId, attrV, geoProp);
    ForwardPending* inclusiveList = regMatchForEntityGet(RegModeInclusive, FwdRetrieveEntity, entityId, attrV, geoProp);

    fwdPendingList = forwardingListsMerge(exclusiveList,  redirectList);
    fwdPendingList = forwardingListsMerge(fwdPendingList, inclusiveList);
    fwdPendingList = forwardingListsMerge(fwdPendingList, auxiliarList);

    // Send - copy from orionldPostEntities
    if (fwdPendingList != NULL)
    {
      for (ForwardPending* fwdPendingP = fwdPendingList; fwdPendingP != NULL; fwdPendingP = fwdPendingP->next)
      {
        // Send the forwarded request and await all responses
        if (fwdPendingP->regP != NULL)
        {
          if (forwardRequestSend(fwdPendingP, dateHeader) == 0)
          {
            ++forwards;
            fwdPendingP->error = false;
          }
          else
          {
            LM_W(("Forwarded request failed"));
            fwdPendingP->error = true;
          }
        }
      }

      int stillRunning = 1;
      int loops        = 0;

      while (stillRunning != 0)
      {
        CURLMcode cm = curl_multi_perform(orionldState.curlFwdMultiP, &stillRunning);
        if (cm != 0)
        {
          LM_E(("Internal Error (curl_multi_perform: error %d)", cm));
          forwards = 0;
          break;
        }

        if (stillRunning != 0)
        {
          cm = curl_multi_wait(orionldState.curlFwdMultiP, NULL, 0, 1000, NULL);
          if (cm != CURLM_OK)
          {
            LM_E(("Internal Error (curl_multi_wait: error %d", cm));
            break;
          }
        }

        if ((++loops >= 10) && ((loops % 5) == 0))
          LM_W(("curl_multi_perform doesn't seem to finish ... (%d loops)", loops));
      }
    }
  }

  KjNode* dbEntityP  = mongocEntityLookup(entityId, &orionldState.in.attrList, orionldState.uriParams.geometryProperty);
  KjNode* apiEntityP = NULL;
  if (dbEntityP == NULL)
  {
    if (distributed == false)
    {
      const char* title = (orionldState.in.attrList.items != 0)? "Combination Entity/Attributes Not Found" : "Entity Not Found";
      orionldError(OrionldResourceNotFound, title, entityId, 404);
      return false;
    }
    // If distributed, then it's perfectly OK to have nothing locally
    // BUT, distributes hasn't been implemented yet, so ...
    const char* title = (orionldState.in.attrList.items != 0)? "Combination Entity/Attributes Not Found" : "Entity Not Found";
    orionldError(OrionldResourceNotFound, title, entityId, 404);
    return false;
  }
  else
  {
    // In the distributed case, one and the same attribute may come from different providers,
    // and we'll have to pick ONE of them.
    // The algorithm to pick one is;
    // 1. Newest observedAt
    // 2. If none of the attributes has an observedAt:  newest modifiedAt
    // 3. If none of the attributes has an modifiedAt - pick the first one
    //
    // So, in order for this to work, in a distributed GET, we can't get rid of the sysAttrs in dbModelToApiEntity2.
    // We also cannot get rid of the sub-attrs (observedAt may be needed) - that would be if orionldState.out.format == Simplified
    // We might need those two for the attribute-instance-decision
    //
    // For the very same reason, all forwarded GET requests must carry the URI-params "?options=sysAttrs,normalized"
    //
    if (forwards > 0)  // Need Normalized and sysattrs if the operation has parts of the entity distributed (to help pick attr instances)
      apiEntityP = dbModelToApiEntity2(dbEntityP, true, RF_NORMALIZED, orionldState.uriParams.lang, true, &orionldState.pd);
    else
      apiEntityP = dbModelToApiEntity2(dbEntityP, orionldState.uriParamOptions.sysAttrs, orionldState.out.format, orionldState.uriParams.lang, true, &orionldState.pd);
  }

  //
  // Now read responses to the forwarded requests
  //
  if (forwards > 0)
  {
    CURLMsg* msgP;
    int      msgsLeft;

    while ((msgP = curl_multi_info_read(orionldState.curlFwdMultiP, &msgsLeft)) != NULL)
    {
      if (msgP->msg != CURLMSG_DONE)
        continue;

      if (msgP->data.result == CURLE_OK)
      {
        ForwardPending* fwdPendingP = fwdPendingLookupByCurlHandle(fwdPendingList, msgP->easy_handle);

        LM(("Got a response: %s", fwdPendingP->rawResponse));
        fwdPendingP->body = kjParse(orionldState.kjsonP, fwdPendingP->rawResponse);
        if (fwdPendingP->body != NULL)
        {
          // Merge in the received body into the local (or nothing)
          if (apiEntityP == NULL)
            apiEntityP = fwdPendingP->body;
          else
            entityMerge(apiEntityP, fwdPendingP->body, orionldState.uriParamOptions.sysAttrs, fwdPendingP->regP->mode == RegModeAuxiliary);
        }
        else
          LM_E(("Internal Error (parse error for the received response of a forwarded request)"));
      }
      else
        LM_E(("CURL Error %d awaiting response to forwarded request: %s", msgP->data.result, curl_easy_strerror(msgP->data.result)));
    }

    kjTreeLog(apiEntityP, "API entity after forwarding");
  }

  if (orionldState.out.contentType == GEOJSON)
  {
    apiEntityP = kjGeojsonEntityTransform(apiEntityP, orionldState.geoPropertyNode);

    //
    // If URI params 'attrs' and 'geometryProperty' are given BUT 'geometryProperty' is not part of 'attrs', then we need to remove 'geometryProperty' from
    // the "properties" object
    //
    if ((orionldState.in.attrList.items > 0) && (orionldState.uriParams.geometryProperty != NULL))
    {
      bool geometryPropertyInAttrList = false;

      for (int ix = 0; ix < orionldState.in.attrList.items; ix++)
      {
        if (strcmp(orionldState.in.geometryPropertyExpanded, orionldState.in.attrList.array[ix]) == 0)
        {
          geometryPropertyInAttrList = true;
          break;
        }
      }

      if (geometryPropertyInAttrList == false)
      {
        KjNode* propertiesP = kjLookup(apiEntityP, "properties");

        if (propertiesP != NULL)
        {
          KjNode* geometryPropertyP = kjLookup(propertiesP, orionldState.in.geometryPropertyExpanded);

          if (geometryPropertyP != NULL)
            kjChildRemove(propertiesP, geometryPropertyP);
        }
      }
    }
  }
  else if (forwards > 0)
  {
    kjTreeLog(apiEntityP, "API Entity to transform");
    // Transform the apiEntityP according to in case orionldState.out.format, lang, and sysAttrs
    bool  sysAttrs = orionldState.uriParamOptions.sysAttrs;
    char* lang     = orionldState.uriParams.lang;

    if (orionldState.out.format == RF_KEYVALUES)
      ntosEntity(apiEntityP, lang);
    else if (orionldState.out.format == RF_CONCISE)
      ntocEntity(apiEntityP, lang, sysAttrs);
    else
      ntonEntity(apiEntityP, lang, sysAttrs);
  }

  orionldState.responseTree   = apiEntityP;
  orionldState.httpStatusCode = 200;

  return true;
}
