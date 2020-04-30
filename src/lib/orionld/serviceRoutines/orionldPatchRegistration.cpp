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
#include "kjson/kjLookup.h"                                     // kjLookup
#include "kjson/kjBuilder.h"                                    // kjChildAdd, ...
}

#include "logMsg/logMsg.h"                                      // LM_*
#include "logMsg/traceLevels.h"                                 // Lmt*

#include "common/globals.h"                                     // parse8601Time
#include "rest/ConnectionInfo.h"                                // ConnectionInfo

#include "orionld/common/orionldState.h"                        // orionldState
#include "orionld/common/orionldErrorResponse.h"                // orionldErrorResponseCreate
#include "orionld/common/urlCheck.h"                            // urlCheck
#include "orionld/common/urnCheck.h"                            // urnCheck
#include "orionld/context/orionldContextItemExpand.h"           // orionldContextItemExpand
#include "orionld/context/orionldContextValueExpand.h"          // orionldContextValueExpand
#include "orionld/context/orionldContextItemAlreadyExpanded.h"  // orionldContextItemAlreadyExpanded
#include "orionld/db/dbConfiguration.h"                         // dbRegistrationGet, dbRegistrationReplace
#include "orionld/payloadCheck/pcheckRegistration.h"            // pcheckRegistration
#include "orionld/serviceRoutines/orionldPatchRegistration.h"   // Own Interface



// -----------------------------------------------------------------------------
//
// kjChildAddOrReplace -
//
// FIXME: move to kjson library - defined in orionldPatchSubscription.cpp
//
extern void kjChildAddOrReplace(KjNode* container, const char* itemName, KjNode* replacementP);



// -----------------------------------------------------------------------------
//
// CORRUPTED_DB
//
#define CORRUPTED_DB(detail)                                                                      \
do                                                                                                \
{                                                                                                 \
  LM_E(("Internal Error (database corruption: %s)", detail));                                     \
  orionldErrorResponseCreate(OrionldBadRequestData, "Database seems to be corrupted", detail);    \
  orionldState.httpStatusCode = SccReceiverInternalError;                                         \
  return false;                                                                                   \
} while (0)



// -----------------------------------------------------------------------------
//
// longToInt -
//
static void longToInt(KjNode* nodeP)
{
  if (nodeP->type == KjObject)
  {
    char*     longString = nodeP->value.firstChildP->value.s;
    long long longValue  = strtol(longString, NULL, 10);

    nodeP->type    = KjInt;
    nodeP->value.i = longValue;
  }
}



// -----------------------------------------------------------------------------
//
// fixDbRegistration -
//
// As long long members are represented as "xxx": { "$numberLong": "1234565678901234" }
// and this gives an error when trying to Update this, we simply change the object to an int.
//
// In a Registration, this must be done for "expiration", and "throttling".
//
static void fixDbRegistration(KjNode* dbRegistrationP)
{
  KjNode* nodeP;

  if ((nodeP = kjLookup(dbRegistrationP, "expiration")) != NULL)
    longToInt(nodeP);
  if ((nodeP = kjLookup(dbRegistrationP, "observationInterval")) != NULL)
  {
    for (KjNode* oiItem = nodeP->value.firstChildP; oiItem != NULL; oiItem = oiItem->next)
    {
      longToInt(oiItem);
    }
  }
  if ((nodeP = kjLookup(dbRegistrationP, "managementInterval")) != NULL)
  {
    for (KjNode* oiItem = nodeP->value.firstChildP; oiItem != NULL; oiItem = oiItem->next)
    {
      longToInt(oiItem);
    }
  }
}



// -----------------------------------------------------------------------------
//
// registrationAttributeAdd -
//
static void registrationAttributeAdd(KjNode* attrsNodeP, const char* shortName, bool isProperty)
{
  KjNode* attrObjectP   = kjObject(orionldState.kjsonP, NULL);
  KjNode* attrNameNodeP = kjString(orionldState.kjsonP, "name", shortName);
  KjNode* attrTypeNodeP = kjString(orionldState.kjsonP, "type", isProperty? "Property" : "Relationship");

  kjChildAdd(attrObjectP, attrNameNodeP);
  kjChildAdd(attrObjectP, attrTypeNodeP);

  kjChildAdd(attrsNodeP, attrObjectP);
}



// -----------------------------------------------------------------------------
//
// ngsildRegistrationInformationToAPIv1Datamodel -
//
// Remove the informationP from the patchTree and insert instead a contextRegistration array with one item
//
// Actually, we need to go from this:
//
//   "information": [                              => each item in the information array becomes an item of the APIv1 contextRegistration array
//     {
//       "entities": [
//         {
//           "id": "",                             => Registration::contextRegistration::entities.id
//           "idPattern": "",                      => Registration::contextRegistration::entities.idPattern
//           "type": ""                            => Registration::contextRegistration::entities.type
//         }
//       ],
//       "properties": [],                         => Registration::contextRegistration::attrs
//       "relationships": []                       => Registration::contextRegistration::attrs
//     },
//     {}
//   ]
//
// To this:
//
//  "contextRegistration" : [
//    {
//      "entities" : [
//        {
//          "id" : "urn:ngsi-ld:Vehicle:A456",
//          "type" : "https://uri.etsi.org/ngsi-ld/default-context/Vehicle"
//        }
//      ],
//      "attrs" : [
//        {
//          "name" : "https://uri.etsi.org/ngsi-ld/default-context/brandName",
//          "type" : "Property",
//          "isDomain" : "false"
//        },
//        {
//          "name" : "https://uri.etsi.org/ngsi-ld/default-context/speed",
//          "type" : "Property",
//          "isDomain" : "false"
//        },
//        {
//          "name" : "https://uri.etsi.org/ngsi-ld/default-context/isParked",
//          "type" : "Relationship",
//          "isDomain" : "false"
//        }
//      ],
//      "providingApplication" : "http://my.csource.org:1026"
//    }
//  ],
//
// So, we can just:
// - Change the name from "information"  to  "contextRegistration"
// - Add "attrs" inside item 0 of "contextRegistration" (there is only ONE item
// - Add "providingApplication" inside item 0
// - Remove "properties" and "relationships" and populate "attrs" with the info from "properties" and "relationships"
//
// Also, if both "id" and "idPattern" is given inside "information::entities", then "information::entities::idPattern" should be set to ""
// The spec states that "id" has precedence over "idPattern".
//
// FIXME:
//   Somewhere we also need to:
//     - copy 'creDate' from the "original" DB content and
//     - add 'modDate'
//
static bool ngsildRegistrationInformationToAPIv1Datamodel(KjNode* informationP, const char* providingApplication)
{
  KjNode* infoItem0      = informationP->value.firstChildP;
  KjNode* propertiesP    = kjLookup(infoItem0, "properties");
  KjNode* relationshipsP = kjLookup(infoItem0, "relationships");
  KjNode* attrsNodeP     = kjArray(orionldState.kjsonP, "attrs");
  KjNode* paNodeP        = kjString(orionldState.kjsonP, "providingApplication", providingApplication);

  informationP->name = (char*) "contextRegistration";

  kjChildAdd(infoItem0, attrsNodeP);

  if (propertiesP)
  {
    kjChildRemove(infoItem0, propertiesP);

    for (KjNode* propertyP = propertiesP->value.firstChildP; propertyP != NULL; propertyP = propertyP->next)
      registrationAttributeAdd(attrsNodeP, propertyP->value.s, true);
  }

  if (relationshipsP)
  {
    kjChildRemove(infoItem0, relationshipsP);

    for (KjNode* relationshipP = relationshipsP->value.firstChildP; relationshipP != NULL; relationshipP = relationshipP->next)
      registrationAttributeAdd(attrsNodeP, relationshipP->value.s, false);
  }

  kjChildAdd(infoItem0, paNodeP);

  return true;
}



// -----------------------------------------------------------------------------
//
// ngsildTimeIntervalToAPIv1Datamodel -
//
void ngsildTimeIntervalToAPIv1Datamodel(KjNode* tiP)
{
  KjNode* startP = kjLookup(tiP, "start");
  KjNode* endP   = kjLookup(tiP, "end");
  int     dateTime;

  dateTime        = parse8601Time(startP->value.s);
  startP->type    = KjInt;
  startP->value.i = dateTime;

  dateTime        = parse8601Time(endP->value.s);
  endP->type      = KjInt;
  endP->value.i   = dateTime;
}



// -----------------------------------------------------------------------------
//
// ngsildExpiresToAPIv1Datamodel -
//
void ngsildExpiresToAPIv1Datamodel(KjNode* expiresP)
{
  expiresP->value.i  = parse8601Time(expiresP->value.s);
  expiresP->type     = KjInt;

  expiresP->name = (char*) "expiration";
}



// -----------------------------------------------------------------------------
//
// ngsildRegistrationToAPIv1Datamodel -
//
// Until Issue #369 is fixed, the "information" can have only ONE item.
// This is due to the fact that APIv2 only allows for ONE item and will be fixed by NOT using mongoBackend for any
// of the Registration requests.
//
// PAYLOAD of an NGSI-LD Registration
// {
//   "id": ""                                      => _id
//   "type": "Registration"                        => not part of the database
//   "name": ""                                    => just added to the APIv1 database model
//   "description": ""                             => just added to the APIv1 database model
//
//   "information": [                              => each item in the information array becomes an item of the APIv1 contextRegistration array
//     {
//       "entities": [
//         {
//           "id": "",                             => Registration::contextRegistration::entities.id
//           "idPattern": "",                      => Registration::contextRegistration::entities.idPattern
//           "type": ""                            => Registration::contextRegistration::entities.type
//         }
//       ],
//       "properties": [],                         => Registration::contextRegistration::attrs
//       "relationships": []                       => Registration::contextRegistration::attrs
//     },
//     {}
//   ],
//
//   "observationInterval": { TimeInterval },      => NEW in NGSI-LD. Simply added to the db document
//   "managementInterval": { TimeInterval },       => NEW in NGSI-LD. Simply added to the db document
//   "location": { GeoJSON },                      => NEW in NGSI-LD. Simply added to the db document
//   "observationSpace": { GeoJSON },              => NEW in NGSI-LD. Simply added to the db document
//   "operationSpace": { GeoJSON },                => NEW in NGSI-LD. Simply added to the db document
//   "expires": "",                                => "expiration"
//   "endpoint": "",                               => contextRegistration::providingApplication
//   "Property 1": {} || [] || "" || ...           => NEW in NGSI-LD. Simply added to the db document, inside the "properties" array
//   "Property 2": {} || [] || "" || ...           => NEW in NGSI-LD. Simply added to the db document, inside the "properties" array
//   ...
//   "Property N": {} || [] || "" || ...           => NEW in NGSI-LD. Simply added to the db document, inside the "properties" array
// }
//
//
// This is a Registration created as NGSI-LD and added to the DB with the APIv1 database model:
//
// {
//  "_id" : "urn:ngsi-ld:ContextSourceRegistration:R1",
//  "description" : "description of reg R1",
//  "expiration" : NumberLong(1861869600),
//  "servicePath" : "/",
//  "contextRegistration" : [
//    {
//      "entities" : [
//        {
//          "id" : "urn:ngsi-ld:Vehicle:A456",
//          "type" : "https://uri.etsi.org/ngsi-ld/default-context/Vehicle"
//        }
//      ],
//      "attrs" : [
//        {
//          "name" : "https://uri.etsi.org/ngsi-ld/default-context/brandName",
//          "type" : "Property",
//          "isDomain" : "false"
//        },
//        {
//          "name" : "https://uri.etsi.org/ngsi-ld/default-context/speed",
//          "type" : "Property",
//          "isDomain" : "false"
//        },
//        {
//          "name" : "https://uri.etsi.org/ngsi-ld/default-context/isParked",
//          "type" : "Relationship",
//          "isDomain" : "false"
//        }
//      ],
//      "providingApplication" : "http://my.csource.org:1026"
//    }
//  ],
//  "format" : "JSON",
//  "createdAt" : REGEX(.*),
//  "modifiedAt" : REGEX(.*),
//  "observationInterval" : {
//    "start" : NumberLong(1546250400),
//    "end" : NumberLong(1861869600)
//  },
//  "managementInterval" : {
//    "start" : NumberLong(1514714400),
//    "end" : NumberLong(1830247200)
//  },
//  "location" : {
//    "type" : "Polygon",
//    "coordinates" : [
//      [
//        [ 100, 0 ],
//        [ 101, 0 ],
//        [ 101, 1 ],
//        [ 100, 1 ],
//        [ 100, 0 ]
//      ]
//    ]
//  },
//  "observationSpace" : {
//    "type" : "Polygon",
//    "coordinates" : [
//      [
//        [ 200, 0 ],
//        [ 201, 0 ],
//        [ 201, 1 ],
//        [ 200, 1 ],
//        [ 200, 0 ]
//      ]
//    ]
//  },
//  "operationSpace" : {
//    "type" : "Polygon",
//    "coordinates" : [
//      [
//        [ 300, 0 ],
//        [ 301, 0 ],
//        [ 301, 1 ],
//        [ 300, 1 ],
//        [ 300, 0 ]
//      ]
//    ]
//  },
//  "properties" : {
//    "https://uri.etsi.org/ngsi-ld/default-context/P1" : 1,
//    "https://uri.etsi.org/ngsi-ld/default-context/P2" : "2",
//    "https://uri.etsi.org/ngsi-ld/default-context/P3" : true,
//    "https://uri.etsi.org/ngsi-ld/default-context/P4" : 4.5,
//    "https://uri.etsi.org/ngsi-ld/default-context/P5" : { "object" : 6 },
//    "https://uri.etsi.org/ngsi-ld/default-context/P6" : [ 7, 8 ]
//  }
// }
//
static bool ngsildRegistrationToAPIv1Datamodel(ConnectionInfo* ciP, KjNode* patchTree, KjNode* dbRegistrationP)
{
  KjNode* informationP         = NULL;
  KjNode* endpointP            = NULL;
  KjNode* observationIntervalP = NULL;
  KjNode* managementIntervalP  = NULL;
  KjNode* expiresP             = NULL;

  //
  // Loop over the patch-tree and modify it to make it compatible with the database model for APIv1
  //
  for (KjNode* fragmentP = patchTree->value.firstChildP; fragmentP != NULL; fragmentP = fragmentP->next)
  {
    if (strcmp(fragmentP->name, "id") == 0)
      fragmentP->name = (char*) "_id";
    else if (strcmp(fragmentP->name, "type") == 0)
    {
      // Just skip it - don't want "type: ContextSourceRegistration" in the DB. Not needed
    }
    else if (strcmp(fragmentP->name, "information") == 0)
      informationP = fragmentP;
    else if (strcmp(fragmentP->name, "expires") == 0)
      expiresP = fragmentP;
    else if (strcmp(fragmentP->name, "endpoint") == 0)
      endpointP = fragmentP;
    else if (strcmp(fragmentP->name, "observationInterval") == 0)
      observationIntervalP = fragmentP;
    else if (strcmp(fragmentP->name, "managementInterval") == 0)
      managementIntervalP = fragmentP;
  }


  //
  // If "endpoint" was given, the providingApplication in the contextRegistration array item needs to change
  //
  KjNode* providingApplicationP = NULL;


  //
  // If either 'endpoint' or 'information' is to be patched, then we need a pointer to
  // contextRegistration[0]::providingApplication.
  //
  // If 'endpoint'    - to modify the value of contextRegistration[0]::providingApplication.
  // If 'information' - to fill in the new contextRegistration[0] with the value of contextRegistration[0]::providingApplication.
  //
  if ((endpointP != NULL) || (informationP != NULL))
  {
    // Take the providing application / endpoint from what's already in the DB
    KjNode* contextRegistrationArrayP  = kjLookup(dbRegistrationP, "contextRegistration");
    if (contextRegistrationArrayP == NULL)
      CORRUPTED_DB("Registration field 'contextRegistration' is missing");
    else if (contextRegistrationArrayP->type != KjArray)
      CORRUPTED_DB("Registration field 'contextRegistration' is not a JSON Array");

    KjNode* contextRegistration0 = contextRegistrationArrayP->value.firstChildP;
    if (contextRegistration0 == NULL)
      CORRUPTED_DB("Registration field 'contextRegistration' is an empty array");
    if (contextRegistration0->type != KjObject)
      CORRUPTED_DB("Registration field 'contextRegistration[0]' is not a JSON Object");

    providingApplicationP = kjLookup(contextRegistration0, "providingApplication");
    if (providingApplicationP == NULL)
      CORRUPTED_DB("Mandatory Registration field 'contextRegistration[0]::providingApplication' is missing");

    if (providingApplicationP->type != KjString)
       CORRUPTED_DB("Registration field 'contextRegistration[0]::providingApplication' must be a JSON String");
  }

  if (endpointP != NULL)
    providingApplicationP->value.s = endpointP->value.s;

  if (informationP != NULL)
    ngsildRegistrationInformationToAPIv1Datamodel(informationP, providingApplicationP->value.s);

  //
  // The 'start' and 'end' of observationInterval and managementInterval come in as strings
  // and must be converted into integers
  //
  if (observationIntervalP != NULL)
    ngsildTimeIntervalToAPIv1Datamodel(observationIntervalP);
  if (managementIntervalP != NULL)
    ngsildTimeIntervalToAPIv1Datamodel(managementIntervalP);
  if (expiresP != NULL)
    ngsildExpiresToAPIv1Datamodel(expiresP);

  return true;
}



// ----------------------------------------------------------------------------
//
// ngsildRegistrationPatch -
//
static void ngsildRegistrationPatch(KjNode* dbRegistrationP, KjNode* patchTree)
{
  KjNode* fragmentP = patchTree->value.firstChildP;
  KjNode* next;

  while (fragmentP != NULL)
  {
    next = fragmentP->next;

    if (fragmentP->type == KjNull)
    {
      KjNode* toRemove = kjLookup(dbRegistrationP, fragmentP->name);

      if (toRemove != NULL)
      {
        kjChildRemove(dbRegistrationP, toRemove);

        //
        // Dangerous to remove without checking ... what if we remove "endpoint" ... ???
        //
      }
    }
    else
      kjChildAddOrReplace(dbRegistrationP, fragmentP->name, fragmentP);

    fragmentP = next;
  }
}



// ----------------------------------------------------------------------------
//
// orionldPatchRegistration -
//
bool orionldPatchRegistration(ConnectionInfo* ciP)
{
  char*    registrationId = orionldState.wildcard[0];
  KjNode*  propertyTree;

  if ((urlCheck(registrationId, NULL) == false) && (urnCheck(registrationId, NULL) == false))
  {
    orionldState.httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(OrionldBadRequestData, "Registration ID must be a valid URI", registrationId);
    return false;
  }

  if (pcheckRegistration(orionldState.requestTree, false, &propertyTree) == false)
  {
    LM_E(("pcheckRegistration FAILED"));
    return false;
  }

  KjNode* dbRegistrationP = dbRegistrationGet(registrationId);

  if (dbRegistrationP == NULL)
  {
    orionldState.httpStatusCode = SccNotFound;
    orionldErrorResponseCreate(OrionldBadRequestData, "Registration not found", registrationId);
    return false;
  }

  //
  // Now that we have the "original" registration, we can check that all registration properties
  // from the PATCH actually exist in the original registration
  //
  KjNode* dbPropertiesP = kjLookup(dbRegistrationP, "properties");

  if ((dbPropertiesP == NULL) || (dbPropertiesP->value.firstChildP == NULL))
  {
    //
    // THERE ARE NO registration properties - so no reg property can be patched - I don't like this ...
    // Why can't I add registration properties ?
    //
    if (propertyTree->value.firstChildP != NULL)
    {
      orionldState.httpStatusCode = SccBadRequest;
      orionldErrorResponseCreate(OrionldBadRequestData, "non-existing registration property", propertyTree->value.firstChildP->name);
      return false;
    }
  }

  //
  // Now we check all reg properties of the patch, make sure they already exist
  // We also need to expand the names of the properties, and perhaps also their values.
  // And, finally - replace the old "DB registration property" with the "Patch registration property".
  //
  KjNode* propertyP = propertyTree->value.firstChildP;
  KjNode* next;

  while (propertyP != NULL)
  {
    bool    valueMayBeExpanded;
    KjNode* dbPropertyP;

    next = propertyP->next;

    if (orionldContextItemAlreadyExpanded(propertyP->name) == false)
      propertyP->name = orionldContextItemExpand(orionldState.contextP, propertyP->name, &valueMayBeExpanded, true, NULL);

    if ((dbPropertyP = kjLookup(dbPropertiesP, propertyP->name)) == NULL)
    {
      orionldState.httpStatusCode = SccBadRequest;
      orionldErrorResponseCreate(OrionldBadRequestData, "non-existing registration property", propertyP->name);
      return false;
    }

    if (valueMayBeExpanded)
      orionldContextValueExpand(propertyP);

    //
    // Replace the registration properties
    //
    kjChildRemove(dbPropertiesP, dbPropertyP);
    kjChildAdd(dbPropertiesP, propertyP);

    propertyP = next;
  }

  //
  // Remove Occurrences of $numberLong, i.e. "expiration"
  //
  // FIXME: This is BAD ... shouldn't change the type of these fields
  //
  fixDbRegistration(dbRegistrationP);

  // Convert the patch-payload to valid APIv1 Database model
  ngsildRegistrationToAPIv1Datamodel(ciP, orionldState.requestTree, dbRegistrationP);

  // Now we're ready to merge to patch into what's already there ...
  ngsildRegistrationPatch(dbRegistrationP, orionldState.requestTree);

  //
  // Overwrite the current Registration in the database
  //
  dbRegistrationReplace(registrationId, dbRegistrationP);

  // All OK? 204
  orionldState.httpStatusCode = SccNoContent;

  return true;
}
