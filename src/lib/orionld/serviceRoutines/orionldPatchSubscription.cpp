/*
*
* Copyright 2018 FIWARE Foundation e.V.
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
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjChildAdd, ...
#include "kjson/kjRender.h"                                    // kjRender
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo

#include "orionld/common/CHECK.h"                              // STRING_CHECK, ...
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/db/dbConfiguration.h"                        // dbSubscriptionGet
#include "orionld/serviceRoutines/orionldPatchSubscription.h"  // Own Interface



// -----------------------------------------------------------------------------
//
// orionldCheckEntityInfo -
//
static bool orionldCheckEntityInfo(ConnectionInfo* ciP, KjNode* subNodeP, const char* filedName)
{
  return true;
}



// -----------------------------------------------------------------------------
//
// orionldCheckGeoQ -
//
static bool orionldCheckGeoQ(ConnectionInfo* ciP, KjNode* subNodeP, const char* filedName)
{
  return true;
}



// -----------------------------------------------------------------------------
//
// subscriptionPayloadCheck -
//
static bool subscriptionPayloadCheck(ConnectionInfo* ciP, KjNode* subNodeP, bool idCanBePresent)
{
  KjNode* idP                     = NULL;
  KjNode* typeP                   = NULL;
  KjNode* nameP                   = NULL;
  KjNode* descriptionP            = NULL;
  KjNode* entitiesP               = NULL;
  KjNode* watchedAttributesP      = NULL;
  KjNode* timeIntervalP           = NULL;
  KjNode* qP                      = NULL;
  KjNode* geoqP                   = NULL;
  KjNode* csfP                    = NULL;
  KjNode* isActiveP               = NULL;
  KjNode* notificationP           = NULL;
  KjNode* expiresP                = NULL;
  KjNode* throttlingP             = NULL;
  KjNode* temporalqP              = NULL;

  if (subNodeP->type != KjObject)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Subscription", "The payload data for updating a subscription must be a JSON Object");
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  for (KjNode* nodeP = subNodeP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (strcmp(nodeP->name, "id") == 0)
    {
      if (idCanBePresent == false)
      {
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid field for Subscription Update", "Subscription::id");
        ciP->httpStatusCode = SccBadRequest;
        return false;
      }

      DUPLICATE_CHECK(idP, "Subscription::id", nodeP);
      STRING_CHECK(nodeP, nodeP->name);
      URI_CHECK(nodeP, nodeP->name);
    }
    else if (strcmp(nodeP->name, "type") == 0)
    {
      DUPLICATE_CHECK(typeP, "Subscription::type", nodeP);
      STRING_CHECK(nodeP, nodeP->name);

      if (strcmp(nodeP->value.s, "Subscription") != 0)
      {
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid value for Subscription Type", nodeP->value.s);
        ciP->httpStatusCode = SccBadRequest;
        return false;
      }
    }
    else if (strcmp(nodeP->name, "name") == 0)
    {
      DUPLICATE_CHECK(nameP, "Subscription::name", nodeP);
      STRING_CHECK(nodeP, nodeP->name);
    }
    else if (strcmp(nodeP->name, "description") == 0)
    {
      DUPLICATE_CHECK(descriptionP, "Subscription::description", nodeP);
      STRING_CHECK(nodeP, nodeP->name);
    }
    else if (strcmp(nodeP->name, "entities") == 0)
    {
      DUPLICATE_CHECK(entitiesP, "Subscription::entities", nodeP);
      ARRAY_CHECK(nodeP, nodeP->name);
      if (orionldCheckEntityInfo(ciP, nodeP, "Subscription::entities") == false)
        return false;
    }
    else if (strcmp(nodeP->name, "watchedAttributes") == 0)
    {
      DUPLICATE_CHECK(watchedAttributesP, "Subscription::watchedAttributes", nodeP);
      ARRAY_CHECK(nodeP, nodeP->name);
      for (KjNode* itemP = nodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
      {
        STRING_CHECK(itemP, "watchedAttributes item");
      }
    }
    else if (strcmp(nodeP->name, "timeInterval") == 0)
    {
      DUPLICATE_CHECK(timeIntervalP, "Subscription::timeInterval", nodeP);
      INTEGER_CHECK(nodeP, "Subscription::timeInterval");
    }
    else if (strcmp(nodeP->name, "q") == 0)
    {
      DUPLICATE_CHECK(qP, "Subscription::q", nodeP);
      STRING_CHECK(nodeP, "Subscription::q");
    }
    else if (strcmp(nodeP->name, "geoQ") == 0)
    {
      DUPLICATE_CHECK(geoqP, "Subscription::geoQ", nodeP);
      OBJECT_CHECK(nodeP, "Subscription::geoQ");
      if (orionldCheckGeoQ(ciP, nodeP, "Subscription::geoQ") == false)
        return false;
    }
    else if (strcmp(nodeP->name, "csf") == 0)
    {
      DUPLICATE_CHECK(csfP, "Subscription::csf", nodeP);
      STRING_CHECK(nodeP, "Subscription::csf");
    }
    else if (strcmp(nodeP->name, "isActive") == 0)
    {
      DUPLICATE_CHECK(isActiveP, "Subscription::isActive", nodeP);
      BOOL_CHECK(nodeP, "Subscription::isActive");
    }
    else if (strcmp(nodeP->name, "notification") == 0)
    {
      DUPLICATE_CHECK(notificationP, "Subscription::notification", nodeP);
      OBJECT_CHECK(nodeP, "Subscription::notification");
    }
    else if (strcmp(nodeP->name, "expires") == 0)
    {
      DUPLICATE_CHECK(expiresP, "Subscription::expires", nodeP);
      STRING_CHECK(nodeP, "Subscription::expires");
      DATETIME_CHECK(expiresP->value.s, "Subscription::expires");
    }
    else if (strcmp(nodeP->name, "throttling") == 0)
    {
      DUPLICATE_CHECK(throttlingP, "Subscription::throttling", nodeP);
      INTEGER_CHECK(nodeP, "Subscription::throttling");
    }
    else if (strcmp(nodeP->name, "temporalQ") == 0)
    {
      DUPLICATE_CHECK(temporalqP, "Subscription::temporalQ", nodeP);
      OBJECT_CHECK(nodeP, "Subscription::temporalQ");
    }
    else if (strcmp(nodeP->name, "status") == 0)
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Attempt to modify Read-Only attribute", "Subscription::status");
      ciP->httpStatusCode = SccBadRequest;
      return false;
    }
    else
    {
      LM_E(("Unknown field in Subscription fragment: '%s'", nodeP->name));
      orionldErrorResponseCreate(OrionldBadRequestData, "Unknown field in Subscription fragment", nodeP->name);
      ciP->httpStatusCode = SccBadRequest;
      return false;
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// kjChildAddOrReplace -
//
// FIXME: move to kjson library - also used in orionldPatchRegistration.cpp
//
void kjChildAddOrReplace(KjNode* container, const char* itemName, KjNode* replacementP)
{
  KjNode* itemToReplace = kjLookup(container, itemName);

  if (itemToReplace == NULL)
  {
    LM_TMP(("PATCH: Adding %s", itemName));
    kjChildAdd(container, replacementP);
  }
  else
  {
    LM_TMP(("PATCH: Replacing %s", itemName));
    itemToReplace->type  = replacementP->type;
    itemToReplace->value = replacementP->value;
    // KjNode::cSum and KjNode::valueString aren't used
  }
}



// ----------------------------------------------------------------------------
//
// ngsildSubscriptionPatch -
//
static void ngsildSubscriptionPatch(KjNode* dbSubscriptionP, KjNode* patchTree)
{
  for (KjNode* fragmentP = patchTree->value.firstChildP; fragmentP != NULL; fragmentP = fragmentP->next)
  {
    if (fragmentP->type == KjNull)
    {
      KjNode* toRemove = kjLookup(dbSubscriptionP, fragmentP->name);

      if (toRemove != NULL)
      {
        LM_TMP(("SPAT: Calling kjChildRemove for '%s'", fragmentP->name));
        kjChildRemove(dbSubscriptionP, toRemove);

        //
        // Dangerous to remove without checking ... what if we remove "Subscription::endpoint" ... ???
        //
      }
      else
        LM_TMP(("SPAT: Can't remove '%s' - it's not present in the DB", fragmentP->name));
    }
    else
    {
      LM_TMP(("SPAT: Calling kjChildAddOrReplace for '%s'", fragmentP->name));
      kjChildAddOrReplace(dbSubscriptionP, fragmentP->name, fragmentP);
    }
  }
}



// -----------------------------------------------------------------------------
//
// ngsildSubscriptionToAPIv1Datamodel -
//
// For the PATCH (step 5) the difference of the APIv1 database model and the NGSI-LD payload data for a Subscription:
//
// PAYLOAD of an NGSI-LD Subscription
// {
//   "id": "urn:ngsi-ld:subscriptions:01",     => "id" => "_id"
//   "type": "Subscription",                   => Not in DB and not needed
//   "name": "Test subscription S01",          => NEW and added to the datamodel
//   "description": "XXX",                     => NEW and added to the datamodel
//   "entities": [                             => SAME, but remember, "isTypePattern" : false (default value)
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
//   "expires": "2028-12-31T10:00:00",        => "expires" => "expiration"
//   "throttling": 5                          => SAME
// }
//
// The subscription saved in mongo (APIv1) looks like this:
//
// {
//   "_id" : "urn:ngsi-ld:subscriptions:01",
//   "expiration" : NumberLong(1861869600),
//   "reference" : "http://valid.url/url",
//   "custom" : false,
//   "mimeType" : "application/ld+json",
//   "throttling" : NumberLong(5),
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
    if (strcmp(fragmentP->name, "type") == 0)
    {
      // Just skip it - don't want "type: Subscription" in the DB. Not needed
    }
    else if (strcmp(fragmentP->name, "entities") == 0)
    {
      // Here we might need to add "isTypePattern" : false to every item in the Array ...
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
    else if (strcmp(fragmentP->name, "expires") == 0)
      fragmentP->name = (char*) "expiration";
  }

  //
  // "q" and "geoQ" must be treated together.
  // That's why this was delayed until after the loop.
  //
  //
  // If "geoQ" present, change its name to "expression"
  // If "q" present:
  //   add "q" to "expression"
  //   if "expression" doesn't exist, create it
  //
  KjNode* expressionP = NULL;
  if (geoqP != NULL)
  {
    geoqP->name = (char*) "expression";
    expressionP = geoqP;
  }

  if (qP != NULL)
  {
    if (expressionP == NULL)
      expressionP = kjObject(orionldState.kjsonP, "expression");
    kjChildAdd(expressionP, qP);
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
        KjNode* uriP    = kjLookup(nItemP, "uri");
        KjNode* acceptP = kjLookup(nItemP, "accept");

        if (uriP != NULL)
        {
          uriP->name = (char*) "reference";
          kjChildAdd(patchTree, nItemP);
        }

        if (acceptP != NULL)
        {
          uriP->name = (char*) "mimeType";
          kjChildAdd(patchTree, nItemP);
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

  if ((nodeP = kjLookup(dbSubscriptionP, "expiration")) != NULL)
  {
    char*      expirationString = nodeP->value.firstChildP->value.s;
    long long  expiration       = strtol(expirationString, NULL, 10);

    nodeP->type    = KjInt;
    nodeP->value.i = expiration;
  }

  if ((nodeP = kjLookup(dbSubscriptionP, "throttling")) != NULL)
  {
    char*      throttlingString = nodeP->value.firstChildP->value.s;
    long long  throttling       = strtol(throttlingString, NULL, 10);

    nodeP->type    = KjInt;
    nodeP->value.i = throttling;
  }
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
bool orionldPatchSubscription(ConnectionInfo* ciP)
{
  char* subscriptionId = orionldState.wildcard[0];

  LM_TMP(("SPAT: orionldPatchSubscription: subscriptionId == '%s'", subscriptionId));

  if ((urlCheck(subscriptionId, NULL) == false) && (urnCheck(subscriptionId, NULL) == false))
  {
    ciP->httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(OrionldBadRequestData, "Subscription ID must be a valid URI", subscriptionId);
    return false;
  }

  LM_TMP(("SPAT: Calling subscriptionPayloadCheck"));
  if (subscriptionPayloadCheck(ciP, orionldState.requestTree, false) == false)
  {
    LM_E(("subscriptionPayloadCheck FAILED"));
    return false;
  }

  LM_TMP(("SPAT: Calling dbSubscriptionGet"));
  KjNode* dbSubscriptionP = dbSubscriptionGet(subscriptionId);

  // <DEBUG>
  char buffer[1024];
  kjRender(orionldState.kjsonP, dbSubscriptionP, buffer, sizeof(buffer));
  LM_TMP(("SPAT: DB tree: '%s'", buffer));
  // </DEBUG>
  LM_TMP(("SPAT: dbSubscriptionGet returned: %p", dbSubscriptionP));

  if (dbSubscriptionP == NULL)
  {
    ciP->httpStatusCode = SccNotFound;
    orionldErrorResponseCreate(OrionldBadRequestData, "Subscription not found", subscriptionId);
    return false;
  }

  //
  // Remove Occurrences of $numberLong, i.e. "expiration"
  //
  // FIXME: This is BAD ... shouldn't change the type of these fields
  //
  fixDbSubscription(dbSubscriptionP);


  LM_TMP(("SPAT: Converting the NGSI-LD Subscription into the APIv1 database model"));
  ngsildSubscriptionToAPIv1Datamodel(orionldState.requestTree);

  //
  // After calling ngsildSubscriptionToAPIv1Datamodel, the incoming payload data has beed structured just as the
  // API v1 database model and the original tree (obtained calling dbSubscriptionGet()) can easily be
  // modified.
  // ngsildSubscriptionPatch() perfoirms that modification
  //
  LM_TMP(("SPAT: Going over the payload data to patch the subscription as a KjNode tree"));
  ngsildSubscriptionPatch(dbSubscriptionP, orionldState.requestTree);

  // <DEBUG>
  kjRender(orionldState.kjsonP, dbSubscriptionP, buffer, sizeof(buffer));
  LM_TMP(("SPAT: PATCHed tree: '%s'", buffer));
  // </DEBUG>

  //
  // Overwrite the current Subscription in the database
  //
  dbSubscriptionReplace(subscriptionId, dbSubscriptionP);

  // All OK? 204
  ciP->httpStatusCode = SccNoContent;

  return true;
}
