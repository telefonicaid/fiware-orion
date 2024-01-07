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
#include <unistd.h>                                              // NULL

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildAdd, kjChildRemove, kjString, kjObject, ...
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState, coreContextUrl
#include "orionld/common/dateTime.h"                             // dateTimeFromString
#include "orionld/types/RegCacheItem.h"                          // RegCacheItem
#include "orionld/dbModel/dbModelFromApiRegistration.h"          // Own interface



// -----------------------------------------------------------------------------
//
// dbModelFromApiRegInfoAttrs -
//
void dbModelFromApiRegInfoAttrs(KjNode* attrsP, KjNode* attrNamesV, const char* attrType)
{
  for (KjNode* attrNameP = attrNamesV->value.firstChildP; attrNameP != NULL; attrNameP = attrNameP->next)
  {
    KjNode* attrObjectP   = kjObject(orionldState.kjsonP, NULL);
    KjNode* attrNameNodeP = kjString(orionldState.kjsonP, "name", attrNameP->value.s);
    KjNode* attrTypeNodeP = kjString(orionldState.kjsonP, "type", attrType);

    kjChildAdd(attrObjectP, attrNameNodeP);
    kjChildAdd(attrObjectP, attrTypeNodeP);

    kjChildAdd(attrsP, attrObjectP);
  }
}



// -----------------------------------------------------------------------------
//
// dbModelFromApiRegInfo -
//
// From API:
//   "information": [
//     {
//       "entities": [ { "id": "", "type": "", "idPattern": "" } ],
//       "propertyNames": [ "", "" ],
//       "relationshipNames": [ "", "" ],
//     }
//   ]
//
// To DB:
//   "contextRegistration" : [
//     {
//       "entities" : [
//         {
//           "id" : "urn:ngsi-ld:Vehicle:A456",
//           "type" : "https://uri.etsi.org/ngsi-ld/default-context/Vehicle",
//           "isPattern": "false"
//         }
//       ],
//       "attrs" : [
//         {
//           "name" : "https://uri.etsi.org/ngsi-ld/default-context/brandName",
//           "type" : "Property",
//           "isDomain" : "false"
//         }
//       ],
//       "providingApplication" : "http://my.csource.org:1026"
//     }
//   ]
//
bool dbModelFromApiRegInfo(KjNode* informationP, KjNode* endpointP, RegCacheItem* rciP)
{
  char* endpoint = NULL;

  if (endpointP != NULL)
    endpoint = endpointP->value.s;
  else if (rciP != NULL)
  {
    endpointP = kjLookup(rciP->regTree, "endpoint");
    if (endpointP != NULL)
      endpoint = endpointP->value.s;
  }

  if (endpoint == NULL)
  {
    LM_E(("Internal Error (no endpoint (providing application) found)"));
    return false;
  }

  informationP->name = (char*) "contextRegistration";

  for (KjNode* infoObjP = informationP->value.firstChildP; infoObjP != NULL; infoObjP = infoObjP->next)
  {
    KjNode* entitiesP              = kjLookup(infoObjP, "entities");
    KjNode* propertyNamesP         = kjLookup(infoObjP, "propertyNames");
    KjNode* relationshipNamesP     = kjLookup(infoObjP, "relationshipNames");
    KjNode* attrsP                 = kjArray(orionldState.kjsonP, "attrs");
    KjNode* providingApplicationP  = kjString(orionldState.kjsonP, "providingApplication", endpoint);
    KjNode* isPatternP;

    if (entitiesP != NULL)
    {
      for (KjNode* entitiesObjP = entitiesP->value.firstChildP; entitiesObjP != NULL; entitiesObjP = entitiesObjP->next)
      {
        KjNode* idPatternP             = kjLookup(entitiesObjP, "idPattern");
        KjNode* idP                    = kjLookup(entitiesObjP, "id");

        //
        // If both "id" and "idPattern" are present, then "idPattern" is ignored
        // If only "idPattern", then idPattern => "id" + isPattern == "true"
        // If only "id", then isPattern == "false"
        //
        //
        if ((idP != NULL) && (idPatternP != NULL))
          kjChildRemove(entitiesObjP, idPatternP);
        else if (idPatternP != NULL)
        {
          idPatternP->name = (char*) "id";
          isPatternP = kjString(orionldState.kjsonP, "isPattern", "true");
          kjChildAdd(entitiesObjP, isPatternP);
        }
      }
    }

    if (propertyNamesP)
    {
      kjChildRemove(infoObjP, propertyNamesP);
      dbModelFromApiRegInfoAttrs(attrsP, propertyNamesP, "Property");
    }

    if (relationshipNamesP)
    {
      kjChildRemove(infoObjP, relationshipNamesP);
      dbModelFromApiRegInfoAttrs(attrsP, relationshipNamesP, "Relationship");
    }

    kjChildAdd(infoObjP, attrsP);
    kjChildAdd(infoObjP, providingApplicationP);
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// dbModelFromApiTimeInterval -
//
void dbModelFromApiTimeInterval(KjNode* intervalP)
{
  KjNode* startAtP = kjLookup(intervalP, "startAt");
  KjNode* endAtP   = kjLookup(intervalP, "endAt");
  char    errorString[256];
  double  ts;

  if ((startAtP != NULL) && (startAtP->type == KjString))
  {
    ts = dateTimeFromString(startAtP->value.s, errorString, sizeof(errorString));
    if (ts < 0)
      LM_E(("startAt: %s", errorString));

    startAtP->type    = KjFloat;
    startAtP->value.f = ts;
  }

  if ((endAtP != NULL) && (endAtP->type == KjString))
  {
    ts = dateTimeFromString(endAtP->value.s, errorString, sizeof(errorString));
    if (ts < 0)
      LM_E(("startAt: %s", errorString));

    endAtP->type    = KjFloat;
    endAtP->value.f = ts;
  }
}



// -----------------------------------------------------------------------------
//
// dbModelFromApiRegistration - modify the request tree to match the db model
//
// The tree is not 100% API
//   - all timestamps are floats already
//
bool dbModelFromApiRegistration(KjNode* apiRegistration, RegCacheItem* rciP)
{
  // id => _id
  // expiresAt => expiration  (Floating point)
  // endpoint => contextRegistration::providingApplication (for every array item)
  // registrationName => name
  // information => contextRegistration
  // information::idPattern => contextRegistration::id + contextRegistration::isPattern == "true"
  // information::propertyNames => contextRegistration::attrs {}
  // information::relationshipNames => contextRegistration::attrs {}
  // observationInterval::startAt => double
  // observationInterval::endAt => double
  //
  KjNode* idP                   = kjLookup(apiRegistration, "id");
  KjNode* registrationNameP     = kjLookup(apiRegistration, "registrationName");
  KjNode* informationP          = kjLookup(apiRegistration, "information");
  KjNode* endpointP             = kjLookup(apiRegistration, "endpoint");
  KjNode* observationIntervalP  = kjLookup(apiRegistration, "observationInterval");
  KjNode* managementIntervalP   = kjLookup(apiRegistration, "managementInterval");
  KjNode* expiresAtP            = kjLookup(apiRegistration, "expiresAt");
  KjNode* statusP               = kjString(orionldState.kjsonP, "status", "active");

  if (idP               != NULL) idP->name               = (char*) "_id";
  if (registrationNameP != NULL) registrationNameP->name = (char*) "name";

  if ((endpointP != NULL) || (informationP != NULL))
  {
    if (endpointP != NULL)
      kjChildRemove(apiRegistration, endpointP);
    dbModelFromApiRegInfo(informationP, endpointP, rciP);
  }

  if (observationIntervalP != NULL)
    dbModelFromApiTimeInterval(observationIntervalP);
  if (managementIntervalP != NULL)
    dbModelFromApiTimeInterval(managementIntervalP);

  if (expiresAtP != NULL)
  {
    char errorString[256];

    expiresAtP->name    = (char*) "expiration";
    expiresAtP->value.f = dateTimeFromString(expiresAtP->value.s, errorString, sizeof(errorString));
    expiresAtP->type    = KjFloat;

    if (expiresAtP->value.f < 0)
      LM_W(("expiration/expiresAt: %s", errorString));
  }

  kjChildAdd(apiRegistration, statusP);

  return true;
}
