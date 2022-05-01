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
#include "logMsg/logMsg.h"                                     // LM_*

extern "C"
{
#include "kbase/kMacros.h"                                     // K_VEC_SIZE
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjString, kjChildAdd, ...
}

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/context/orionldCoreContext.h"                // ORIONLD_CORE_CONTEXT_URL
#include "orionld/kjTree/kjTreeLog.h"                          // kjTreeLog
#include "orionld/dbModel/dbModelFromApiSubscription.h"        // Own interface



// -----------------------------------------------------------------------------
//
// dbModelFromApiKeyValues -
//
static KjNode* dbModelFromApiKeyValues(KjNode* kvObjectArray, const char* name)
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
// dbModelFromApiSubscription -
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
// Minimum NGSIv1 Subscription:  (See ngsild_minimal_v2_subscription.test)
// {
//   "_id" : REGEX(.*),
//   "expiration" : -1
//   "reference" : "http://127.0.0.1:9997/notify",
//   "custom" : false,
//   "mimeType" : "application/json",
//   "throttling" : -1,
//   "servicePath" : "/#",
//   "status" : "active",
//   "entities" : [
//     {
//       "id" : "E1",
//       "isPattern" : "false"
//     }
//   ],
//   "attrs" : [ ],
//   "metadata" : [ ],
//   "blacklist" : false,
//   "createdAt" : REGEX(.*),
//   "modifiedAt" : REGEX(.*),
//   "conditions" : [ ],
//   "expression" : {
//     "q" : "",
//     "mq" : "",
//     "geometry" : "",
//     "coords" : "",
//     "georel" : "",
//     "geoproperty" : ""
//   },
//   "format" : "JSON"
// }
//
bool dbModelFromApiSubscription(KjNode* apiSubscriptionP, bool patch)
{
  KjNode* entitiesP           = NULL;
  KjNode* qP                  = NULL;
  KjNode* mqP                 = NULL;
  KjNode* geoqP               = NULL;
  KjNode* notificationP       = NULL;
  KjNode* expressionP         = NULL;
  KjNode* throttlingP         = NULL;
  KjNode* attrsP              = NULL;
  KjNode* formatP             = NULL;
  KjNode* watchedAttributesP  = NULL;
  KjNode* isActiveP           = NULL;

  kjTreeLog(apiSubscriptionP, "Incoming");
  LM_TMP(("subP->lastChild: '%s' (%s)", apiSubscriptionP->lastChild->name, kjValueType(apiSubscriptionP->lastChild->type)));
  //
  // Loop over the patch-tree and modify to make it compatible with the database model for APIv1
  //
  for (KjNode* fragmentP = apiSubscriptionP->value.firstChildP; fragmentP != NULL; fragmentP = fragmentP->next)
  {
    if (strcmp(fragmentP->name, "type") == 0 || strcmp(fragmentP->name, "@type") == 0)
    {
      // Just skip it - don't want "type: Subscription" in the DB. Not needed
      continue;
    }
    else if (strcmp(fragmentP->name, "entities") == 0)
    {
      entitiesP = fragmentP;
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
    {
      watchedAttributesP = fragmentP;
      fragmentP->name = (char*) "conditions";
    }
    else if (strcmp(fragmentP->name, "q") == 0)
    {
      qP = fragmentP;
      LM_TMP(("Got a 'q': '%s'", qP->value.s));
    }
    else if (strcmp(fragmentP->name, "mq") == 0)  // Not NGSI-LD, but added in qFix() (orionldPostSubscriptions.cpp)
    {
      mqP = fragmentP;
      LM_TMP(("Got an 'mq': '%s'", mqP->value.s));
    }
    else if (strcmp(fragmentP->name, "geoQ") == 0)
      geoqP = fragmentP;
    else if (strcmp(fragmentP->name, "isActive") == 0)
    {
      isActiveP = fragmentP;

      //
      // Must change name to "status" and change type from Bool to String
      //
      fragmentP->name = (char*) "status";
      fragmentP->type = KjString;

      fragmentP->value.s = (fragmentP->value.b == true)? (char*) "active" : (char*) "inactive";
    }
    else if (strcmp(fragmentP->name, "notification") == 0)
    {
      notificationP = fragmentP;
      LM_TMP(("Got a notification"));
    }
    else if ((strcmp(fragmentP->name, "expires") == 0) || (strcmp(fragmentP->name, "expiresAt") == 0))
    {
      fragmentP->name    = (char*) "expiration";
      fragmentP->type    = KjFloat;
      fragmentP->value.f = parse8601Time(fragmentP->value.s);  // FIXME: Already done in pcheckSubscription() ...
    }
    else if (strcmp(fragmentP->name, "throttling") == 0)
      throttlingP = fragmentP;
  }

  // If it is not a PATCH - then some mandatory fields (for the DB) must be added
  if (patch == false)
  {
    if (entitiesP == NULL)
    {
      entitiesP = kjArray(orionldState.kjsonP, "entities");
      kjChildAdd(apiSubscriptionP, entitiesP);
    }

    if (throttlingP == NULL)
    {
      throttlingP = kjInteger(orionldState.kjsonP, "throttling", 0);
      kjChildAdd(apiSubscriptionP, throttlingP);
    }
  }

  if (geoqP != NULL)
  {
    //
    // As "geoQ" in NGSI-LD Subscription contains 3-4/6 fields of what is "expression" in the DB Model
    // if present, it is simply renamed to "expression"
    //
    geoqP->name = (char*) "expression";
    expressionP = geoqP;

    // If 'geoproperty' is not there, add it:
    KjNode* geopropertyP = kjLookup(expressionP, "geoproperty");
    if ((geopropertyP == NULL) && (patch == false))
    {
      geopropertyP = kjString(orionldState.kjsonP, "geoproperty", "");
      kjChildAdd(expressionP, geopropertyP);
    }
  }
  else if (patch == false)
  {
    // If "geoQ" is not present, then an empty "expression" is inserted
    expressionP = kjObject(orionldState.kjsonP, "expression");
    kjChildAdd(apiSubscriptionP, expressionP);

    // And, as the geo fields are always present even if empty, we need to add them here:
    const char* fields[] = { "geometry", "coords", "georel", "geoproperty" };
    for (unsigned int ix = 0; ix < K_VEC_SIZE(fields); ix++)
    {
      KjNode* nodeP = kjString(orionldState.kjsonP, fields[ix], "");
      kjChildAdd(expressionP, nodeP);
    }
  }

  if (patch == false)
  {
    // q
    if (qP != NULL)
      kjChildRemove(apiSubscriptionP, qP);
    else
      qP = kjString(orionldState.kjsonP, "q", "");

    kjChildAdd(expressionP, qP);

    // mq
    if (mqP != NULL)
      kjChildRemove(apiSubscriptionP, mqP);
    else
      mqP = kjString(orionldState.kjsonP, "mq", "");
    kjChildAdd(expressionP, mqP);
  }

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
        attrsP = nItemP;

        // Change name to "attrs" and move up to toplevel
        nItemP->name = (char*) "attrs";
        kjChildRemove(notificationP, nItemP);
        kjChildAdd(apiSubscriptionP, nItemP);
        kjTreeLog(apiSubscriptionP, "with attrs");
      }
      else if (strcmp(nItemP->name, "format") == 0)
      {
        formatP = nItemP;
        // Keep the name, just move the node up to toplevel
        kjChildRemove(notificationP, formatP);
        kjChildAdd(apiSubscriptionP, formatP);
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
          kjChildRemove(nItemP, uriP);
          kjChildAdd(apiSubscriptionP, uriP);
        }

        if (acceptP != NULL)
        {
          acceptP->name = (char*) "mimeType";
          kjChildRemove(nItemP, acceptP);
          kjChildAdd(apiSubscriptionP, acceptP);
        }
        else if (patch == false)
        {
          acceptP = kjString(orionldState.kjsonP, "mimeType", "application/json");
          kjChildAdd(apiSubscriptionP, acceptP);
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
          KjNode* headers = dbModelFromApiKeyValues(receiverInfoP, "headers");
          kjChildAdd(apiSubscriptionP, headers);
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
          KjNode* kvs = dbModelFromApiKeyValues(notifierInfoP, "notifierInfo");
          kjChildAdd(apiSubscriptionP, kvs);
        }
      }

      nItemP = next;
    }

    // Remove the "notification" item from the patch-tree
    kjChildRemove(apiSubscriptionP, notificationP);
  }

  //
  // The rest of the function is to add mandatory fields to the DB, in case they're not present already
  // None of that is for PATCH
  //
  if (patch == true)
    return true;

  if (attrsP == NULL)
  {
    attrsP = kjArray(orionldState.kjsonP, "attrs");
    kjChildAdd(apiSubscriptionP, attrsP);
  }

  if (formatP == NULL)
  {
    formatP = kjString(orionldState.kjsonP, "format", "normalized");
    kjChildAdd(apiSubscriptionP, formatP);
  }

  if (watchedAttributesP == NULL)
  {
    watchedAttributesP = kjArray(orionldState.kjsonP, "conditions");
    kjChildAdd(apiSubscriptionP, watchedAttributesP);
  }

  if (isActiveP == NULL)
  {
    isActiveP = kjString(orionldState.kjsonP, "status", "active");
    kjChildAdd(apiSubscriptionP, isActiveP);
  }

  // Adding a few fields not used in NGSI-LD but in NGSIv2
  KjNode* customNodeP      = kjBoolean(orionldState.kjsonP, "custom", false);
  KjNode* servicePathNodeP = kjString(orionldState.kjsonP, "servicePath", "/#");
  KjNode* blacklistNodeP   = kjBoolean(orionldState.kjsonP, "blacklist", false);

  kjChildAdd(apiSubscriptionP, customNodeP);
  kjChildAdd(apiSubscriptionP, servicePathNodeP);
  kjChildAdd(apiSubscriptionP, blacklistNodeP);

  // And finally, the @context
  if (orionldState.payloadContextNode != NULL)
  {
    orionldState.payloadContextNode->name = (char*) "ldContext";
    kjChildAdd(apiSubscriptionP, orionldState.payloadContextNode);
  }
  else  // Core Context
  {
    KjNode* contextP = kjString(orionldState.kjsonP, "ldContext", ORIONLD_CORE_CONTEXT_URL_V1_0);
    kjChildAdd(apiSubscriptionP, contextP);
  }

  return true;
}
