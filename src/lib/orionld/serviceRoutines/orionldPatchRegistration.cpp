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
#include "orionld/common/geoJsonCheck.h"                       // geoJsonCheck
#include "orionld/context/orionldContextItemExpand.h"          // orionldContextItemExpand
#include "orionld/db/dbConfiguration.h"                        // dbRegistrationGet, dbRegistrationReplace
#include "orionld/serviceRoutines/orionldPatchRegistration.h"  // Own Interface



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
  ciP->httpStatusCode = SccReceiverInternalError;                                                 \
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
// As long long members are respresented as "xxx": { "$numberLong": "1234565678901234" }
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
  char*   longName      = orionldContextItemExpand(orionldState.contextP, shortName, NULL, true, NULL);
  KjNode* attrObjectP   = kjObject(orionldState.kjsonP, NULL);
  KjNode* attrNameNodeP = kjString(orionldState.kjsonP, "name", longName);
  KjNode* attrTypeNodeP = kjString(orionldState.kjsonP, "type", isProperty? "Property" : "Relationship");

  kjChildAdd(attrObjectP, attrNameNodeP);
  kjChildAdd(attrObjectP, attrTypeNodeP);

  kjChildAdd(attrsNodeP, attrObjectP);

  LM_TMP(("RPAT: Added %s %s ('%s')", isProperty? "Property" : "Relationship", shortName, longName));
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

  informationP->name = (char*) "contextRegistration";

  kjChildAdd(infoItem0, attrsNodeP);


  if (propertiesP)
  {
    kjChildRemove(infoItem0, propertiesP);

    LM_TMP(("RPAT: Adding properties"));
    for (KjNode* propertyP = propertiesP->value.firstChildP; propertyP >= NULL; propertyP = propertyP->next)
      registrationAttributeAdd(attrsNodeP, propertyP->name, true);
  }

  if (relationshipsP)
  {
    kjChildRemove(infoItem0, relationshipsP);

    LM_TMP(("RPAT: Adding relationships"));
    for (KjNode* relationshipP = relationshipsP->value.firstChildP; relationshipP >= NULL; relationshipP = relationshipP->next)
      registrationAttributeAdd(attrsNodeP, relationshipP->name, false);
  }

  return true;
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
  KjNode* informationP = NULL;
  KjNode* endpointP    = NULL;
  KjNode* modDateP     = NULL;
  KjNode* creDateP     = NULL;

  //
  // Loop over the patch-tree and modify to make it compatible with the database model for APIv1
  //
  LM_TMP(("RPAT: In ngsildRegistrationToAPIv1Datamodel"));
  for (KjNode* fragmentP = patchTree->value.firstChildP; fragmentP != NULL; fragmentP = fragmentP->next)
  {
    LM_TMP(("RPAT: treating field '%s' of the patch-tree", fragmentP->name));
    if (strcmp(fragmentP->name, "id") == 0)
      fragmentP->name = (char*) "_id";
    else if (strcmp(fragmentP->name, "type") == 0)
    {
      // Just skip it - don't want "type: ContextSourceRegistration" in the DB. Not needed
    }
    else if (strcmp(fragmentP->name, "information") == 0)
      informationP = fragmentP;
    else if (strcmp(fragmentP->name, "expires") == 0)
      fragmentP->name = (char*) "expiration";
    else if (strcmp(fragmentP->name, "endpoint") == 0)
      endpointP = fragmentP;
    else if (strcmp(fragmentP->name, "modDate") == 0)
      modDateP = fragmentP;
    else if (strcmp(fragmentP->name, "creDate") == 0)
      creDateP = fragmentP;
  }


  //
  // If "endpoint" was given, the providingApplication in the contextRegistration array item needs to change
  //
  KjNode* providingApplicationP = NULL;

  LM_TMP(("RPAT: endpointP at %p", endpointP));
  LM_TMP(("RPAT: informationP at %p", informationP));

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

  LM_TMP(("RPAT: informationP at %p", informationP));
  if (informationP != NULL)
  {
    LM_TMP(("RPAT: Calling ngsildRegistrationInformationToAPIv1Datamodel"));
    ngsildRegistrationInformationToAPIv1Datamodel(informationP, providingApplicationP->value.s);
  }

  if (modDateP != NULL)
  {
    LM_TMP(("RPAT: modDateP exists - must modify it to 'right now'"));
  }
  else
  {
    LM_TMP(("RPAT: modDateP doesn't exist - must add it with value 'right now'"));
  }

  if (creDateP == NULL)
  {
    LM_TMP(("RPAT: creDateP doesn't exist - CORRUPTED DB"));
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// orionldCheckGeoJsonGeometry -
//
static bool orionldCheckGeoJsonGeometry(ConnectionInfo* ciP, KjNode* geoAttributeP, const char* fieldName)
{
  return geoJsonCheck(ciP, geoAttributeP, NULL, NULL);
}



// -----------------------------------------------------------------------------
//
// orionldCheckRegistrationInformationEntity -
//
// An item of the "entities" array is a JSON Object and can have three different mebmers:
//   * id             (Optional JSON String that is a valid URI)
//   * idPattern      (Optional JSON String that is a valid REGEX)
//   * type           (Mandatory JSON String)
//
static bool orionldCheckRegistrationInformationEntity(ConnectionInfo* ciP, KjNode* entityP)
{
  KjNode* idP         = NULL;
  KjNode* idPatternP  = NULL;
  KjNode* typeP       = NULL;

  for (KjNode* entityItemP = entityP->value.firstChildP; entityItemP != NULL; entityItemP = entityItemP->next)
  {
    if (strcmp(entityItemP->name, "id") == 0)
    {
      DUPLICATE_CHECK(idP, "Registration::information[X]::entities[X]::id", entityItemP);
      STRING_CHECK(entityItemP, "Registration::information[X]::entities[X]::id");
      URI_CHECK(entityItemP, "Registration::information[X]::entities[X]::id");
    }
    else if (strcmp(entityItemP->name, "idPattern") == 0)
    {
      DUPLICATE_CHECK(idPatternP, "Registration::information[X]::entities[X]::idPattern", entityItemP);
      STRING_CHECK(entityItemP, "Registration::information[X]::entities[X]::idPattern");
      EMPTY_STRING_CHECK(entityItemP, "Registration::information[X]::entities[X]::idPattern");
      // FIXME: How check for valid REGEX???
    }
    else if (strcmp(entityItemP->name, "type") == 0)
    {
      DUPLICATE_CHECK(typeP, "Registration::information[X]::entities[X]::type", entityItemP);
      STRING_CHECK(entityItemP, "Registration::information[X]::entities[X]::type");
      EMPTY_STRING_CHECK(entityItemP, "Registration::information[X]::entities[X]::id");
    }
    else
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid field for Registration::information[X]::entities[X]", entityItemP->name);
      ciP->httpStatusCode = SccBadRequest;
      return false;
    }
  }

  // Only if Fully NGSI-LD compliant
  if (typeP == NULL)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Missing mandatory field", "Registration::information[X]::entities[X]::type");
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// orionldCheckRegistrationInformation
//
// The "information" field can have only three members:
//   * entities       (Mandatory JSON Array)
//   * properties     (Optional JSON Array with strings)
//   * relationships  (Optional JSON Array with strings)
//
static bool orionldCheckRegistrationInformation(ConnectionInfo* ciP, KjNode* informationP)
{
  for (KjNode* infoItemP = informationP->value.firstChildP; infoItemP != NULL; infoItemP = infoItemP->next)
  {
    if (strcmp(infoItemP->name, "entities") == 0)
    {
      ARRAY_CHECK(infoItemP, "Registration::information[X]::entities");

      if (infoItemP->value.firstChildP == NULL)  // Empty Array
      {
        LM_E(("Registration::information[X]::entities is an empty array"));
        orionldErrorResponseCreate(OrionldBadRequestData, "Empty Array", "Registration::information[X]::entities");
        ciP->httpStatusCode = SccBadRequest;
        return false;
      }
      LM_TMP(("RC: Registration::information[X]::entities firstChild at: 0x%x", infoItemP->value.firstChildP));

      for (KjNode* entityP = infoItemP->value.firstChildP; entityP != NULL; entityP = entityP->next)
      {
        OBJECT_CHECK(entityP, "Registration::information[X]::entities[X]");
        if (orionldCheckRegistrationInformationEntity(ciP, entityP) == false)
          return false;
      }
    }
    else if (strcmp(infoItemP->name, "properties") == 0)
    {
      ARRAY_CHECK(infoItemP, "Registration::information[X]::properties");
      EMPTY_ARRAY_CHECK(infoItemP, "Registration::information[X]::properties");
      for (KjNode* propP = infoItemP->value.firstChildP; propP != NULL; propP = propP->next)
      {
        STRING_CHECK(propP, "Registration::information[X]::properties[X]");
        EMPTY_STRING_CHECK(propP, "Registration::information[X]::properties[X]");
      }
    }
    else if (strcmp(infoItemP->name, "relationships") == 0)
    {
      ARRAY_CHECK(infoItemP, "Registration::information[X]::relationships");
      EMPTY_ARRAY_CHECK(infoItemP, "Registration::information[X]::relationships");
      for (KjNode* relP = infoItemP->value.firstChildP; relP != NULL; relP = relP->next)
      {
        STRING_CHECK(relP, "Registration::information[X]::relationships[X]");
        EMPTY_STRING_CHECK(relP, "Registration::information[X]::relationships[X]");
        URI_CHECK(relP, "Registration::information[X]::relationships[X]");
      }
    }
    else
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid field for Registration::information[X]", infoItemP->name);
      ciP->httpStatusCode = SccBadRequest;
      return false;
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// orionldCheckRegistrationInformationArray
//
static bool orionldCheckRegistrationInformationArray(ConnectionInfo* ciP, KjNode* informationArrayP, const char* fieldName)
{
  if (informationArrayP->value.firstChildP == NULL)  // Empty Array
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Empty Array", "Registration::information");
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  for (KjNode* informationP = informationArrayP->value.firstChildP; informationArrayP != NULL; informationArrayP = informationArrayP->next)
  {
    OBJECT_CHECK(informationP, "Registration::information[X]");

    if (orionldCheckRegistrationInformation(ciP, informationP) == false)
      return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// orionldCheckObservationInterval -
//
static bool orionldCheckObservationInterval(ConnectionInfo* ciP, KjNode* timeIntervalNodeP, const char* fieldName)
{
  KjNode* startP = NULL;
  KjNode* endP   = NULL;
  int64_t start  = 0;
  int64_t end    = 0;

  LM_TMP(("OI: examining an ObservationInterval named '%s'", timeIntervalNodeP));
  for (KjNode* tiItemP = timeIntervalNodeP->value.firstChildP; tiItemP != NULL; tiItemP = tiItemP->next)
  {
    LM_TMP(("OI: examining a child of the ObservationInterval named '%s'", tiItemP->name));
    if (strcmp(tiItemP->name, "start") == 0)
    {
      DUPLICATE_CHECK(startP, "Registration::observationInterval::start", tiItemP);
      STRING_CHECK(tiItemP, "Registration::observationInterval::start");
      DATETIME_CHECK(tiItemP->value.s, start, "Registration::observationInterval::start");
    }
    else if (strcmp(tiItemP->name, "end") == 0)
    {
      DUPLICATE_CHECK(endP, "Registration::observationInterval::end", tiItemP);
      STRING_CHECK(tiItemP, "Registration::observationInterval::end");
      DATETIME_CHECK(tiItemP->value.s, end, "Registration::observationInterval::end");
    }
    else
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid field for Registration::observationInterval", tiItemP->name);
      ciP->httpStatusCode = SccBadRequest;
      return false;
    }
  }

  if ((startP == NULL) && (endP == NULL))
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Empty Object", "Registration::observationInterval");
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  if (startP == NULL)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Missing mandatory field", "Registration::observationInterval::start");
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  if (endP == NULL)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Missing mandatory field", "Registration::observationInterval::end");
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  if (start > end)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Inconsistent DateTimes", "Registration::observationInterval ends before it starts");
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// orionldCheckManagementInterval -
//
static bool orionldCheckManagementInterval(ConnectionInfo* ciP, KjNode* timeIntervalNodeP, const char* fieldName)
{
  KjNode* startP = NULL;
  KjNode* endP   = NULL;
  int64_t start  = 0;
  int64_t end    = 0;

  for (KjNode* tiItemP = timeIntervalNodeP->value.firstChildP; tiItemP != NULL; tiItemP = tiItemP->next)
  {
    if (strcmp(tiItemP->name, "start") == 0)
    {
      DUPLICATE_CHECK(startP, "Registration::managementInterval::start", tiItemP);
      STRING_CHECK(tiItemP, "Registration::managementInterval::start");
      DATETIME_CHECK(tiItemP->value.s, start, "Registration::managementInterval::start");
    }
    else if (strcmp(tiItemP->name, "end") == 0)
    {
      DUPLICATE_CHECK(endP, "Registration::managementInterval::end", tiItemP);
      STRING_CHECK(tiItemP, "Registration::managementInterval::end");
      DATETIME_CHECK(tiItemP->value.s, end, "Registration::managementInterval::end");
    }
    else
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid field for Registration::managementInterval", tiItemP->name);
      ciP->httpStatusCode = SccBadRequest;
      return false;
    }
  }

  if ((startP == NULL) && (endP == NULL))
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Empty Object", "Registration::managementInterval");
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  if (startP == NULL)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Missing mandatory field", "Registration::managementInterval::start");
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  if (endP == NULL)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Missing mandatory field", "Registration::managementInterval::end");
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  if (start > end)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Inconsistent DateTimes", "Registration::managementInterval ends before it starts");
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// registrationPayloadCheck -
//
static bool registrationPayloadCheck(ConnectionInfo* ciP, KjNode* registrationP, bool idCanBePresent)
{
  KjNode*  idP                   = NULL;
  KjNode*  typeP                 = NULL;
  KjNode*  nameP                 = NULL;
  KjNode*  descriptionP          = NULL;
  KjNode*  informationP          = NULL;
  KjNode*  observationIntervalP  = NULL;
  KjNode*  managementIntervalP   = NULL;
  KjNode*  locationP             = NULL;
  KjNode*  observationSpaceP     = NULL;
  KjNode*  operationSpaceP       = NULL;
  KjNode*  expiresP              = NULL;
  KjNode*  endpointP             = NULL;
  KjNode*  propertiesTree        = kjObject(orionldState.kjsonP, NULL);  // Temp storage for properties
  int64_t  dateTime;

  if (registrationP->type != KjObject)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Registration", "The payload data for updating a registration must be a JSON Object");
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }


  //
  // Loop over the entire registration (incoming payload data) and check all is OK
  //
  // To be able to easy and fast check that no registration properties are duplicated, the registration properties are
  // removed from the tree and added to 'propertiesTree'.
  // Later, these registration properties must be put back!
  //
  // It's quicker to implement this like that, as there's no need to go over the entire path tree and also not necessary to look for >1 matches.
  // When Property P1 is about to be added to 'propertiesTree' for a second time, the duplication will be noticed and the error triggered.
  //
  KjNode* nodeP = registrationP->value.firstChildP;
  KjNode* next;

  while (nodeP != NULL)
  {
    next = nodeP->next;

    if (strcmp(nodeP->name, "id") == 0)
    {
      if (idCanBePresent == false)
      {
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid field for Registration Update", "Registration::id");
        ciP->httpStatusCode = SccBadRequest;
        return false;
      }

      DUPLICATE_CHECK(idP, "Registration::id", nodeP);
      STRING_CHECK(nodeP, "Registration::id");
      URI_CHECK(nodeP, "Registration::id");
    }
    else if (strcmp(nodeP->name, "type") == 0)
    {
      DUPLICATE_CHECK(typeP, "Registration::type", nodeP);
      STRING_CHECK(nodeP, "Registration::type");

      if (strcmp(nodeP->value.s, "ContextSourceRegistration") != 0)
      {
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid value for Registration::type", nodeP->value.s);
        ciP->httpStatusCode = SccBadRequest;
        return false;
      }
    }
    else if (strcmp(nodeP->name, "name") == 0)
    {
      DUPLICATE_CHECK(nameP, "Registration::name", nodeP);
      STRING_CHECK(nodeP, "Registration::name");
    }
    else if (strcmp(nodeP->name, "description") == 0)
    {
      DUPLICATE_CHECK(descriptionP, "Registration::description", nodeP);
      STRING_CHECK(nodeP, "Registration::description");
    }
    else if (strcmp(nodeP->name, "information") == 0)
    {
      DUPLICATE_CHECK(informationP, "Registration::information", nodeP);
      ARRAY_CHECK(nodeP, "Registration::information");
      if (orionldCheckRegistrationInformationArray(ciP, nodeP, "Registration::information") == false)
        return false;
    }
    else if (strcmp(nodeP->name, "observationInterval") == 0)
    {
      DUPLICATE_CHECK(observationIntervalP, "Registration::observationInterval", nodeP);
      OBJECT_CHECK(nodeP, "Registration::observationInterval");
      EMPTY_OBJECT_CHECK(nodeP, "Registration::observationInterval");
      if (orionldCheckObservationInterval(ciP, nodeP, "Registration::observationInterval") == false)
        return false;
    }
    else if (strcmp(nodeP->name, "managementInterval") == 0)
    {
      DUPLICATE_CHECK(managementIntervalP, "Registration::managementInterval", nodeP);
      OBJECT_CHECK(nodeP, "Registration::managementInterval");
      if (orionldCheckManagementInterval(ciP, nodeP, "Registration::managementInterval") == false)
        return false;
    }
    else if (strcmp(nodeP->name, "location") == 0)
    {
      LM_TMP(("MLS: Got a 'location' field at %p", nodeP));
      DUPLICATE_CHECK(locationP, "Registration::location", nodeP);
      OBJECT_CHECK(nodeP, "Registration::location");
      LM_TMP(("MLS: Calling orionldCheckGeoJsonGeometry"));
      if (orionldCheckGeoJsonGeometry(ciP, locationP, "Registration::location") == false)
        return false;
    }
    else if (strcmp(nodeP->name, "observationSpace") == 0)
    {
      DUPLICATE_CHECK(observationSpaceP, "Registration::observationSpace", nodeP);
      OBJECT_CHECK(nodeP, "Registration::observationSpace");
      if (orionldCheckGeoJsonGeometry(ciP, nodeP, "Registration::observationSpace") == false)
        return false;
    }
    else if (strcmp(nodeP->name, "operationSpace") == 0)
    {
      DUPLICATE_CHECK(operationSpaceP, "Registration::operationSpace", nodeP);
      OBJECT_CHECK(nodeP, "Registration::operationSpace");
      if (orionldCheckGeoJsonGeometry(ciP, nodeP, "Registration::operationSpace") == false)
        return false;
    }
    else if (strcmp(nodeP->name, "expires") == 0)
    {
      DUPLICATE_CHECK(expiresP, "Registration::expires", nodeP);
      STRING_CHECK(nodeP, "Registration::expires");
      DATETIME_CHECK(expiresP->value.s, dateTime, "Registration::expires");
    }
    else if (strcmp(nodeP->name, "endpoint") == 0)
    {
      DUPLICATE_CHECK(endpointP, "Registration::endpoint", nodeP);
      STRING_CHECK(nodeP, "Registration::endpoint");
      URI_CHECK(nodeP, nodeP->name);
    }
    else
    {
      // This is a Property (with keyValues) - can be anything
      kjChildRemove(registrationP, nodeP);

      //
      // Add the Property to 'propertiesTree'
      //
      // But, before adding, look it up.
      // If already there, then we have a case of duplicate property and that is an error
      //
      if (kjLookup(propertiesTree, nodeP->name) != NULL)
      {
        ciP->httpStatusCode = SccBadRequest;
        orionldErrorResponseCreate(OrionldBadRequestData, "Duplicate Property in Registration Update", nodeP->name);
        return false;
      }

      kjChildAdd(propertiesTree, nodeP);
    }

    nodeP = next;
  }

  //
  // Before we can return, we need to put all the properties from 'propertiesTree' back into the 'registrationP'.
  // This is REALLY FAST !!!
  // We just make the "next pointer" of the last child of 'registrationP' point to the first chile of 'propertiesTree'.
  //
  if (propertiesTree->value.firstChildP != NULL)
  {
    registrationP->lastChild->next = propertiesTree->value.firstChildP;
    registrationP->lastChild       = propertiesTree->lastChild;
  }

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

    LM_TMP(("RPAT: Patching fragment '%s'", fragmentP->name));
    if (fragmentP->type == KjNull)
    {
      KjNode* toRemove = kjLookup(dbRegistrationP, fragmentP->name);

      if (toRemove != NULL)
      {
        LM_TMP(("RPAT: Calling kjChildRemove for '%s'", fragmentP->name));
        kjChildRemove(dbRegistrationP, toRemove);

        //
        // Dangerous to remove without checking ... what if we remove "Registration::endpoint" ... ???
        //
      }
      else
        LM_TMP(("RPAT: Can't remove '%s' - it's not present in the DB", fragmentP->name));
    }
    else
    {
      LM_TMP(("RPAT: Calling kjChildAddOrReplace for '%s'", fragmentP->name));
      kjChildAddOrReplace(dbRegistrationP, fragmentP->name, fragmentP);
    }

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

  LM_TMP(("RPAT: orionldPatchRegistration: registrationId == '%s'", registrationId));

  if ((urlCheck(registrationId, NULL) == false) && (urnCheck(registrationId, NULL) == false))
  {
    ciP->httpStatusCode = SccBadRequest;
    orionldErrorResponseCreate(OrionldBadRequestData, "Registration ID must be a valid URI", registrationId);
    return false;
  }

  LM_TMP(("RPAT: Calling registrationPayloadCheck"));
  if (registrationPayloadCheck(ciP, orionldState.requestTree, false) == false)
  {
    LM_E(("registrationPayloadCheck FAILED"));
    return false;
  }

  LM_TMP(("RPAT: Calling dbRegistrationGet"));
  KjNode* dbRegistrationP = dbRegistrationGet(registrationId);

  if (dbRegistrationP == NULL)
  {
    ciP->httpStatusCode = SccNotFound;
    orionldErrorResponseCreate(OrionldBadRequestData, "Registration not found", registrationId);
    return false;
  }

  // <DEBUG>
  char buffer[1024];
  kjRender(orionldState.kjsonP, dbRegistrationP, buffer, sizeof(buffer));
  LM_TMP(("RPAT: -------------------------------------------------------------------"));
  LM_TMP(("RPAT: DB tree: '%s'", buffer));
  LM_TMP(("RPAT: -------------------------------------------------------------------"));
  kjRender(orionldState.kjsonP, orionldState.requestTree, buffer, sizeof(buffer));
  LM_TMP(("RPAT: patch: '%s'", buffer));
  LM_TMP(("RPAT: -------------------------------------------------------------------"));
  // </DEBUG>

  //
  // Remove Occurrences of $numberLong, i.e. "expiration"
  //
  // FIXME: This is BAD ... shouldn't change the type of these fields
  //
  fixDbRegistration(dbRegistrationP);

  ngsildRegistrationToAPIv1Datamodel(ciP, orionldState.requestTree, dbRegistrationP);
  kjRender(orionldState.kjsonP, orionldState.requestTree, buffer, sizeof(buffer));
  LM_TMP(("RPAT: Modified patch-tree: '%s'", buffer));

  ngsildRegistrationPatch(dbRegistrationP, orionldState.requestTree);
  kjRender(orionldState.kjsonP, dbRegistrationP, buffer, sizeof(buffer));
  LM_TMP(("RPAT: New tree to replace what is in the DB: '%s'", buffer));

  //
  // Overwrite the current Registration in the database
  //
  dbRegistrationReplace(registrationId, dbRegistrationP);

  // All OK? 204
  ciP->httpStatusCode = SccNoContent;

  return true;
}
