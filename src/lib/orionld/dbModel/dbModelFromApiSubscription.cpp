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
#include "kbase/kMacros.h"                                     // K_VEC_SIZE
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjString, kjChildAdd, ...
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/common/dateTime.h"                           // dateTimeFromString
#include "orionld/context/orionldCoreContext.h"                // orionldCoreContextP
#include "orionld/context/orionldContextSimplify.h"            // orionldContextSimplify
#include "orionld/dbModel/dbModelFromApiKeyValues.h"           // dbModelFromApiKeyValues
#include "orionld/dbModel/dbModelFromApiCoordinates.h"         // dbModelFromApiCoordinates
#include "orionld/dbModel/dbModelFromGeometry.h"               // dbModelFromGeometry
#include "orionld/dbModel/dbModelFromGeorel.h"                 // dbModelFromGeorel
#include "orionld/dbModel/dbModelFromApiSubscription.h"        // Own interface



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
//     "geometry": "Point",                   => "geoQ.geometry" => "expression.geometry"
//     "coordinates": [1,2],                  => "geoQ.coordinates" => "expression.coords"
//     "georel": "near;maxDistance==1",       => "geoQ.georel" => "expression.georel"
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
//     "geometry" : "point",
//     "coords" : "1,2",
//     "georel" : "near;maxDistance:1"
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
      qP = fragmentP;
    else if (strcmp(fragmentP->name, "mq") == 0)  // Not NGSI-LD, but added by orionldPostSubscriptions(), for NGSIv2
      mqP = fragmentP;
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
      notificationP = fragmentP;
    else if ((strcmp(fragmentP->name, "expires") == 0) || (strcmp(fragmentP->name, "expiresAt") == 0))
    {
      char errorString[256];

      fragmentP->name    = (char*) "expiration";
      fragmentP->type    = KjFloat;
      fragmentP->value.f = dateTimeFromString(fragmentP->value.s, errorString, sizeof(errorString));

      if (fragmentP->value.f < 0)
        LM_E(("expiration/expiresAt: %s", errorString));
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
    KjNode* coordinatesP = kjLookup(geoqP, "coordinates");
    KjNode* geometryP    = kjLookup(geoqP, "geometry");
    KjNode* georelP      = kjLookup(geoqP, "georel");

    //
    // As "geoQ" in NGSI-LD Subscription contains many fields (4 out of 6) of what is "expression" in the DB Model
    // it is simply renamed to "expression" and then 'q' and 'mq' are added to it
    //
    geoqP->name = (char*) "expression";
    expressionP = geoqP;

    // Change Point to point
    if (geometryP != NULL)
      geometryP->value.s = (char*) dbModelFromGeometry(geometryP->value.s);

    // near;maxDistance==X => near;maxDistance:X
    if (georelP != NULL)
      georelP->value.s = (char*) dbModelFromGeorel(georelP->value.s);

    // Change "coordinates" to "coord", and remove '[]'
    if (coordinatesP != NULL)  // Can't be NULL !!!
    {
      if (dbModelFromApiCoordinates(coordinatesP, "geoQ::coordinates", geometryP) == false)
        return false;
    }

    //
    // If 'geoproperty' is not there, add it:
    // Database Model decision. Perhaps a bad one ...
    // I could set it to "location", as that's the default value ...
    //
    KjNode* geopropertyP = kjLookup(geoqP, "geoproperty");
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
  kjTreeLog(notificationP, "notificationP", LmtSR);
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
      }
      else if (strcmp(nItemP->name, "format") == 0)
      {
        formatP = nItemP;
        // Keep the name, just move the node up to toplevel
        kjChildRemove(notificationP, formatP);
        kjChildAdd(apiSubscriptionP, formatP);
      }
      else if (strcmp(nItemP->name, "showChanges") == 0)
      {
        kjChildRemove(notificationP, nItemP);
        kjChildAdd(apiSubscriptionP, nItemP);
      }
      else if (strcmp(nItemP->name, "sysAttrs") == 0)
      {
        kjChildRemove(notificationP, nItemP);
        kjChildAdd(apiSubscriptionP, nItemP);
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
    // Simplify if possible
    int items;
    if (orionldState.payloadContextNode->type == KjArray)
    {
      orionldState.payloadContextNode = orionldContextSimplify(orionldState.payloadContextNode, &items);

      if (items == 1)
      {
        orionldState.payloadContextNode->type  = orionldState.payloadContextNode->value.firstChildP->type;
        orionldState.payloadContextNode->value = orionldState.payloadContextNode->value.firstChildP->value;
      }
    }

    if (orionldState.payloadContextNode->type != KjString)
    {
      // The NGSI-LD spec states (soon) that it MUST be a string
      // BUT, that's for the "notification context"
      //
      // For now, I'll just overwrite the context with the Core Context
      //
      orionldState.payloadContextNode->type    = KjString;
      orionldState.payloadContextNode->value.s = orionldCoreContextP->url;
      LM_W(("Warning - the context is not a string - changing it for the Core Context (API Spec v1.6)"));
    }

    orionldState.payloadContextNode->name = (char*) "ldContext";
    kjChildAdd(apiSubscriptionP, orionldState.payloadContextNode);
  }
  else  // Core Context
  {
    char*   contextUrl = (orionldState.contextP != NULL)? orionldState.contextP->url : coreContextUrl;
    KjNode* ldContextP = kjString(orionldState.kjsonP, "ldContext", contextUrl);
    kjChildAdd(apiSubscriptionP, ldContextP);
  }

  return true;
}
