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

#include "orionld/common/orionldState.h"                         // orionldState, coreContextUrl
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
bool dbModelFromApiRegInfo(KjNode* informationP, KjNode* endpointP)
{
  informationP->name = (char*) "contextRegistration";

  for (KjNode* infoObjP = informationP->value.firstChildP; infoObjP != NULL; infoObjP = infoObjP->next)
  {
    KjNode* entitiesP              = kjLookup(infoObjP, "entities");
    KjNode* propertyNamesP         = kjLookup(infoObjP, "propertyNames");
    KjNode* relationshipNamesP     = kjLookup(infoObjP, "relationshipNames");
    KjNode* attrsP                 = kjArray(orionldState.kjsonP, "attrs");
    KjNode* providingApplicationP  = kjString(orionldState.kjsonP, "providingApplication", endpointP->value.s);
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
        {
          kjChildRemove(entitiesObjP, idPatternP);
          idPatternP = NULL;
        }
        else if (idPatternP != NULL)
          idPatternP->name = (char*) "id";

        isPatternP = kjString(orionldState.kjsonP, "isPattern", (idPatternP != NULL)? "true" : "false");
        kjChildAdd(entitiesObjP, isPatternP);
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
// dbModelFromApiRegistration - modify the request tree to match the db model
//
// The tree is not 100% API
//   - all timestamps are floats already
//
bool dbModelFromApiRegistration(KjNode* apiRegistration)
{
  // id => _id
  // expiresAt => expiration  (Floating point)
  // endpoint => contextRegistration::providingApplication (for every array item)
  // registrationName => name
  // information => contextRegistration
  // information::idPattern => contextRegistration::id + contextRegistration::isPattern == "true"
  // information::propertyNames => contextRegistration::attrs {}
  // information::relationshipNames => contextRegistration::attrs {}

  KjNode* idP               = kjLookup(apiRegistration, "id");
  KjNode* expiresAtP        = kjLookup(apiRegistration, "expiresAt");
  KjNode* registrationNameP = kjLookup(apiRegistration, "registrationName");
  KjNode* informationP      = kjLookup(apiRegistration, "information");
  KjNode* endpointP         = kjLookup(apiRegistration, "endpoint");
  KjNode* statusP           = kjString(orionldState.kjsonP, "status", "active");

  if (idP               != NULL) idP->name               = (char*) "_id";
  if (expiresAtP        != NULL) expiresAtP->name        = (char*) "expiration";
  if (registrationNameP != NULL) registrationNameP->name = (char*) "name";

  kjChildRemove(apiRegistration, endpointP);
  dbModelFromApiRegInfo(informationP, endpointP);
  kjChildAdd(apiRegistration, statusP);

  return true;
}
