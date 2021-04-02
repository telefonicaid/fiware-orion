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
extern "C"
{
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjChildAdd, ...
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/payloadCheck/pcheckUri.h"                    // pcheckUri
#include "orionld/payloadCheck/pcheckSubscription.h"           // pcheckSubscription
#include "orionld/db/dbConfiguration.h"                        // dbSubscriptionGet
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
static bool ngsildSubscriptionPatch(ConnectionInfo* ciP, KjNode* dbSubscriptionP, KjNode* patchTree, KjNode* qP, KjNode* expressionP)
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
          orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Subscription Fragment - attempt to remove a mandatory field", fragmentP->name);
          orionldState.httpStatusCode = 400;
          return false;
        }

        kjChildRemove(dbSubscriptionP, toRemove);
      }
    }
    else
    {
      if ((fragmentP != qP) && (fragmentP != expressionP))
        kjChildAddOrReplace(dbSubscriptionP, fragmentP->name, fragmentP);
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
    else if (strcmp(fragmentP->name, "expires") == 0)
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
        KjNode* uriP    = kjLookup(nItemP, "uri");
        KjNode* acceptP = kjLookup(nItemP, "accept");

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
  char* detail;

  if (pcheckUri(subscriptionId, true, &detail) == false)
  {
    orionldState.httpStatusCode = 400;
    orionldErrorResponseCreate(OrionldBadRequestData, "Subscription ID must be a valid URI", subscriptionId);  // FIXME: Include 'detail' and name (subscriptionId)
    return false;
  }

  KjNode* watchedAttributesNodeP = NULL;
  KjNode* timeIntervalNodeP      = NULL;
  KjNode* qP                     = NULL;
  KjNode* geoqP                  = NULL;

  if (pcheckSubscription(orionldState.requestTree, false, &watchedAttributesNodeP, &timeIntervalNodeP, &qP, &geoqP) == false)
  {
    LM_E(("pcheckSubscription FAILED"));
    return false;
  }

  KjNode* dbSubscriptionP = dbSubscriptionGet(subscriptionId);

  if (dbSubscriptionP == NULL)
  {
    orionldErrorResponseCreate(OrionldResourceNotFound, "Subscription not found", subscriptionId);
    orionldState.httpStatusCode = 404;
    return false;
  }


  //
  // Make sure we don't get both watchedAttributed AND timeInterval
  // If so, the PATCH is invalid
  // Right now, timeInterval id not supported, but once it is, if ever, this code will come in handy
  //
  if (timeIntervalNodeP != NULL)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Not Implemented", "Subscription::timeInterval is not implemented");
    orionldState.httpStatusCode = 501;
    return false;
  }

  if ((watchedAttributesNodeP != NULL) && (timeIntervalNodeP != NULL))
  {
    LM_W(("Bad Input (Both 'watchedAttributes' and 'timeInterval' given in Subscription Payload Data)"));
    orionldState.httpStatusCode = 400;
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Subscription Payload Data", "Both 'watchedAttributes' and 'timeInterval' given");
    return false;
  }
  else if (watchedAttributesNodeP != NULL)
  {
    KjNode* dbTimeIntervalNodeP = kjLookup(dbSubscriptionP, "timeInterval");

    if ((dbTimeIntervalNodeP != NULL) && (dbTimeIntervalNodeP->value.i != -1))
    {
      LM_W(("Bad Input (Attempt to set 'watchedAttributes' to a Subscription that is of type 'timeInterval'"));
      orionldState.httpStatusCode = 400;
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Subscription Payload Data", "Attempt to set 'watchedAttributes' to a Subscription that is of type 'timeInterval'");
      return false;
    }
  }
  else if (timeIntervalNodeP != NULL)
  {
    KjNode* dbConditionsNodeP = kjLookup(dbSubscriptionP, "conditions");

    if ((dbConditionsNodeP != NULL) && (dbConditionsNodeP->value.firstChildP != NULL))
    {
      LM_W(("Bad Input (Attempt to set 'timeInterval' to a Subscription that is of type 'watchedAttributes')"));
      orionldState.httpStatusCode = 400;
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Subscription Payload Data", "Attempt to set 'timeInterval' to a Subscription that is of type 'watchedAttributes'");
      return false;
    }
  }


  //
  // Remove Occurrences of $numberLong, i.e. "expiration"
  //
  // FIXME: This is BAD ... shouldn't change the type of these fields
  //
  fixDbSubscription(dbSubscriptionP);
  ngsildSubscriptionToAPIv1Datamodel(orionldState.requestTree);

  //
  // After calling ngsildSubscriptionToAPIv1Datamodel, the incoming payload data has beed structured just as the
  // API v1 database model and the original tree (obtained calling dbSubscriptionGet()) can easily be
  // modified.
  // ngsildSubscriptionPatch() performs that modification
  //
  if (ngsildSubscriptionPatch(ciP, dbSubscriptionP, orionldState.requestTree, qP, geoqP) == false)
    return false;

  // Update modifiedAt
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
  dbSubscriptionReplace(subscriptionId, dbSubscriptionP);

  // All OK? 204 No Content
  orionldState.httpStatusCode = 204;

  return true;
}
