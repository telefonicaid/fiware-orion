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
#include <string>                                              // std::string
#include <vector>                                              // std::vector

extern "C"
{
#include "kalloc/kaAlloc.h"                                    // kaAlloc
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjChildAdd, ...
#include "kjson/kjClone.h"                                     // kjClone
#include "kjson/kjRenderSize.h"                                // kjFastRenderSize
#include "kjson/kjRender.h"                                    // kjFastRender
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "cache/subCache.h"                                    // CachedSubscription, subCacheItemLookup
#include "common/globals.h"                                    // parse8601Time

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/common/urlParse.h"                           // urlParse
#include "orionld/common/mimeTypeFromString.h"                 // mimeTypeFromString
#include "orionld/types/KeyValue.h"                            // KeyValue, keyValueLookup, keyValueAdd
#include "orionld/context/orionldAttributeExpand.h"            // orionldAttributeExpand
#include "orionld/payloadCheck/PCHECK.h"                       // PCHECK_URI
#include "orionld/payloadCheck/pcheckSubscription.h"           // pcheckSubscription
#include "orionld/db/dbConfiguration.h"                        // dbSubscriptionGet
#include "orionld/kjTree/kjTreeLog.h"                          // kjTreeLog
#include "orionld/kjTree/kjChildAddOrReplace.h"                // kjChildAddOrReplace
#include "orionld/serviceRoutines/orionldPatchSubscription.h"  // Own Interface



// ----------------------------------------------------------------------------
//
// okToRemove -
//
static bool okToRemove(const char* fieldName)
{
  if ((strcmp(fieldName, "id") == 0) || (strcmp(fieldName, "@id") == 0))
    return false;
  else if (strcmp(fieldName, "notification") == 0)
    return false;
  else if (strcmp(fieldName, "status") == 0)
    return false;

  return true;
}



// ----------------------------------------------------------------------------
//
// ngsildSubscriptionPatch -
//
// The 'q' and 'geoQ' of an NGSI-LD comes in like this:
// {
//   "q": "",
//   "geoQ": {
//     "geometry": "",
//     ""
// }
//
// In the DB, 'q' and 'geoQ' are inside "expression":
// {
//   "expression" : {
//     "q" : "https://uri=etsi=org/ngsi-ld/default-context/P2>10",
//     "mq" : "",
//     "geometry" : "circle",
//     "coords" : "1,2",
//     "georel" : "near"
//   }
// }
//
// So, if "geoQ" is present in the patch tree, then "geoQ" replaces "expression",
// by simply changing its name from "geoQ" to "expression".
// DON'T forget the "q", that is also part of "expression" but not a part of "geoQ".
// If "geoQ" replaces "expression", then we may need to maintain the "q" inside the old "expression".
// OR, if "q" is also in the patch tree, then we'll simply move it inside "expression" (former "geoQ").
//
static bool ngsildSubscriptionPatch(KjNode* dbSubscriptionP, CachedSubscription* cSubP, KjNode* patchTree, KjNode* qP, KjNode* expressionP)
{
  KjNode* fragmentP = patchTree->value.firstChildP;
  KjNode* next;

  while (fragmentP != NULL)
  {
    next = fragmentP->next;

    if (fragmentP->type == KjNull)
    {
      KjNode* toRemove = kjLookup(dbSubscriptionP, fragmentP->name);

      if (toRemove != NULL)
      {
        if (okToRemove(fragmentP->name) == false)
        {
          orionldError(OrionldBadRequestData, "Invalid Subscription Fragment - attempt to remove a mandatory field", fragmentP->name, 500);
          return false;
        }

        kjChildRemove(dbSubscriptionP, toRemove);
      }
    }
    else
    {
      if ((fragmentP != qP) && (fragmentP != expressionP))
        kjChildAddOrReplace(dbSubscriptionP, fragmentP->name, fragmentP);

      if (strcmp(fragmentP->name, "status") == 0)
      {
        if (strcmp(fragmentP->value.s, "active") == 0)
        {
          cSubP->isActive = true;
          cSubP->status   = "active";
        }
        else
        {
          cSubP->isActive = false;
          cSubP->status   = "paused";
        }
      }
    }

    fragmentP = next;
  }


  //
  // If geoqP/expressionP != NULL, then it replaces the "expression" in the DB
  // If also qP != NULL, then this qP is added to geoqP/expressionP
  // If not, we have to lookup 'q' in the old "expression" and add it to geoqP/expressionP
  //
  if (expressionP != NULL)
  {
    KjNode* dbExpressionP = kjLookup(dbSubscriptionP, "expression");

    if (dbExpressionP != NULL)
      kjChildRemove(dbSubscriptionP, dbExpressionP);
    kjChildRemove(patchTree, expressionP);
    kjChildAdd(dbSubscriptionP, expressionP);

    //
    // If 'q' is not present in the patch tree, and we have replaced the 'expression', then
    // we need to get the 'q' from the old 'expression' and add it to the new expression.
    //
    if ((qP == NULL) && (dbExpressionP != NULL))
      qP = kjLookup(dbExpressionP, "q");

    if (qP != NULL)
      kjChildAdd(expressionP, qP);
  }
  else if (qP != NULL)
  {
    KjNode* dbExpressionP = kjLookup(dbSubscriptionP, "expression");

    if (dbExpressionP != NULL)
      kjChildAddOrReplace(dbExpressionP, "q", qP);
    else
    {
      // A 'q' has been given but there is no "expression" - need to create one
      expressionP = kjObject(orionldState.kjsonP, "expression");

      kjChildAdd(expressionP, qP);
      kjChildAdd(dbSubscriptionP, expressionP);
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// ngsildKeyValuesToDatabaseModel -
//
KjNode* ngsildKeyValuesToDatabaseModel(KjNode* kvObjectArray, const char* name)
{
  KjNode* kvs = kjObject(orionldState.kjsonP, name);

  for (KjNode* kv = kvObjectArray->value.firstChildP; kv != NULL; kv = kv->next)
  {
    KjNode* key   = kjLookup(kv, "key");
    KjNode* value = kjLookup(kv, "value");

    // Steal 'value' from 'kv'
    kjChildRemove(kv, value);

    value->name = key->value.s;
    kjChildAdd(kvs, value);
  }

  return kvs;
}



// -----------------------------------------------------------------------------
//
// ngsildSubscriptionToAPIv1Datamodel -
//
// For the PATCH (step 5) the difference of the APIv1 database model and the NGSI-LD payload data for a Subscription:
//
// PAYLOAD of an NGSI-LD Subscription
// {
//   "id": "urn:ngsi-ld:subscriptions:01",         => "id" => "_id"
//   "type": "Subscription",                       => Not in DB and not needed
//   "subscriptionName": "Test subscription S01",  => NEW and added to the datamodel ("name" is accepteb as well, and used for the DB - old name of the field)
//   "description": "XXX",                         => NEW and added to the datamodel
//   "entities": [                                 => SAME, but remember, "isTypePattern" : false (default value)
//     {
//       "id": "urn:ngsi-ld:E01",
//       "type": "T1"
//     }
//   ],
//   "watchedAttributes": [ "P2" ],           => "watchedAttributes" => "conditions"
//   "q": "P2>10",                            => "q" => "expression.q"
//   "geoQ": {                                => disappears: its children go into "expression"
//     "geometry": "circle",                  => "geoQ.geometry" => "expression.geometry"
//     "coordinates": "1,2",                  => "geoQ.coordinates" => "expression.coords"
//     "georel": "near",                      => "geoQ.georel" => "expression.georel"
//     "geoproperty": "not supported"         => MUST BE ADDED: expression.geoproperty
//   },
//   "csf": "not implemented",                => NEW and added to the datamodel
//   "isActive": false,                       => "isActive" => "status" (== "inactive" or "active")
//   "notification": {                        => disappears: its children go into other fields
//     "attributes": [ "P1", "P2", "A3" ],    => "notification.attributes" => "attrs"
//     "format": "keyValues",                 => "notification.format" => "format"
//     "endpoint": {                          => disappears: its children go into other fields
//       "uri": "http://valid.url/url",       => "endpoint.uri" => "reference"
//       "accept": "application/ld+json"      => "endpoint.accept" => "mimeType"
//     }
//   },
//   "expiresAt": "2028-12-31T10:00:00",      => "expiresAt" => "expiration"
//   "throttling": 5                          => SAME
// }
//
// The subscription saved in mongo (APIv1) looks like this:
//
// {
//   "_id" : "urn:ngsi-ld:subscriptions:01",
//   "expiration" : 1861869600,
//   "reference" : "http://valid.url/url",
//   "custom" : false,
//   "mimeType" : "application/ld+json",
//   "throttling" : 5,
//   "servicePath" : "/",
//   "status" : "inactive",
//   "entities" : [
//     {
//       "id" : "urn:ngsi-ld:E01",
//       "isPattern" : "false",
//       "type" : "https://uri.etsi.org/ngsi-ld/default-context/T1",
//       "isTypePattern" : false
//     }
//   ],
//   "attrs" : [
//     "https://uri.etsi.org/ngsi-ld/default-context/P1",
//     "https://uri.etsi.org/ngsi-ld/default-context/P2",
//     "https://uri.etsi.org/ngsi-ld/default-context/A3"
//   ],
//   "metadata" : [ ],
//   "blacklist" : false,
//   "name" : "Test subscription S01",
//   "ldContext" : "https://uri.etsi.org/ngsi-ld/v1/ngsi-ld-core-context.jsonld",
//   "conditions" : [
//     "https://uri.etsi.org/ngsi-ld/default-context/P2"
//   ],
//   "expression" : {
//     "q" : "https://uri=etsi=org/ngsi-ld/default-context/P2>10",
//     "mq" : "",
//     "geometry" : "circle",
//     "coords" : "1,2",
//     "georel" : "near"
//   },
//   "format" : "keyValues"
// }
//
static bool ngsildSubscriptionToAPIv1Datamodel(KjNode* patchTree)
{
  KjNode* qP            = NULL;
  KjNode* geoqP         = NULL;
  KjNode* notificationP = NULL;

  //
  // Loop over the patch-tree and modify to make it compatible with the database model for APIv1
  //
  for (KjNode* fragmentP = patchTree->value.firstChildP; fragmentP != NULL; fragmentP = fragmentP->next)
  {
    if (strcmp(fragmentP->name, "type") == 0 || strcmp(fragmentP->name, "@type") == 0)
    {
      // Just skip it - don't want "type: Subscription" in the DB. Not needed
      continue;
    }
    else if (strcmp(fragmentP->name, "entities") == 0)
    {
      // Make sure there is an "id" and an "isPattern" in the output
      for (KjNode* entityNodeP = fragmentP->value.firstChildP; entityNodeP != NULL; entityNodeP = entityNodeP->next)
      {
        KjNode* isTypePatternP  = kjBoolean(orionldState.kjsonP, "isTypePattern", false);
        KjNode* idP             = kjLookup(entityNodeP, "id");
        KjNode* idPatternP      = kjLookup(entityNodeP, "idPattern");

        if (idP == NULL)
          idP = kjLookup(entityNodeP, "@id");

        if ((idP == NULL) && (idPatternP == NULL))
        {
          KjNode* idNodeP        = kjString(orionldState.kjsonP, "id", ".*");
          KjNode* isPatternNodeP = kjString(orionldState.kjsonP, "isPattern", "true");

          kjChildAdd(entityNodeP, idNodeP);
          kjChildAdd(entityNodeP, isPatternNodeP);
        }
        else if (idP == NULL)
        {
          KjNode* isPatternNodeP = kjString(orionldState.kjsonP, "isPattern", "true");
          kjChildAdd(entityNodeP, isPatternNodeP);
          idPatternP->name = (char*) "id";
        }
        else if (idPatternP == NULL)
        {
          KjNode* isPatternNodeP = kjString(orionldState.kjsonP, "isPattern", "false");
          kjChildAdd(entityNodeP, isPatternNodeP);
        }

        kjChildAdd(entityNodeP, isTypePatternP);
      }
    }
    else if (strcmp(fragmentP->name, "watchedAttributes") == 0)
      fragmentP->name = (char*) "conditions";
    else if (strcmp(fragmentP->name, "q") == 0)
      qP = fragmentP;
    else if (strcmp(fragmentP->name, "geoQ") == 0)
      geoqP = fragmentP;
    else if (strcmp(fragmentP->name, "isActive") == 0)
    {
      //
      // Must change name to "status" and change type from Bool to String
      //
      fragmentP->name = (char*) "status";
      fragmentP->type = KjString;

      fragmentP->value.s = (fragmentP->value.b == true)? (char*) "active" : (char*) "inactive";
    }
    else if (strcmp(fragmentP->name, "notification") == 0)
      notificationP = fragmentP;
    else if ((strcmp(fragmentP->name, "expires") == 0) || (strcmp(fragmentP->name, "expiresAt") == 0))
    {
      fragmentP->name    = (char*) "expiration";
      fragmentP->type    = KjFloat;
      fragmentP->value.f = parse8601Time(fragmentP->value.s);  // FIXME: Already done in pcheckSubscription() ...
    }
  }

  if (geoqP != NULL)
    geoqP->name = (char*) "expression";

  if (qP != NULL)
    kjChildRemove(patchTree, qP);


  //
  // The "notification" field is also treated after the loop, just to make the loop "nicer"
  // As the "notification" object must be removed from the tree and all its children moved elsewhere
  // it's much better to not do this inside the loop. Especially as the tree must be modified and a for-loop
  // would no longer be possible
  //
  if (notificationP != NULL)
  {
    KjNode* nItemP = notificationP->value.firstChildP;
    KjNode* next;

    // Loop over the "notification" fields and put them where they should be (according to the APIv1 database model)
    while (nItemP != NULL)
    {
      next = nItemP->next;

      if (strcmp(nItemP->name, "attributes") == 0)
      {
        // Change name to "attrs" and move up to toplevel
        nItemP->name = (char*) "attrs";
        kjChildAdd(patchTree, nItemP);
      }
      else if (strcmp(nItemP->name, "format") == 0)
      {
        // Keep the name, just move the node up to toplevel
        kjChildAdd(patchTree, nItemP);
      }
      else if (strcmp(nItemP->name, "endpoint") == 0)
      {
        KjNode* uriP           = kjLookup(nItemP, "uri");
        KjNode* acceptP        = kjLookup(nItemP, "accept");
        KjNode* receiverInfoP  = kjLookup(nItemP, "receiverInfo");
        KjNode* notifierInfoP  = kjLookup(nItemP, "notifierInfo");

        kjChildRemove(notificationP, nItemP);
        if (uriP != NULL)
        {
          uriP->name = (char*) "reference";
          kjChildAdd(patchTree, uriP);
        }

        if (acceptP != NULL)
        {
          acceptP->name = (char*) "mimeType";
          kjChildAdd(patchTree, acceptP);
        }

        if (receiverInfoP != NULL)
        {
          //
          // receiverInfo is an array of objects in the API:
          //   "receiverInfo": [ { "key": "k1", "value": "v1" }, { }, ... ]
          //
          // In the database, it is instead stored as an object (it's much shorter) and under a different name.
          //   "headers": { "k1": "v1", ... }
          //
          // This is amended here
          //
          KjNode* headers = ngsildKeyValuesToDatabaseModel(receiverInfoP, "headers");
          kjChildAdd(patchTree, headers);
        }

        if (notifierInfoP != NULL)
        {
          //
          // notifierInfo is an array of objects in the API:
          //   "notifierInfo": [ { "key": "k1", "value": "v1" }, { }, ... ]
          //
          // In the database, it is instead stored as an object (it's much shorter):
          //   "notifierInfo": { "k1": "v1", ... }
          //
          // This is amended here
          //
          KjNode* kvs = ngsildKeyValuesToDatabaseModel(notifierInfoP, "notifierInfo");
          kjChildAdd(patchTree, kvs);
        }
      }

      nItemP = next;
    }

    // Finally, remove the "notification" item from the patch-tree
    kjChildRemove(patchTree, notificationP);
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// fixDbSubscription -
//
// As long long members are respresented as "xxx": { "$numberLong": "1234565678901234" }
// and this gives an error when trying to Update this, we simply change the object to an int.
//
// In a Subscription, this must be done for "expiration", and "throttling".
//
static void fixDbSubscription(KjNode* dbSubscriptionP)
{
  KjNode* nodeP;

  //
  // If 'expiration' is an Object, it means it's a NumberLong and it is then changed to a double
  //
  if ((nodeP = kjLookup(dbSubscriptionP, "expiration")) != NULL)
  {
    if (nodeP->type == KjObject)
    {
      char*      expirationString = nodeP->value.firstChildP->value.s;
      double     expiration       = strtold(expirationString, NULL);

      nodeP->type    = KjFloat;
      nodeP->value.f = expiration;
    }
  }

  //
  // If 'throttling' is an Object, it means it's a NumberLong and it is then changed to a double
  //
  if ((nodeP = kjLookup(dbSubscriptionP, "throttling")) != NULL)
  {
    if (nodeP->type == KjObject)
    {
      char*      throttlingString = nodeP->value.firstChildP->value.s;
      long long  throttling       = strtold(throttlingString, NULL);

      nodeP->type    = KjFloat;
      nodeP->value.f = throttling;
    }
  }
}



// -----------------------------------------------------------------------------
//
// subCacheItemUpdateEntities -
//
static bool subCacheItemUpdateEntities(CachedSubscription* cSubP, KjNode* entityArray)
{
  cSubP->entityIdInfos.empty();
  for (KjNode* entityP = entityArray->value.firstChildP; entityP != NULL; entityP = entityP->next)
  {
    KjNode*      idP          = kjLookup(entityP, "id");
    KjNode*      idPatternP   = kjLookup(entityP, "idPattern");
    KjNode*      typeP        = kjLookup(entityP, "type");
    EntityInfo*  entityInfoP;

    if ((idP == NULL) && (idPatternP == NULL))
      entityInfoP = new EntityInfo(".*", std::string(typeP->value.s), "true", false);
    else if (idP != NULL)
      entityInfoP = new EntityInfo(std::string(idP->value.s), std::string(typeP->value.s), "false", false);
    else
      entityInfoP = new EntityInfo(std::string(idPatternP->value.s), std::string(typeP->value.s), "true", false);

    cSubP->entityIdInfos.push_back(entityInfoP);
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// subCacheItemUpdateWatchedAttributes -
//
static bool subCacheItemUpdateWatchedAttributes(CachedSubscription* cSubP, KjNode* itemP)
{
  cSubP->notifyConditionV.empty();

  int  attrs = 0;
  for (KjNode* attrNodeP = itemP->value.firstChildP; attrNodeP != NULL; attrNodeP = attrNodeP->next)
  {
    char* longName = orionldAttributeExpand(orionldState.contextP, attrNodeP->value.s, true, NULL);

    cSubP->notifyConditionV.push_back(longName);
    ++attrs;
  }

  if ((int) cSubP->notifyConditionV.size() != attrs)
    LM_RE(false, ("Expected %d items in watchedAttributes list - there are %d !!!", attrs, cSubP->notifyConditionV.size()));

  return true;
}



// -----------------------------------------------------------------------------
//
// subCacheItemUpdateGeoQ -
//
static bool subCacheItemUpdateGeoQ(CachedSubscription* cSubP, KjNode* itemP)
{
  KjNode* geometryP     = kjLookup(itemP, "geometry");
  KjNode* coordinatesP  = kjLookup(itemP, "coordinates");
  KjNode* georelP       = kjLookup(itemP, "georel");
  KjNode* geopropertyP  = kjLookup(itemP, "geoproperty");

  if (geometryP    != NULL) cSubP->expression.geometry    = geometryP->value.s;
  if (georelP      != NULL) cSubP->expression.georel      = georelP->value.s;
  if (geopropertyP != NULL) cSubP->expression.geoproperty = geopropertyP->value.s;

  if (coordinatesP != NULL)
  {
    int   coordsSize = kjFastRenderSize(coordinatesP) + 100;
    char* coords     = kaAlloc(&orionldState.kalloc, coordsSize);

    kjFastRender(coordinatesP, coords);
    cSubP->expression.coords = coords;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// subCacheItemUpdateNotificationEndpoint -
//
static bool subCacheItemUpdateNotificationEndpoint(CachedSubscription* cSubP, KjNode* endpointP)
{
  KjNode* uriP           = kjLookup(endpointP, "uri");
  KjNode* acceptP        = kjLookup(endpointP, "accept");
  KjNode* receiverInfoP  = kjLookup(endpointP, "receiverInfo");
  KjNode* notifierInfoP  = kjLookup(endpointP, "notifierInfo");

  if (uriP != NULL)
  {
    char*           url;
    char*           protocol;
    char*           ip;
    unsigned short  port;
    char*           rest;

    url = strdup(uriP->value.s);
    if (urlParse(cSubP->url, &protocol, &ip, &port, &rest) == true)
    {
      free(cSubP->url);
      cSubP->url      = url;
      cSubP->protocol = protocol;
      cSubP->ip       = ip;
      cSubP->port     = port;
      cSubP->rest     = rest;
    }
    else
      LM_W(("Invalid url '%s'", uriP->value.s));
  }

  if (acceptP != NULL)
  {
    uint32_t acceptMask;
    cSubP->httpInfo.mimeType = mimeTypeFromString(acceptP->value.s, NULL, true, false, &acceptMask);
  }

  if (receiverInfoP != NULL)
  {
    //
    // for each key-value pair in "receiverInfo" -
    // - look it up in cSubP->httpInfo.receiverInfo
    // - replace the value if there, add if not there
    //
    for (KjNode* riP = receiverInfoP->value.firstChildP; riP != NULL; riP = riP->next)
    {
      KjNode*   keyP   = kjLookup(riP, "key");
      KjNode*   valueP = kjLookup(riP, "value");
      KeyValue* kvP    = keyValueLookup(cSubP->httpInfo.receiverInfo, keyP->value.s);

      if (kvP != NULL)
        strncpy(kvP->value, valueP->value.s, sizeof(kvP->value) - 1);
      else
        keyValueAdd(&cSubP->httpInfo.receiverInfo, keyP->value.s, valueP->value.s);
    }
  }

  if (notifierInfoP != NULL)
  {
    //
    // notifierInfo is storeed in cSubP->httpInfo.headers
    // - just set it (in the std::map)
    //
    for (KjNode* riP = notifierInfoP->value.firstChildP; riP != NULL; riP = riP->next)
    {
      KjNode*   keyP   = kjLookup(riP, "key");
      KjNode*   valueP = kjLookup(riP, "value");

      cSubP->httpInfo.headers[keyP->value.s] = valueP->value.s;
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// subCacheItemUpdateNotification -
//
static bool subCacheItemUpdateNotification(CachedSubscription* cSubP, KjNode* itemP)
{
  KjNode* attributesP = kjLookup(itemP, "attributes");
  KjNode* formatP     = kjLookup(itemP, "format");
  KjNode* endpointP   = kjLookup(itemP, "endpoint");

  if (attributesP != NULL)  // Those present in the notification
  {
    cSubP->attributes.empty();
    for (KjNode* attrNodeP = attributesP->value.firstChildP; attrNodeP != NULL; attrNodeP = attrNodeP->next)
    {
      char* longName = orionldAttributeExpand(orionldState.contextP, attrNodeP->value.s, true, NULL);
      cSubP->attributes.push_back(longName);
    }
  }

  if (formatP != NULL)
    cSubP->renderFormat = stringToRenderFormat(formatP->value.s);

  if (endpointP != NULL)
    return subCacheItemUpdateNotificationEndpoint(cSubP, endpointP);

  return true;
}



// -----------------------------------------------------------------------------
//
// subCacheItemUpdate -
//
static bool subCacheItemUpdate(OrionldTenant* tenantP, const char* subscriptionId, KjNode* subscriptionTree)
{
  CachedSubscription* cSubP = subCacheItemLookup(tenantP->tenant, subscriptionId);
  bool                r     = true;

  if (cSubP == NULL)
    LM_RE(false, ("Internal Error (can't find the subscription '%s' in the subscription cache)", subscriptionId));

  cacheSemTake(__FUNCTION__, "Updating a cached subscription");
  subCacheState = ScsSynchronizing;

  for (KjNode* itemP = subscriptionTree->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    if ((strcmp(itemP->name, "subscriptionName") == 0) || (strcmp(itemP->name, "name") == 0))
    {
      // If v1.1-1.3? give error if "subscriptionName" ?
      // If > v1.3?   give error if "name"             ?
      cSubP->name = itemP->value.s;
    }
    else if (strcmp(itemP->name, "description") == 0)
    {
      // The description is only stored in the database ...
      //
      // cSubP->description = itemP->value.s;
    }
    else if (strcmp(itemP->name, "entities") == 0)
    {
      subCacheItemUpdateEntities(cSubP, itemP);
    }
    else if (strcmp(itemP->name, "watchedAttributes") == 0)
    {
      subCacheItemUpdateWatchedAttributes(cSubP, itemP);
    }
    else if (strcmp(itemP->name, "timeInterval") == 0)
    {
      LM_W(("Not Implemented (Orion-LD doesn't implement periodical notifications"));
    }
    else if (strcmp(itemP->name, "lang") == 0)
    {
      cSubP->lang = itemP->value.s;
    }
    else if (strcmp(itemP->name, "q") == 0)
    {
      cSubP->expression.q = itemP->value.s;
    }
    else if (strcmp(itemP->name, "geoQ") == 0)
    {
      subCacheItemUpdateGeoQ(cSubP, itemP);
    }
    else if (strcmp(itemP->name, "csf") == 0)
    {
      //
      // csf is not implemented (only used for Subscriptions on Context Source Registrations)
      //
      // cSubP->csf = itemP->value.s;
    }
    else if (strcmp(itemP->name, "isActive") == 0)
    {
      if (itemP->value.b == true)
      {
        cSubP->isActive = true;
        cSubP->status   = "active";
      }
      else
      {
        cSubP->isActive = false;
        cSubP->status   = "paused";
      }
    }
    else if (strcmp(itemP->name, "notification") == 0)
    {
      subCacheItemUpdateNotification(cSubP, itemP);
    }
    else if ((strcmp(itemP->name, "expires") == 0) || (strcmp(itemP->name, "expiresAt") == 0))
    {
      // Give error for expires/expiresAt and version of NGSI-LD ?
      cSubP->expirationTime = parse8601Time(itemP->value.s);
    }
    else if (strcmp(itemP->name, "throttling") == 0)
    {
      if (itemP->type == KjInt)
        cSubP->throttling = (double) itemP->value.i;
      else if (itemP->type == KjFloat)
        cSubP->throttling = itemP->value.f;
      else
        LM_W(("Invalid type for 'throttling'"));
    }
    else if (strcmp(itemP->name, "scopeQ") == 0)
    {
      LM_W(("Not Implemented (Orion-LD doesn't support Multi-Type (yet)"));
    }
    else if (strcmp(itemP->name, "lang") == 0)
    {
      LM_W(("Not Implemented (Orion-LD doesn't support LanguageProperty just yet"));
    }
    else
    {
      orionldError(OrionldBadRequestData, "Invalid field for subscription patch", itemP->name, 400);
      r = false;
    }
  }

  subCacheState = ScsIdle;
  cacheSemGive(__FUNCTION__, "Updated a cached subscription");
  return r;
}



// ----------------------------------------------------------------------------
//
// orionldPatchSubscription -
//
// 1. Check that orionldState.wildcard[0] is a valid subscription ID (a URI) - 400 Bad Request ?
// 2. Make sure the payload data is a correct Subscription fragment
//    - No values can be NULL
//    - Expand attribute names ans entity types if present
// 3. GET the subscription from mongo, by callinbg dbSubscriptionGet(orionldState.wildcard[0])
// 4. If not found - 404
//
// 5. Go over the fragment (incoming payload data) and modify the 'subscription from mongo':
//    * If the provided Fragment (merge patch) contains members that do not appear within the target (their URIs do
//      not match), those members are added to the target.
//    * the target member value is replaced by value given in the Fragment, if non-null values.
//    * If null values in the Fragment, then remove in the target
//
// 6. Call dbSubscriptionReplace(char* subscriptionId, KjNode* subscriptionTree) to replace the old sub with the new
//    Or, dbSubscriptionUpdate(char* subscriptionId, KjNode* toAddP, KjNode* toRemoveP, KjNode* toUpdate)
//
bool orionldPatchSubscription(void)
{
  PCHECK_URI(orionldState.wildcard[0], true, 0, "Subscription ID must be a valid URI", orionldState.wildcard[0], 400);

  char*   subscriptionId         = orionldState.wildcard[0];
  KjNode* watchedAttributesNodeP = NULL;
  KjNode* timeIntervalNodeP      = NULL;
  KjNode* qP                     = NULL;
  KjNode* geoqP                  = NULL;

  if (pcheckSubscription(orionldState.requestTree, false, &watchedAttributesNodeP, &timeIntervalNodeP, &qP, &geoqP, true) == false)
  {
    LM_E(("pcheckSubscription FAILED"));
    return false;
  }

  KjNode* dbSubscriptionP = dbSubscriptionGet(subscriptionId);

  if (dbSubscriptionP == NULL)
  {
    orionldError(OrionldResourceNotFound, "Subscription not found", subscriptionId, 404);
    return false;
  }


  //
  // Make sure we don't get both watchedAttributed AND timeInterval
  // If so, the PATCH is invalid
  // Right now, timeInterval is not supported, but once it is, if ever, this code will come in handy
  //
  if (timeIntervalNodeP != NULL)
  {
    orionldError(OrionldBadRequestData, "Not Implemented", "Subscription::timeInterval is not implemented", 501);
    return false;
  }

  if ((watchedAttributesNodeP != NULL) && (timeIntervalNodeP != NULL))
  {
    LM_W(("Bad Input (Both 'watchedAttributes' and 'timeInterval' given in Subscription Payload Data)"));
    orionldError(OrionldBadRequestData, "Invalid Subscription Payload Data", "Both 'watchedAttributes' and 'timeInterval' given", 400);
    return false;
  }
  else if (watchedAttributesNodeP != NULL)
  {
    KjNode* dbTimeIntervalNodeP = kjLookup(dbSubscriptionP, "timeInterval");

    if ((dbTimeIntervalNodeP != NULL) && (dbTimeIntervalNodeP->value.i != -1))
    {
      orionldError(OrionldBadRequestData,
                   "Invalid Subscription Payload Data",
                   "Attempt to set 'watchedAttributes' to a Subscription that is of type 'timeInterval'",
                   400);
      return false;
    }
  }
  else if (timeIntervalNodeP != NULL)
  {
    KjNode* dbConditionsNodeP = kjLookup(dbSubscriptionP, "conditions");

    if ((dbConditionsNodeP != NULL) && (dbConditionsNodeP->value.firstChildP != NULL))
    {
      LM_W(("Bad Input (Attempt to set 'timeInterval' to a Subscription that is of type 'watchedAttributes')"));
      orionldError(OrionldBadRequestData, "Invalid Subscription Payload Data", "Attempt to set 'timeInterval' to a Subscription that is of type 'watchedAttributes'", 400);
      return false;
    }
  }


  //
  // Remove Occurrences of $numberLong, i.e. "expiration"
  //
  // FIXME: This is BAD ... shouldn't change the type of these fields
  //
  fixDbSubscription(dbSubscriptionP);
  KjNode* patchBody = kjClone(orionldState.kjsonP, orionldState.requestTree);

  ngsildSubscriptionToAPIv1Datamodel(orionldState.requestTree);

  //
  // After calling ngsildSubscriptionToAPIv1Datamodel, the incoming payload data has beed structured just as the
  // API v1 database model and the original tree (obtained calling dbSubscriptionGet()) can easily be
  // modified.
  // ngsildSubscriptionPatch() performs that modification.
  //
  CachedSubscription* cSubP = subCacheItemLookup(orionldState.tenantP->tenant, subscriptionId);
  LM_TMP(("Q: cSubP->qP at %p", cSubP->qP));
  if (ngsildSubscriptionPatch(dbSubscriptionP, cSubP, orionldState.requestTree, qP, geoqP) == false)
    LM_RE(false, ("KZ: ngsildSubscriptionPatch failed!"));

  //
  // Update modifiedAt
  //
  KjNode* modifiedAtP = kjLookup(dbSubscriptionP, "modifiedAt");

  if (modifiedAtP != NULL)
    modifiedAtP->value.f = orionldState.requestTime;
  else
  {
    modifiedAtP = kjFloat(orionldState.kjsonP, "modifiedAt", orionldState.requestTime);
    kjChildAdd(dbSubscriptionP, modifiedAtP);
  }


  //
  // Overwrite the current Subscription in the database
  //
  if (dbSubscriptionReplace(subscriptionId, dbSubscriptionP) == false)
  {
    orionldError(OrionldInternalError, "Database Error", "patching a subscription", 500);
    return false;
  }

  // Modify the subscription in the subscription cache
  if (subCacheItemUpdate(orionldState.tenantP, subscriptionId, patchBody) == false)
    LM_E(("Internal Error (unable to update the cached subscription '%s' after a PATCH)", subscriptionId));

  // All OK? 204 No Content
  orionldState.httpStatusCode = 204;

  return true;
}
