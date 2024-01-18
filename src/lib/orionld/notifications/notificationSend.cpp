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
#include <string.h>                                              // strlen
#include <sys/uio.h>                                             // writev, iovec
#include <sys/select.h>                                          // select
#include <curl/curl.h>                                           // curl

#include <string>                                                // std::string (all because of receiverInfo!!!)
#include <map>                                                   // std::map    (all because of receiverInfo!!!)

extern "C"
{
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kjson/kjRenderSize.h"                                  // kjFastRenderSize
#include "kjson/kjRender.h"                                      // kjFastRender
#include "kjson/kjBuilder.h"                                     // kjObject, kjArray, kjString, kjChildAdd, ...
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjClone.h"                                       // kjClone
}

#include "logMsg/logMsg.h"

#include "cache/CachedSubscription.h"                            // CachedSubscription

#include "orionld/types/OrionldAlteration.h"                     // OrionldAlterationMatch, OrionldAlteration, orionldAlterationType
#include "orionld/types/OrionLdRestService.h"                    // OrionLdRestService
#include "orionld/common/orionldState.h"                         // orionldState, coreContextUrl, userAgentHeader
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/common/uuidGenerate.h"                         // uuidGenerate
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/common/langStringExtract.h"                    // langStringExtract
#include "orionld/kjTree/kjEntityIdLookupInEntityArray.h"        // kjEntityIdLookupInEntityArray
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContextP
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/mqtt/mqttNotify.h"                             // mqttNotify
#include "orionld/notifications/httpNotify.h"                    // httpNotify
#include "orionld/notifications/httpsNotify.h"                   // httpsNotify
#include "orionld/notifications/notificationDataToGeoJson.h"     // notificationDataToGeoJson
#include "orionld/notifications/previousValueAdd.h"              // previousValueAdd
#include "orionld/notifications/notificationSend.h"              // Own interface



// -----------------------------------------------------------------------------
//
// Fixed value headers - mocve to separate file - also used in pernot/pernotSend.cpp
//
const char* contentTypeHeaderJson    = (char*) "Content-Type: application/json\r\n";
const char* contentTypeHeaderJsonLd  = (char*) "Content-Type: application/ld+json\r\n";
const char* contentTypeHeaderGeoJson = (char*) "Content-Type: application/geo+json\r\n";
const char* acceptHeader             = (char*) "Accept: application/json\r\n";

const char* normalizedHeader         = (char*) "Ngsild-Attribute-Format: Normalized\r\n";
const char* conciseHeader            = (char*) "Ngsild-Attribute-Format: Concise\r\n";
const char* simplifiedHeader         = (char*) "Ngsild-Attribute-Format: Simplified\r\n";

const char* normalizedHeaderNgsiV2   = (char*) "Ngsiv2-Attrsformat: normalized\r\n";
const char* keyValuesHeaderNgsiV2    = (char*) "Ngsiv2-Attrsformat: keyValues\r\n";

char    userAgentHeader[64];     // "User-Agent: orionld/" + ORIONLD_VERSION + \r\n" - initialized in orionldServiceInit()
size_t  userAgentHeaderLen = 0;  // Set in orionldServiceInit()



// -----------------------------------------------------------------------------
//
// static buffer for small notifications (payload body)
//
static __thread char body[4 * 1024];



// -----------------------------------------------------------------------------
//
// attributeToSimplified - move to its own module
//
// 1. Find the type
// 2. Knowing the type, find the value ("value", "object", or "languageMap")
// 3. Make the value node the RHS of the attribute
//
static void attributeToSimplified(KjNode* attrP, const char* lang)
{
  bool languangeMap = false;

  // Get the attribute type
  KjNode* attrTypeP = kjLookup(attrP, "type");
  if (attrTypeP       == NULL)      LM_RVE(("Attribute '%s' has no type", attrP->name));
  if (attrTypeP->type != KjString)  LM_RVE(("Attribute '%s' has a type that is not a JSON String", attrP->name));

  // Get the value
  char* valueFieldName = (char*) "value";
  if (strcmp(attrTypeP->value.s, "Relationship") == 0)
    valueFieldName = (char*) "object";
  else if (strcmp(attrTypeP->value.s, "LanguageProperty") == 0)
  {
    languangeMap = true;
    valueFieldName = (char*) "languageMap";
  }

  KjNode* valueP = kjLookup(attrP, valueFieldName);

  if (valueP == NULL)
    LM_RVE(("Attribute '%s' has no value '%s'", attrP->name, valueFieldName));

  if ((languangeMap == true) && (lang[0] != 0))
  {
    char*   pickedLanguage;  // Not used (Simplified), but langStringExtract needs it
    KjNode* langNodeP = langItemPick(valueP, attrP->name, lang, &pickedLanguage);

    attrP->value       = langNodeP->value;
    attrP->type        = langNodeP->type;
  }
  else
  {
    attrP->type  = valueP->type;
    attrP->value = valueP->value;
  }
}



// -----------------------------------------------------------------------------
//
// attributeToConcise - move to its own module
//
// 1. Find and remove the type
// 2. If only one item left and it's "value" - Simplified
//
static void attributeToConcise(KjNode* attrP, bool* simplifiedP, const char* lang)
{
  // Get the attribute type and remove it
  KjNode* attrTypeP = kjLookup(attrP, "type");
  if (attrTypeP == NULL)
    LM_RVE(("Attribute '%s' has no type", attrP->name));
  if (attrTypeP->type != KjString)
    LM_RVE(("Attribute '%s' has a type that is not a JSON String", attrP->name));

  kjChildRemove(attrP, attrTypeP);

  if ((lang[0] != 0) && (strcmp(attrTypeP->value.s, "LanguageProperty") == 0))
  {
    KjNode* valueP    = kjLookup(attrP, "languageMap");
    char*   pickedLanguage;  // Name of the picked language
    KjNode* langNodeP = langItemPick(valueP, attrP->name, lang, &pickedLanguage);

    valueP->value       = langNodeP->value;
    valueP->type        = langNodeP->type;
    valueP->name        = (char*) "value";

    KjNode* langP   = kjString(orionldState.kjsonP, "lang", pickedLanguage);
    kjChildAdd(attrP, langP);
    return;
  }

  if ((strcmp(attrTypeP->value.s, "Property") != 0) && (strcmp(attrTypeP->value.s, "GeoProperty") != 0))
    return;

  // If only one item left in attrP - Simplified
  if ((attrP->value.firstChildP != NULL) && (attrP->value.firstChildP->next == NULL))
  {
    attrP->type  = attrP->value.firstChildP->type;
    attrP->value = attrP->value.firstChildP->value;
    *simplifiedP = true;
  }
}



// -----------------------------------------------------------------------------
//
// attributeToNormalized -
//
static void attributeToNormalized(KjNode* attrP, const char* lang)
{
  KjNode* attrTypeP = kjLookup(attrP, "type");

  //
  // LanguageProperty and 'lang'
  //
  if (attrTypeP != NULL)
  {
    if ((lang[0] != 0) && (strcmp(attrTypeP->value.s, "LanguageProperty")  == 0))
    {
      KjNode* valueP = kjLookup(attrP, "languageMap");
      if (valueP != NULL)
      {
        char*   pickedLanguage;  // Name of the picked language
        KjNode* langNodeP = langItemPick(valueP, attrP->name, lang, &pickedLanguage);

        valueP->value       = langNodeP->value;
        valueP->type        = langNodeP->type;

        // Change type form 'LanguageProperty' to 'Property'
        attrTypeP->value.s = (char*) "Property";

        // Change "languageMap" to "value" for the value
        valueP->name = (char*) "value";

        KjNode* langP = kjString(orionldState.kjsonP, "lang", pickedLanguage);
        kjChildAdd(attrP, langP);
      }
    }
  }
}



// -----------------------------------------------------------------------------
//
// attributeFix - compaction and format (concise, simplified, normalized)
//
static void attributeFix(KjNode* attrP, CachedSubscription* subP)
{
  bool simplified = (subP->renderFormat == RF_SIMPLIFIED);
  bool concise    = (subP->renderFormat == RF_CONCISE);

  // Never mind "location", "observationSpace", and "operationSpace"
  // It is probably faster to lookup their alias (and get the same back) as it is to
  // do three string-comparisons in every loop
  //
  eqForDot(attrP->name);
  char* attrLongName = attrP->name;

  attrP->name = orionldContextItemAliasLookup(subP->contextP, attrP->name, NULL, NULL);

  //
  // ".added" and ".removed" are help arrays for Merge+Patch behaviour
  // They shouldn't be here at this point but if they are, they're to be ignored
  // So, we just remove them, for now.
  //
  // All this needs to be carefully studied and probably modified.
  //
  KjNode* addedP    = kjLookup(attrP, ".added");
  KjNode* removedP  = kjLookup(attrP, ".removed");

  if (addedP   != NULL) kjChildRemove(attrP, addedP);
  if (removedP != NULL) kjChildRemove(attrP, removedP);


  //
  // If vocab-property, its value needs to be compacted
  //
  KjNode* vocabP = kjLookup(attrP, "vocab");
  if (vocabP != NULL)
  {
    if (vocabP->type == KjString)
      vocabP->value.s = orionldContextItemAliasLookup(subP->contextP, vocabP->value.s, NULL, NULL);
    else if (vocabP->type == KjArray)
    {
      for (KjNode* wordP = vocabP->value.firstChildP; wordP != NULL; wordP = wordP->next)
      {
        if (wordP->type == KjString)
          wordP->value.s = orionldContextItemAliasLookup(subP->contextP, wordP->value.s, NULL, NULL);
      }
    }
  }

  bool asSimplified = false;
  if (attrP->type == KjObject)
  {
    if      (simplified)  attributeToSimplified(attrP, subP->lang.c_str());
    else if (concise)     attributeToConcise(attrP, &asSimplified, subP->lang.c_str());
    else                  attributeToNormalized(attrP, subP->lang.c_str());
  }

  if ((asSimplified == false) && (simplified == false))
  {
    //
    // Here we're "in subAttributeFix"
    //

    // Add the "previousValue", unless RF_SIMPLIFIED
    if ((subP->renderFormat != RF_SIMPLIFIED) && (subP->showChanges == true))
      previousValueAdd(attrP, attrLongName);

    LM_T(LmtPatchEntity, ("Fixing attribute '%s' (JSON type: %s)", attrP->name, kjValueType(attrP->type)));
    for (KjNode* saP = attrP->value.firstChildP; saP != NULL; saP = saP->next)
    {
      if (strcmp(saP->name, "value")       == 0) continue;
      if (strcmp(saP->name, "object")      == 0) continue;
      if (strcmp(saP->name, "languageMap") == 0) continue;
      if (strcmp(saP->name, "vocab")       == 0) continue;
      if (strcmp(saP->name, "unitCode")    == 0) continue;

      eqForDot(saP->name);
      saP->name = orionldContextItemAliasLookup(subP->contextP, saP->name, NULL, NULL);

      if (saP->type == KjObject)
      {
        if (subP->renderFormat == RF_SIMPLIFIED)
          attributeToSimplified(saP, subP->lang.c_str());
        else if (subP->renderFormat == RF_CONCISE)
          attributeToConcise(saP, &asSimplified, subP->lang.c_str());  // asSimplified is not used down here
      }
    }
  }
}



// -----------------------------------------------------------------------------
//
// entityFix - compaction and format (concise, simplified, normalized)
//
KjNode* entityFix(KjNode* originalEntityP, CachedSubscription* subP)
{
  KjNode* entityP   = kjClone(orionldState.kjsonP, originalEntityP);

  //
  // ".added" and ".removed" are help arrays for Merge+Patch behaviour
  // They shouldn't be here at this point but if they are, they're to be ignored
  // So, we just remove them, for now.
  //
  // All this needs to be carefully studied and probably modified.
  //
  KjNode* addedP    = kjLookup(entityP, ".added");
  KjNode* removedP  = kjLookup(entityP, ".removed");

  if (addedP   != NULL) kjChildRemove(entityP, addedP);
  if (removedP != NULL) kjChildRemove(entityP, removedP);

  for (KjNode* attrP = entityP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    if (strcmp(attrP->name, "id")         == 0) continue;
    if (strcmp(attrP->name, "createdAt")  == 0) continue;
    if (strcmp(attrP->name, "modifiedAt") == 0) continue;
    if (strcmp(attrP->name, "deletedAt")  == 0) continue;
    if (strcmp(attrP->name, "observedAt") == 0) continue;

    if (strcmp(attrP->name, "type") == 0)
    {
      attrP->value.s = orionldContextItemAliasLookup(subP->contextP, attrP->value.s, NULL, NULL);
      continue;
    }

    attributeFix(attrP, subP);  // FIXME: No need to call this function for DELETE Op ...
  }

  return entityP;
}



// -----------------------------------------------------------------------------
//
// orionldEntityToNgsiV2 -
//
KjNode* orionldEntityToNgsiV2(OrionldContext* contextP, KjNode* entityP, bool keyValues, bool compact)
{
  KjNode* v2EntityP = kjClone(orionldState.kjsonP, entityP);

  // For all attributes, create a "metadata" object and move all sub-attributes inside
  // Then move back "value", "type" to the attribute
  for (KjNode* attrP = v2EntityP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    if (attrP->type != KjObject)  // attributes are objects, "id", "type", etc, are not
    {
      if (strcmp(attrP->name, "type") == 0)
      {
        if (compact == true)
        {
          eqForDot(attrP->value.s);
          attrP->value.s = orionldContextItemAliasLookup(contextP, attrP->value.s, NULL, NULL);
        }
        else
          attrP->value.s = orionldContextItemExpand(contextP, attrP->value.s, true, NULL);
      }

      continue;
    }

    eqForDot(attrP->name);
    if (compact == true)
      attrP->name = orionldContextItemAliasLookup(orionldState.contextP, attrP->name, NULL, NULL);

    // Turn object, languageMap to 'value'
    KjNode* objectP      = kjLookup(attrP, "object");
    KjNode* languageMapP = kjLookup(attrP, "languageMap");

    if (objectP      != NULL)  objectP->name      = (char*) "value";
    if (languageMapP != NULL)  languageMapP->name = (char*) "value";

    if (keyValues)
    {
      KjNode* valueP = kjLookup(attrP, "value");

      if (valueP != NULL)
      {
        attrP->type  = valueP->type;
        attrP->value = valueP->value;
      }

      continue;
    }

    // Got an attribute
    // - create a "metadata" object for the attribute
    // - move all sub-attributes inside "metadata"
    KjNode* metadataObjectP = kjObject(orionldState.kjsonP, "metadata");
    KjNode* mdP = attrP->value.firstChildP;
    KjNode* next;

    while (mdP != NULL)
    {
      if (mdP->type != KjObject)
      {
        mdP = mdP->next;
        continue;
      }
      next = mdP->next;

      // Turn object, languageMap to 'value'
      KjNode* objectP      = kjLookup(mdP, "object");
      KjNode* languageMapP = kjLookup(mdP, "languageMap");

      if (objectP      != NULL)  objectP->name      = (char*) "value";
      if (languageMapP != NULL)  languageMapP->name = (char*) "value";

      if (strcmp(mdP->name, "value") != 0)
      {
        kjChildRemove(attrP, mdP);
        kjChildAdd(metadataObjectP, mdP);
      }

      eqForDot(mdP->name);
      if (compact)
        mdP->name = orionldContextItemAliasLookup(orionldState.contextP, mdP->name, NULL, NULL);

      mdP = next;
    }

    kjChildAdd(attrP, metadataObjectP);
  }

  return v2EntityP;
}



// -----------------------------------------------------------------------------
//
// attributeFilter -
//
static KjNode* attributeFilter(KjNode* apiEntityP, OrionldAlterationMatch* mAltP)
{
  KjNode* filteredEntityP = kjObject(orionldState.kjsonP, NULL);
  KjNode* attrP           = apiEntityP->value.firstChildP;
  KjNode* next;

  while (attrP != NULL)
  {
    next = attrP->next;

    bool clone = false;
    if      (strcmp(attrP->name, "id")   == 0) clone = true;
    else if (strcmp(attrP->name, "type") == 0) clone = true;
    else
    {
      char dotName[512];
      strncpy(dotName, attrP->name, sizeof(dotName) - 1);
      eqForDot(dotName);

      for (int ix = 0; ix < (int) mAltP->subP->attributes.size(); ix++)
      {
        const char* attrName = mAltP->subP->attributes[ix].c_str();

        if (strcmp(dotName, attrName) == 0)
        {
          clone = true;
          LM_T(LmtShowChanges, ("Adding the attribute '%s' to a notification entity - add also the previousValue!", attrName));
          break;
        }
      }
    }

    if (clone)
    {
      KjNode* nodeP = kjClone(orionldState.kjsonP, attrP);
      kjChildAdd(filteredEntityP, nodeP);
    }

    attrP = next;
  }

  return filteredEntityP;
}



// -----------------------------------------------------------------------------
//
// notificationTreeForNgsiV2 -
//
static KjNode* notificationTreeForNgsiV2(OrionldAlterationMatch* matchP)
{
  CachedSubscription* subP                 = matchP->subP;
  KjNode*             notificationP        = kjObject(orionldState.kjsonP, NULL);
  KjNode*             subscriptionIdNodeP  = kjString(orionldState.kjsonP, "subscriptionId", subP->subscriptionId);
  KjNode*             dataNodeP            = kjArray(orionldState.kjsonP,  "data");
  bool                keyValues            = false;
  bool                compact              = false;

  //
  // Filter out unwanted attributes, if so requested (by the Subscription)
  //
  KjNode* apiEntityP = matchP->altP->finalApiEntityP;  // This is not correct - can be more than one entity

  if (subP->attributes.size() > 0)
    apiEntityP = attributeFilter(apiEntityP, matchP);

  if ((subP->renderFormat == RF_CROSS_APIS_SIMPLIFIED) || (subP->renderFormat == RF_CROSS_APIS_SIMPLIFIED_COMPACT))
    keyValues = true;

  if ((subP->renderFormat == RF_CROSS_APIS_NORMALIZED_COMPACT) || (subP->renderFormat == RF_CROSS_APIS_SIMPLIFIED_COMPACT))
    compact = true;

  KjNode* ngsiv2EntityP = orionldEntityToNgsiV2(subP->contextP, apiEntityP, keyValues, compact);

  kjChildAdd(dataNodeP, ngsiv2EntityP);  // Adding only the first one ...
  kjChildAdd(notificationP, dataNodeP);
  kjChildAdd(notificationP, subscriptionIdNodeP);

  return notificationP;
}



// -----------------------------------------------------------------------------
//
// notificationTree -
//
static KjNode* notificationTree(OrionldAlterationMatch* matchList)
{
  CachedSubscription* subP          = matchList->subP;
  KjNode*             notificationP = kjObject(orionldState.kjsonP, NULL);
  char                notificationId[80];

  uuidGenerate(notificationId, sizeof(notificationId), "urn:ngsi-ld:Notification:");  // notificationId could be a thread variable ...

  KjNode* idNodeP              = kjString(orionldState.kjsonP, "id", notificationId);
  KjNode* typeNodeP            = kjString(orionldState.kjsonP, "type", "Notification");
  KjNode* subscriptionIdNodeP  = kjString(orionldState.kjsonP, "subscriptionId", subP->subscriptionId);
  KjNode* notifiedAtNodeP      = kjString(orionldState.kjsonP, "notifiedAt", orionldState.requestTimeString);
  KjNode* dataNodeP            = kjArray(orionldState.kjsonP,  "data");

  kjChildAdd(notificationP, idNodeP);
  kjChildAdd(notificationP, typeNodeP);
  kjChildAdd(notificationP, subscriptionIdNodeP);
  kjChildAdd(notificationP, notifiedAtNodeP);
  kjChildAdd(notificationP, dataNodeP);

  // Reason for the notification
  if (triggerOperation == true)
  {
    char trigger[128];
    snprintf(trigger, sizeof(trigger) - 1, "%s %s", orionldState.verbString, orionldState.urlPath);
    KjNode* triggerP = kjString(orionldState.kjsonP, "trigger", trigger);
    kjChildAdd(notificationP, triggerP);
  }

  for (OrionldAlterationMatch* matchP = matchList; matchP != NULL; matchP = matchP->next)
  {
    KjNode* apiEntityP = (subP->sysAttrs == false)? matchP->altP->finalApiEntityP : matchP->altP->finalApiEntityWithSysAttrsP;

    LM_T(LmtSysAttrs, ("sysAttrs:%s, apiEntityP at %p", (subP->sysAttrs == true)? "true" : "false", apiEntityP));
    if (apiEntityP == NULL)
      apiEntityP = matchP->altP->finalApiEntityP;  // Temporary !!!

    // If the entity is already in "data", and, it's not a BATCH Operation, skip - already there
    if (orionldState.serviceP->isBatchOp == false)
    {
      KjNode* idP = kjLookup(apiEntityP, "id");
      if (idP == NULL)
        LM_X(1, ("Internal Error (notification entity without an id)"));
      if (kjEntityIdLookupInEntityArray(dataNodeP, idP->value.s) != NULL)
      {
        LM_T(LmtNotificationBody, ("Skipping entity '%s'", idP->value.s));
        continue;
      }
    }

    //
    // Filter out unwanted attributes, if so requested (by the Subscription)
    //
    if (matchP->subP->attributes.size() > 0)
      apiEntityP = attributeFilter(apiEntityP, matchP);

    apiEntityP = entityFix(apiEntityP, subP);
    kjChildAdd(dataNodeP, apiEntityP);
  }

  if (subP->httpInfo.mimeType == MT_JSONLD)  // Add @context to the entity
  {
    KjNode* contextNodeP = kjString(orionldState.kjsonP, "@context", orionldState.contextP->url);  // FIXME: use context from subscription!
    kjChildAdd(notificationP, contextNodeP);
  }

  return notificationP;
}



// -----------------------------------------------------------------------------
//
// notificationSend -
//
// writev is used for the notifications.
// The advantage with writev is that is takes as input an array of buffers, meaning there's no
// need to copy the entire payload into one single buffer:
//
// ssize_t writev(int fd, const struct iovec* iov, int iovcnt);
//
// struct iovec
// {
//   void  *iov_base;    /* Starting address */
//   size_t iov_len;     /* Number of bytes to transfer */
// };
//
// To adapt notificationSend to pernot, I need:
// - mAltP->subP->renderFormat   (easy)
// - mAltP->subP->subscriptionId (notificationTreeForNgsiV2 - easy)
// - finalApiEntityP             (notificationTreeForNgsiV2 - easy - that's the output of the query for Pernot)
// - subP->attributes            (notificationTreeForNgsiV2 - easy)
// - subP->contextP              (notificationTreeForNgsiV2 - easy)
// - finalApiEntityWithSysAttrsP (notificationTree - no probs, must add sysAttrs to the query if Pern ot sub has sysAttrs set)
// - subP->httpInfo.mimeType     (notificationTree - easy)
// - subP->ldContext             (notificationSend)
// - subP->httpInfo.notifierInfo (notificationSend)
// - mAltP->subP->rest
//
//
int notificationSend(OrionldAlterationMatch* mAltP, double timestamp, CURL** curlHandlePP)
{
  bool ngsiv2 = (mAltP->subP->renderFormat >= RF_CROSS_APIS_NORMALIZED);

  // <DEBUG>
  if (lmTraceIsSet(LmtAlt) == true)
  {
    for (OrionldAlterationMatch* mP = mAltP; mP != NULL; mP = mP->next)
    {
      LM_T(LmtAlt, ("AlterationMatch %p", mP));
      LM_T(LmtAlt, ("  Subscription     %s", mP->subP->subscriptionId));
      LM_T(LmtAlt, ("  Entity:          %s", mP->altP->entityId));
      LM_T(LmtAlt, ("  inEntityP:       %p", mP->altP->inEntityP));
      LM_T(LmtAlt, ("  finalApiEntityP: %p", mP->altP->finalApiEntityP));
      LM_T(LmtAlt, ("- - - - - -"));
    }
  }
  // </DEBUG>

  //
  // Outgoing Payload Body
  //
  KjNode* notificationP = (ngsiv2 == false)? notificationTree(mAltP) : notificationTreeForNgsiV2(mAltP);
  char*   preferHeader  = NULL;

  if ((ngsiv2 == false) && (mAltP->subP->httpInfo.mimeType == MT_GEOJSON))
  {
    char*       geometryProperty = (char*) mAltP->subP->expression.geoproperty.c_str();
    char*       attrs            = NULL;
    bool        concise          = mAltP->subP->renderFormat == RF_CONCISE;
    const char* context          = mAltP->subP->ldContext.c_str();

    if (geometryProperty[0] == 0)
      geometryProperty = (char*) "location";

    // Extract attrs from (mAltP->subP->attributes
    for (unsigned int ix = 0; ix < mAltP->subP->httpInfo.notifierInfo.size(); ix++)
    {
      KeyValue* kvP = mAltP->subP->httpInfo.notifierInfo[ix];
      if (strcmp(kvP->key, "Prefer") == 0)
        preferHeader = kvP->value;
    }

    notificationDataToGeoJson(notificationP, attrs, geometryProperty, preferHeader, concise, context);
  }

  long unsigned int  payloadBodySize  = kjFastRenderSize(notificationP);
  char*              payloadBody      = (payloadBodySize < sizeof(body))? body : kaAlloc(&orionldState.kalloc, payloadBodySize);

  kjFastRender(notificationP, payloadBody);


  //
  // Preparing the HTTP headers which will be pretty much the same for all notifications
  // What differs is Content-Length, Content-Type, and the Request header
  //

  //
  // Outgoing Header
  //
  char    requestHeader[512];
  size_t  requestHeaderLen = 0;

  if (mAltP->subP->protocol == HTTP)
  {
    // The slash before the URL (rest) is needed as it was removed in "urlParse" in orionld/common/urlParse.cpp
    if (mAltP->subP->renderFormat < RF_CROSS_APIS_NORMALIZED)
      requestHeaderLen = snprintf(requestHeader, sizeof(requestHeader), "POST /%s?subscriptionId=%s HTTP/1.1\r\n",
                                  mAltP->subP->rest,
                                  mAltP->subP->subscriptionId);
    else
      requestHeaderLen = snprintf(requestHeader, sizeof(requestHeader), "POST /%s HTTP/1.1\r\n", mAltP->subP->rest);

    LM_T(LmtNotificationSend, ("%s: URL PATH for notification == '%s'", mAltP->subP->subscriptionId, mAltP->subP->rest));
  }

  //
  // Content-Length
  //
  char              contentLenHeader[32];
  char*             lenP           = &contentLenHeader[16];
  int               sizeLeftForLen = 16;                   // 16: sizeof(contentLenHeader) - 16
  long unsigned int contentLength  = strlen(payloadBody);  // FIXME: kjFastRender should return the size

  strcpy(contentLenHeader, "Content-Length: 0");  // Can't modify inside static strings, so need a char-vec on the stack for contentLenHeader
  snprintf(lenP, sizeLeftForLen, "%d\r\n", (int) contentLength);  // Adding Content-Length inside contentLenHeader

  int headers  = 7;  // the minimum number of request headers


  //
  // Headers to be forwarded in notifications (taken from the request that provoked the notification)
  //
  if (orionldState.in.tenant        != NULL)    ++headers;
  if (orionldState.in.xAuthToken    != NULL)    ++headers;
  if (orionldState.in.authorization != NULL)    ++headers;


  //
  // Headers from Subscription::notification::endpoint::receiverInfo+headers (or custom notification in NGSIv2 ...)
  //
  headers += mAltP->subP->httpInfo.headers.size();


  // Let's limit the number of headers to 50
  if (headers > 50)
    LM_X(1, ("Too many HTTP headers (>50) for a Notification - to support that many, the broker needs a SW update and to be recompiled"));

  char          hostHeader[512];
  size_t        hostHeaderLen;

  hostHeaderLen = snprintf(hostHeader, sizeof(hostHeader), "Host: %s:%d\r\n", mAltP->subP->ip, mAltP->subP->port);

  int           ioVecLen   = headers + 3;  // Request line + X headers + empty line + payload body
  int           headerIx   = 7;
  struct iovec  ioVec[53]  = {
    { requestHeader,                 requestHeaderLen },
    { contentLenHeader,              strlen(contentLenHeader) },
    { (void*) contentTypeHeaderJson, 32 },  // Index 2
    { (void*) userAgentHeader,       userAgentHeaderLen },
    { (void*) hostHeader,            hostHeaderLen },
    { (void*) acceptHeader,          26 },
    { (void*) normalizedHeader,      37 }   // Index 6
  };

  //
  // Content-Type and Link
  //
  bool addLinkHeader = true;

  if (preferHeader != NULL)
  {
    if (strcmp(preferHeader, "body=json") == 0)
      addLinkHeader = false;
  }

  if (mAltP->subP->httpInfo.mimeType == MT_JSONLD)  // If Content-Type is application/ld+json, modify slot 2 of ioVec
  {
    ioVec[2].iov_base = (void*) contentTypeHeaderJsonLd;  // REPLACE "application/json" with "application/ld+json"
    ioVec[2].iov_len  = 35;
    addLinkHeader     = false;
  }
  else if (mAltP->subP->httpInfo.mimeType == MT_GEOJSON)
  {
    ioVec[2].iov_base = (void*) contentTypeHeaderGeoJson;  // REPLACE "application/json" with "application/geo+json"
    ioVec[2].iov_len  = 36;
  }

  if ((addLinkHeader == true) && (ngsiv2 == false))  // Add Link header - but not if NGSIv2 Cross Notification
  {
    char         linkHeader[512];
    const char*  link = (mAltP->subP->ldContext == "")? orionldCoreContextP->url : mAltP->subP->ldContext.c_str();

    snprintf(linkHeader, sizeof(linkHeader), "Link: <%s>; rel=\"http://www.w3.org/ns/json-ld#context\"; type=\"application/ld+json\"\r\n", link);

    ioVec[headerIx].iov_base = linkHeader;
    ioVec[headerIx].iov_len  = strlen(linkHeader);
    ++headerIx;
  }

  //
  // Ngsild-Attribute-Format / Ngsiv1-Attrsformat
  //
  if (mAltP->subP->renderFormat == RF_CONCISE)
  {
    ioVec[6].iov_base = (void*) conciseHeader;
    ioVec[6].iov_len  = 34;
  }
  else if (mAltP->subP->renderFormat == RF_SIMPLIFIED)
  {
    ioVec[6].iov_base = (void*) simplifiedHeader;
    ioVec[6].iov_len  = 37;
  }
  else if ((mAltP->subP->renderFormat == RF_CROSS_APIS_NORMALIZED) || (mAltP->subP->renderFormat == RF_CROSS_APIS_NORMALIZED_COMPACT))
  {
    ioVec[6].iov_base = (void*) normalizedHeaderNgsiV2;
    ioVec[6].iov_len  = 32;
  }
  else if ((mAltP->subP->renderFormat == RF_CROSS_APIS_SIMPLIFIED) || (mAltP->subP->renderFormat == RF_CROSS_APIS_SIMPLIFIED_COMPACT))
  {
    ioVec[6].iov_base = (void*) keyValuesHeaderNgsiV2;
    ioVec[6].iov_len  = 31;
  }


  //
  // Ngsild-Tenant
  //
  if ((orionldState.in.tenant != NULL) && (ngsiv2 == false))
  {
    int   len = strlen(orionldState.in.tenant) + 20;  // Ngsild-Tenant: xxx\r\n0 - '\r' seems to not count for strlen ...
    char* buf = kaAlloc(&orionldState.kalloc, len);

    ioVec[headerIx].iov_len  = snprintf(buf, len, "Ngsild-Tenant: %s\r\n", orionldState.in.tenant);
    ioVec[headerIx].iov_base = buf;

    ++headerIx;
  }


  //
  // X-Auth-Token
  //
  if (orionldState.in.xAuthToken != NULL)
  {
    int   len = strlen(orionldState.in.xAuthToken) + 20;  // X-Auth-Token: xxx\r\n0
    char* buf = kaAlloc(&orionldState.kalloc, len);

    ioVec[headerIx].iov_len  = snprintf(buf, len, "X-Auth-Token: %s\r\n", orionldState.in.xAuthToken);
    ioVec[headerIx].iov_base = buf;
    ++headerIx;
  }


  //
  // Authorization
  //
  if (orionldState.in.authorization != NULL)
  {
    int   len = strlen(orionldState.in.authorization) + 20;  // Authorization: xxx\r\n0
    char* buf = kaAlloc(&orionldState.kalloc, len);

    ioVec[headerIx].iov_len  = snprintf(buf, len, "Authorization: %s\r\n", orionldState.in.authorization);
    ioVec[headerIx].iov_base = buf;
    ++headerIx;
  }

  //
  // FIXME: Store headers in a better way - see issue #1095
  //
  for (std::map<std::string, std::string>::const_iterator it = mAltP->subP->httpInfo.headers.begin(); it != mAltP->subP->httpInfo.headers.end(); ++it)
  {
    const char* key    = it->first.c_str();
    char*       value  = (char*) it->second.c_str();

    if (strcmp(value, "urn:ngsi-ld:request") == 0)
    {
      if (orionldState.in.httpHeaders != NULL)
      {
        KjNode* kvP = kjLookup(orionldState.in.httpHeaders, key);
        if ((kvP != NULL) && (kvP->type == KjString))
          value = kvP->value.s;
        else
          continue;  // Not found in initial request - ignoring the header
      }
      else
        continue;  // No incoming headers?  Must ignore the urn:ngsi-ld:request header, no other choice
    }

    int   len = strlen(key) + strlen(value) + 10;
    char* buf = kaAlloc(&orionldState.kalloc, len);

    ioVec[headerIx].iov_len  = snprintf(buf, len, "%s: %s\r\n", key, value);
    ioVec[headerIx].iov_base = buf;
    ++headerIx;
  }

  // Empty line delimiting HTTP Headers and Payload Body
  ioVec[headerIx].iov_base = (void*) "\r\n";
  ioVec[headerIx].iov_len  = 2;
  ++headerIx;


  // Payload Body
  ioVec[headerIx].iov_base = payloadBody;
  ioVec[headerIx].iov_len  = contentLength;

  ioVecLen = headerIx + 1;

  //
  // The message is ready - just need to be sent
  //
  if (mAltP->subP->protocol == HTTP)
    return httpNotify(mAltP->subP,
                      NULL,
                      mAltP->subP->subscriptionId,
                      mAltP->subP->ip,
                      mAltP->subP->port,
                      mAltP->subP->rest,
                      ioVec,
                      ioVecLen,
                      timestamp);
  else if (mAltP->subP->protocol == HTTPS)   return httpsNotify(mAltP->subP, ioVec, ioVecLen, timestamp, curlHandlePP);
  else if (mAltP->subP->protocol == MQTT)    return mqttNotify(mAltP->subP,  ioVec, ioVecLen, timestamp);

  LM_W(("%s: Unsupported protocol for notifications: '%s'", mAltP->subP->subscriptionId, mAltP->subP->protocol));
  return -1;
}
