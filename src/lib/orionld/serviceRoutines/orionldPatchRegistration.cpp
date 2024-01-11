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
#include <strings.h>                                           // bzero

extern "C"
{
#include "kalloc/kaStrdup.h"                                   // kaStrdup
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjChildRemove, ...
#include "kjson/kjClone.h"                                     // kjClone
#include "kjson/kjFree.h"                                      // kjFree
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/types/RegistrationMode.h"                    // registrationMode
#include "orionld/types/RegCacheItem.h"                        // RegCacheItem
#include "orionld/types/OrionLdRestService.h"                  // OrionLdRestService
#include "orionld/types/DistOpType.h"                          // distOpTypeMask
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/common/CHECK.h"                              // STRING_CHECK, ...
#include "orionld/common/tenantList.h"                         // tenant0
#include "orionld/common/dateTime.h"                           // dateTimeFromString
#include "orionld/payloadCheck/PCHECK.h"                       // PCHECK_URI
#include "orionld/payloadCheck/pcheckRegistration.h"           // pcheckRegistration
#include "orionld/legacyDriver/legacyPatchRegistration.h"      // legacyPatchRegistration
#include "orionld/regCache/regCacheItemLookup.h"               // regCacheItemLookup
#include "orionld/regCache/regCacheIdPatternRegexCompile.h"    // regCacheIdPatternRegexCompile
#include "orionld/regCache/regCacheItemRegexRelease.h"         // regCacheItemRegexRelease
#include "orionld/regCache/regCachePresent.h"                  // regCachePresent
#include "orionld/dbModel/dbModelFromApiRegistration.h"        // dbModelFromApiRegistration
#include "orionld/dbModel/dbModelToApiRegistration.h"          // dbModelToApiRegistration
#include "orionld/mongoc/mongocRegistrationGet.h"              // mongocRegistrationGet
#include "orionld/mongoc/mongocRegistrationReplace.h"          // mongocRegistrationReplace
#include "orionld/serviceRoutines/orionldPatchRegistration.h"  // Own Interface



// -----------------------------------------------------------------------------
//
// FIXME: to its own module (in dbModel lib?)
//
extern void apiModelToCacheRegistration(KjNode* apiRegistrationP);



// -----------------------------------------------------------------------------
//
// kjStringPatch -
//
static void kjStringPatch(KjNode* container, const char* fieldName, char* stringValue)
{
  KjNode* nodeP    = kjLookup(container, fieldName);
  bool    deletion = (strcmp(stringValue, "urn:ngsi-ld:null") == 0);

  if (nodeP != NULL)
  {
    if (deletion == false)
    {
      nodeP->type    = KjString;
      nodeP->value.s = kaStrdup(&orionldState.kalloc, stringValue);
    }
    else
      kjChildRemove(container, nodeP);
  }
  else
  {
    if (deletion == false)
    {
      nodeP = kjString(orionldState.kjsonP, fieldName, stringValue);
      kjChildAdd(container, nodeP);
    }
  }
}



// -----------------------------------------------------------------------------
//
// kjIntegerPatch -
//
static void kjIntegerPatch(KjNode* container, const char* fieldName, long long iValue)
{
  KjNode* nodeP = kjLookup(container, fieldName);

  if (nodeP != NULL)
  {
    nodeP->type    = KjInt;
    nodeP->value.i = iValue;
  }
  else
  {
    nodeP = kjInteger(orionldState.kjsonP, fieldName, iValue);
    kjChildAdd(container, nodeP);
  }
}



// -----------------------------------------------------------------------------
//
// kjFloatPatch -
//
static void kjFloatPatch(KjNode* container, const char* fieldName, double dValue)
{
  KjNode* nodeP = kjLookup(container, fieldName);

  if (nodeP != NULL)
  {
    nodeP->type    = KjFloat;
    nodeP->value.f = dValue;
  }
  else
  {
    nodeP = kjFloat(orionldState.kjsonP, fieldName, dValue);
    kjChildAdd(container, nodeP);
  }
}



// -----------------------------------------------------------------------------
//
// kjBooleanPatch -
//
static void kjBooleanPatch(KjNode* container, const char* fieldName, bool bValue)
{
  KjNode* nodeP = kjLookup(container, fieldName);

  if (nodeP != NULL)
  {
    nodeP->type    = KjBoolean;
    nodeP->value.b = bValue;
  }
  else
  {
    nodeP = kjBoolean(orionldState.kjsonP, fieldName, bValue);
    kjChildAdd(container, nodeP);
  }
}



// -----------------------------------------------------------------------------
//
// kjCompoundPatch -
//
static void kjCompoundPatch(KjNode* container, const char* fieldName, KjNode* rhs)
{
  KjNode* nodeP = kjLookup(container, fieldName);
  KjNode* clone = kjClone(orionldState.kjsonP, rhs);

  if (nodeP != NULL)
  {
    nodeP->value     = clone->value;
    nodeP->type      = clone->type;
    nodeP->lastChild = clone->lastChild;
  }
  else
  {
    clone->name = (char*) fieldName;
    kjChildAdd(container, clone);
  }
}



// -----------------------------------------------------------------------------
//
// kjDateTimePatch -
//
static void kjDateTimePatch(KjNode* container, const char* fieldName, char* dateTime)
{
  char    errorString[256];
  KjNode* nodeP  = kjLookup(container, fieldName);
  double  dValue = dateTimeFromString(dateTime, errorString, sizeof(errorString));

  if (dValue < 0)
    LM_E(("dateTimeFromString: %s", errorString));
  else if (nodeP != NULL)
  {
    nodeP->type    = KjFloat;
    nodeP->value.f = dValue;
  }
  else
  {
    nodeP = kjFloat(orionldState.kjsonP, fieldName, dValue);
    kjChildAdd(container, nodeP);
  }
}



// -----------------------------------------------------------------------------
//
// kjTimeIntervalPatch -
//
static void kjTimeIntervalPatch(KjNode* container, const char* fieldName, KjNode* newIntervalP)
{
  KjNode* intervalP  = kjLookup(container, fieldName);
  KjNode* newStartAt = kjLookup(newIntervalP, "startAt");
  KjNode* newEndAt   = kjLookup(newIntervalP, "endAt");

  if (intervalP == NULL)
  {
    if ((newStartAt == NULL) || (newEndAt == NULL))
    {
      LM_W(("The TimeInterval '%s' did not previously exist and one of startAt/endAt is missing", fieldName));

      //
      // For now, add it the one that is missing (copy of the other one)
      // That's ... an interval that starts and ends at the same time ...
      //
      if (newStartAt == NULL)
        newStartAt = newEndAt;
      if (newEndAt == NULL)
        newEndAt = newStartAt;
    }

    intervalP = kjObject(orionldState.kjsonP, fieldName);
    kjChildAdd(container, intervalP);
  }

  if (newStartAt != NULL)
    kjDateTimePatch(intervalP, "startAt", newStartAt->value.s);
  if (newEndAt != NULL)
    kjDateTimePatch(intervalP, "endAt", newEndAt->value.s);
}



// -----------------------------------------------------------------------------
//
// managementPatch -
//
static void managementPatch(KjNode* dbRegP, KjNode* managementPatchP)
{
  KjNode* dbManagementP = kjLookup(dbRegP, "management");

  if (dbManagementP == NULL)
  {
    dbManagementP = kjClone(orionldState.kjsonP, managementPatchP);
    kjChildAdd(dbRegP, dbManagementP);
    return;
  }

  for (KjNode* patchItemP = managementPatchP->value.firstChildP; patchItemP != NULL; patchItemP = patchItemP->next)
  {
    if (strcmp(patchItemP->name, "localOnly") == 0)
      kjBooleanPatch(dbManagementP, patchItemP->name, patchItemP->value.b);
    else if (strcmp(patchItemP->name, "timeout") == 0)
    {
      if (patchItemP->type == KjInt)
        kjIntegerPatch(dbManagementP, patchItemP->name, patchItemP->value.i);
      else if (patchItemP->type == KjFloat)
        kjFloatPatch(dbManagementP, patchItemP->name, patchItemP->value.f);
    }
    else if (strcmp(patchItemP->name, "cooldown") == 0)
    {
      if (patchItemP->type == KjInt)
        kjIntegerPatch(dbManagementP, patchItemP->name, patchItemP->value.i);
      else if (patchItemP->type == KjFloat)
        kjFloatPatch(dbManagementP, patchItemP->name, patchItemP->value.f);
    }
    else if (strcmp(patchItemP->name, "cacheDuration") == 0) {}          // Ignored (Orion-LD doesn't implement this feature)
  }
}



// -----------------------------------------------------------------------------
//
// dbModelFromApiRegInformation -
//
static KjNode* dbModelFromApiRegInformation(KjNode* apiInformationP, const char* endpointUrl)
{
  KjNode* crV = kjArray(orionldState.kjsonP, "contextRegistration");

  for (KjNode* apiInfoItemP = apiInformationP->value.firstChildP; apiInfoItemP != NULL; apiInfoItemP = apiInfoItemP->next)
  {
    KjNode* dbCrItemP           = kjObject(orionldState.kjsonP, NULL);
    KjNode* apiEntitiesArrayP   = kjLookup(apiInfoItemP, "entities");
    KjNode* dbAttrsArrayP       = kjArray(orionldState.kjsonP, "attrs");
    KjNode* apiProperties       = kjLookup(apiInfoItemP, "propertyNames");
    KjNode* apiRelationships    = kjLookup(apiInfoItemP, "relationshipNames");

    if (apiEntitiesArrayP)
    {
      // Not DB-Model ready yet, but, let's just clone it and add it to dbCrItemP and fix it later
      KjNode* dbEntiesArrayP = kjClone(orionldState.kjsonP, apiEntitiesArrayP);
      kjChildAdd(dbCrItemP, dbEntiesArrayP);

      for (KjNode* dbEntityP = dbEntiesArrayP->value.firstChildP; dbEntityP != NULL; dbEntityP = dbEntityP->next)
      {
        KjNode* idPatternP = kjLookup(dbEntityP, "idPattern");
        if (idPatternP != NULL)
        {
          idPatternP->name   = (char*) "id";
          KjNode* isPatternP = kjString(orionldState.kjsonP, "isPattern", "true");
          kjChildAdd(dbEntityP, isPatternP);
        }
      }
    }
    kjChildAdd(dbCrItemP, dbAttrsArrayP);

    if (apiProperties != NULL)
    {
      for (KjNode* propertyNameP = apiProperties->value.firstChildP; propertyNameP != NULL; propertyNameP = propertyNameP->next)
      {
        KjNode* propObject = kjObject(orionldState.kjsonP, NULL);  // No name as part of an array
        KjNode* propTypeP  = kjString(orionldState.kjsonP, "type", "Property");
        KjNode* propNameP  = kjString(orionldState.kjsonP, "name", propertyNameP->value.s);

        kjChildAdd(propObject, propNameP);
        kjChildAdd(propObject, propTypeP);
        kjChildAdd(dbAttrsArrayP, propObject);
      }
    }

    // If apiRelationships
    if (apiRelationships != NULL)
    {
      for (KjNode* relationshipNameP = apiRelationships->value.firstChildP; relationshipNameP != NULL; relationshipNameP = relationshipNameP->next)
      {
        KjNode* relObject = kjObject(orionldState.kjsonP, NULL);  // No name as part of an array
        KjNode* relTypeP  = kjString(orionldState.kjsonP, "type", "Relationship");
        KjNode* relNameP  = kjString(orionldState.kjsonP, "name", relationshipNameP->value.s);

        kjChildAdd(relObject, relNameP);
        kjChildAdd(relObject, relTypeP);
        kjChildAdd(dbAttrsArrayP, relObject);
      }
    }

    KjNode* providingApplicationP = kjString(orionldState.kjsonP, "providingApplication", endpointUrl);
    kjChildAdd(dbCrItemP, providingApplicationP);

    kjChildAdd(crV, dbCrItemP);
  }

  return crV;
}



// -----------------------------------------------------------------------------
//
// informationAndEndpointPatch -
//
static void informationAndEndpointPatch(KjNode* dbRegP, KjNode* informationP, KjNode* endpointP)
{
  KjNode* dbCrP = kjLookup(dbRegP, "contextRegistration");
  if (dbCrP == NULL)
    LM_RVE(("Database Error (invalid registration in DB: contextRegistration array is missing)"));

  KjNode* crV = dbModelFromApiRegInformation(informationP, endpointP->value.s);

  dbCrP->name      = crV->name;
  dbCrP->value     = crV->value;
  dbCrP->lastChild = crV->lastChild;
}



// -----------------------------------------------------------------------------
//
// informationPatch -
//
static void informationPatch(KjNode* dbRegP, KjNode* informationP)
{
  //
  // Only "information" has been modified - meaning, we need to extract the "providingApplication" (endpoint)
  // from the database (it's part of the old value of "contextRegistration")
  //
  KjNode* dbCrP = kjLookup(dbRegP, "contextRegistration");

  if (dbCrP == NULL)
    LM_RVE(("Database Error (invalid registration in DB: contextRegistration array is missing)"));

  KjNode* dbCr1P = dbCrP->value.firstChildP;  // First item in the contextRegistration array
  if (dbCr1P == NULL)
    LM_RVE(("Database Error (invalid registration in DB: empty contextRegistration array)"));

  KjNode* providingApplicationP = kjLookup(dbCr1P, "providingApplication");
  if (providingApplicationP == NULL)
    LM_RVE(("Database Error (invalid registration in DB: missing 'providingApplication' in contextRegistration array item)"));

  //
  // Good, we have the endpoint. Now we can transform the "information" array into DB model
  //

  // Transform "API Registration::information" to DB-Model
  KjNode* crV = dbModelFromApiRegInformation(informationP, providingApplicationP->value.s);

  // Finally, replace the old "contextRegistration" with the output from dbModelFromApiRegInformation
  dbCrP->name      = crV->name;
  dbCrP->value     = crV->value;
  dbCrP->lastChild = crV->lastChild;
}



// -----------------------------------------------------------------------------
//
// endpointPatch - only the endpoint has changed - replace it in every item of the array
//
static void endpointPatch(KjNode* dbRegP, KjNode* endpointP)
{
  char*   endpointURL = endpointP->value.s;
  KjNode* dbCrV       = kjLookup(dbRegP, "contextRegistration");

  if (dbCrV != NULL)  // Can't ever be NULL ... just protection
  {
    for (KjNode* dbCrP = dbCrV->value.firstChildP; dbCrP != NULL; dbCrP = dbCrP->next)
    {
      KjNode* paP = kjLookup(dbCrP, "providingApplication");

      if (paP != NULL)  // Can't ever be NULL ...
        paP->value.s = endpointURL;
    }
  }
}



// -----------------------------------------------------------------------------
//
// dbRegistrationPatch -
//
static bool dbRegistrationPatch(KjNode* dbRegP, KjNode* regPatch, KjNode* propertyTree)
{
  KjNode* informationP = NULL;
  KjNode* endpointP    = NULL;

  //
  // Go over the entire patch ('regPatch') and modify the DB Model Registration tree ('dbRegP') accordingly
  //
  // NOTE:
  //   "id" and "type" have already been removed from 'regPatch'
  //
  for (KjNode* patchItemP = regPatch->value.firstChildP; patchItemP != NULL; patchItemP = patchItemP->next)
  {
    if ((strcmp(patchItemP->name, "registrationName") == 0) ||
        (strcmp(patchItemP->name, "name")             == 0))
    {
      kjStringPatch(dbRegP, "name", patchItemP->value.s);
    }
    else if (strcmp(patchItemP->name, "description") == 0)
      kjStringPatch(dbRegP, patchItemP->name, patchItemP->value.s);
    else if (strcmp(patchItemP->name, "information") == 0)
      informationP = patchItemP;
    else if (strcmp(patchItemP->name, "tenant") == 0)
      kjStringPatch(dbRegP, patchItemP->name, patchItemP->value.s);
    else if ((strcmp(patchItemP->name, "observationInterval") == 0) ||
             (strcmp(patchItemP->name, "managementInterval")  == 0))
    {
      kjTimeIntervalPatch(dbRegP, patchItemP->name, patchItemP);
    }
    else if ((strcmp(patchItemP->name, "location")         == 0) ||
             (strcmp(patchItemP->name, "observationSpace") == 0) ||
             (strcmp(patchItemP->name, "operationSpace")   == 0))
    {
      kjCompoundPatch(dbRegP, patchItemP->name, patchItemP);
    }
    else if (strcmp(patchItemP->name, "expiresAt") == 0)
      kjDateTimePatch(dbRegP, "expiration", patchItemP->value.s);  // NOTE the change of name expiresAt => expiration (NGSIv1 DB Model)
    else if (strcmp(patchItemP->name, "endpoint") == 0)
      endpointP = patchItemP;
    else if (strcmp(patchItemP->name, "contextSourceInfo") == 0)
      kjCompoundPatch(dbRegP, patchItemP->name, patchItemP);
    else if (strcmp(patchItemP->name, "scope") == 0)
    {
      if (patchItemP->type == KjString)
        kjStringPatch(dbRegP, patchItemP->name, patchItemP->value.s);
      else
        kjCompoundPatch(dbRegP, patchItemP->name, patchItemP);
    }
    else if (strcmp(patchItemP->name, "mode") == 0)
    {
      orionldError(OrionldBadRequestData, "Attempt to modify a write-once registration attribute", "mode", 400);
      return false;
    }
    else if (strcmp(patchItemP->name, "operations") == 0)
      kjCompoundPatch(dbRegP, patchItemP->name, patchItemP);
    else if (strcmp(patchItemP->name, "management") == 0)
      managementPatch(dbRegP, patchItemP);
    else if (strcmp(patchItemP->name, "refreshRate") == 0) {}  // Ignored (as Orion-LD doesn't implement this functionality)
    else if (strcmp(patchItemP->name, "status")      == 0) {}  // Ignored (as it's a read-only field)
    else if (strcmp(patchItemP->name, "timesSent")   == 0) {}  // Ignored (as it's a read-only field)
    else if (strcmp(patchItemP->name, "timesFailed") == 0) {}  // Ignored (as it's a read-only field)
    else if (strcmp(patchItemP->name, "lastSuccess") == 0) {}  // Ignored (as it's a read-only field)
    else if (strcmp(patchItemP->name, "lastFailure") == 0) {}  // Ignored (as it's a read-only field)
  }

  if (propertyTree != NULL)
  {
    KjNode* propertiesP  = kjLookup(dbRegP, "properties");
    if (propertiesP == NULL)
    {
      propertiesP = kjObject(orionldState.kjsonP, "properties");
      kjChildAdd(dbRegP, propertiesP);
    }

    for (KjNode* regPropertyP = propertyTree->value.firstChildP; regPropertyP != NULL; regPropertyP = regPropertyP->next)
    {
      if ((regPropertyP->type == KjArray) || (regPropertyP->type == KjObject))
        kjCompoundPatch(propertiesP, regPropertyP->name, regPropertyP);
      else if (regPropertyP->type == KjString)
        kjStringPatch(propertiesP, regPropertyP->name, regPropertyP->value.s);
      else if (regPropertyP->type == KjInt)
        kjIntegerPatch(propertiesP, regPropertyP->name, regPropertyP->value.i);
      else if (regPropertyP->type == KjFloat)
        kjFloatPatch(propertiesP, regPropertyP->name, regPropertyP->value.f);
      else if (regPropertyP->type == KjBoolean)
        kjBooleanPatch(propertiesP, regPropertyP->name, regPropertyP->value.b);
      else if (regPropertyP->type == KjNull)
      {
        KjNode* itemToRemove = kjLookup(propertiesP, regPropertyP->name);
        if (itemToRemove != NULL)
          kjChildRemove(propertiesP, itemToRemove);
      }
    }
  }

  if ((informationP != NULL) && (endpointP != NULL))
    informationAndEndpointPatch(dbRegP, informationP, endpointP);
  else if ((informationP != NULL) && (endpointP == NULL))
    informationPatch(dbRegP, informationP);
  else if ((informationP == NULL) && (endpointP != NULL))
    endpointPatch(dbRegP, endpointP);

  return true;
}



// -----------------------------------------------------------------------------
//
// regCounter -
//
static void regCounter(KjNode* dbRegP, const char* fieldName, int delta)
{
  KjNode* counterNodeP = kjLookup(dbRegP, fieldName);
  if (counterNodeP != NULL)
    counterNodeP->value.i += delta;
  else
  {
    counterNodeP = kjInteger(orionldState.kjsonP, fieldName, delta);
    kjChildAdd(dbRegP, counterNodeP);
  }
}



// -----------------------------------------------------------------------------
//
// regTimestamp -
//
static void regTimestamp(KjNode* dbRegP, const char* fieldName, double ts)
{
  KjNode* tsNodeP = kjLookup(dbRegP, fieldName);

  if (tsNodeP != NULL)
  {
    if (ts > tsNodeP->value.f)
      tsNodeP->value.f = ts;
  }
  else
  {
    tsNodeP = kjFloat(orionldState.kjsonP, fieldName, ts);
    kjChildAdd(dbRegP, tsNodeP);
  }
}



// ----------------------------------------------------------------------------
//
// orionldPatchRegistration -
//
bool orionldPatchRegistration(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))
    return legacyPatchRegistration();

  char* registrationId = orionldState.wildcard[0];

  PCHECK_URI(orionldState.wildcard[0], true, 0, "Registration ID must be a valid URI", orionldState.wildcard[0], 400);

  //
  // Check presence of Registration id + type
  //
  if ((orionldState.payloadIdNode != NULL) && (strcmp(orionldState.payloadIdNode->value.s, registrationId) != 0))
  {
    orionldError(OrionldBadRequestData, "Mismatch in Registration Id", orionldState.payloadIdNode->value.s, 400);
    return false;
  }

  if ((orionldState.payloadTypeNode != NULL) && (strcmp(orionldState.payloadTypeNode->value.s, "ContextSourceRegistration") != 0))
  {
    orionldError(OrionldBadRequestData, "Invalid value for 'type'", orionldState.payloadTypeNode->value.s, 400);
    return false;
  }

  RegCacheItem* rciP        = regCacheItemLookup(orionldState.tenantP->regCache, registrationId);
  KjNode*       regPatch    = orionldState.requestTree;
  KjNode*       dbRegP      = mongocRegistrationGet(registrationId);

  if (dbRegP == NULL)
  {
    orionldError(OrionldResourceNotFound, "Registration Not Found", registrationId, 404);
    return false;
  }

  //
  // Get the registration mode
  //
  KjNode*      regModeNodeP = kjLookup(dbRegP, "mode");
  const char*  regMode      = (regModeNodeP != NULL)? regModeNodeP->value.s : "inclusive";


  //
  // Check the validity of the incoming payload body - is it valid for a PATCH of a registration
  //
  // FIXME: the 5th parameter to pcheckRegistration says it's a PATCH.
  //        This parameter is passed to pcheckTimeInterval and if true, we don't need both
  //        startAt and endAt. One of them would be sufficient.
  //        HOWEVER, what if for example "observationInterval" didn't previously exist, it is patched in.
  //        if that is so, then both startAt and endAt are necessary
  //
  KjNode*         propertyTree = NULL;
  OrionldContext* fwdContextP  = NULL;
  if (pcheckRegistration(regMode, orionldState.requestTree, registrationId, false, false, &propertyTree, &fwdContextP) == false)
    return false;

  //
  // All good, we can start!
  //
  // The "versions" of the registration:
  //   1. rciP->regTree:  In the regCache   (cacheRegP)
  //   2. dbRegP:         In the database
  //   3. regPatch:       Incoming patch
  //
  // What we need:
  //   1. dbRegNewP     For the DB (dbRegP + regPatch + cacheRegP[counters+timestamps] + new modifiedAt
  //   2. cacheRegNewP  convert dbRegNewP after successful write to DB
  //
  // How to achieve it:
  //   1. dbRegP is used as the base for the merge with the incoming (regPatch)
  //   2. Update modifiedAt
  //   3. Update counters+timestamps from the cached copy
  //   4. Merge in the PATCH (regPatch)
  //   5. Write to mongo (overwrite the entire registration).
  //      - Even better would be to split the update in 3 parts:
  //        - $incr for counters
  //        - $max for timestamps
  //        - overwrite only those fields that are part of the PATCH
  //   6. Take the resulting DB-Reg and convert it into a cache-reg
  //   7. Replace the old cached registration
  //   8. Return 204 if all OK
  //

  //
  // modifiedAt
  //
  KjNode* modifiedAtP = kjLookup(dbRegP, "modifiedAt");

  if (modifiedAtP != NULL)
    modifiedAtP->value.f = orionldState.requestTime;

  // If "modifiedAt" or "createdAt" are present in the incoming payload data - they're to be ignored
  KjNode* nodeP;

  nodeP = kjLookup(regPatch, "modifiedAt");
  if (nodeP != NULL)
    kjChildRemove(regPatch, nodeP);

  nodeP = kjLookup(regPatch, "createdAt");
  if (nodeP != NULL)
    kjChildRemove(regPatch, nodeP);


  if (rciP != NULL)
  {
    //
    // Update counters+timestamps of dbRegP from the cached registration (that may have newer values)
    //
    if (rciP->deltas.timesSent   > 0)  regCounter(dbRegP,   "timesSent",   rciP->deltas.timesSent);
    if (rciP->deltas.timesFailed > 0)  regCounter(dbRegP,   "timesFailed", rciP->deltas.timesFailed);
    if (rciP->deltas.lastSuccess > 0)  regTimestamp(dbRegP, "lastSuccess", rciP->deltas.lastSuccess);
    if (rciP->deltas.lastFailure > 0)  regTimestamp(dbRegP, "lastFailure", rciP->deltas.lastFailure);

    KjNode* operationsP = kjLookup(regPatch, "operations");
    if (operationsP != NULL)
      rciP->opMask  = distOpTypeMask(operationsP);

    // The 'mode' of the registration cannot be altered

    if (fwdContextP != NULL)
      rciP->contextP = fwdContextP;

    KjNode* informationP = kjLookup(regPatch, "information");
    if (informationP != NULL)
      regCacheItemRegexRelease(rciP);
  }

  //
  // Merge in the PATCH (regPatch)
  //
  if (dbRegistrationPatch(dbRegP, regPatch, propertyTree) == false)
    return false;

  //
  // Write to mongo (overwriting the entire registration)
  // NOTE: To replace an item in mongo, the "_id" can't be present
  //       So, we clone the DB Reg and remove the _id from it.
  //       Must clone as dbRegP is used later for dbModelToApiRegistration
  KjNode* dbRegCopy = kjClone(orionldState.kjsonP, dbRegP);
  KjNode* _idP      = kjLookup(dbRegCopy, "_id");
  if (_idP != NULL)
    kjChildRemove(dbRegCopy, _idP);

  bool b = mongocRegistrationReplace(registrationId, dbRegCopy);
  if (b == false)
    return false;

  //
  // Replace the old cached registration
  //
  dbModelToApiRegistration(dbRegP, true, true);

  kjFree(rciP->regTree);
  rciP->regTree = kjClone(NULL, dbRegP);
  bzero(&rciP->deltas, sizeof(rciP->deltas));

  //
  // Update the regTree fields that are mirrored in RegCacheItem
  //
  KjNode* operationsP  = kjLookup(rciP->regTree, "operations");
  KjNode* modeP        = kjLookup(rciP->regTree, "mode");
  KjNode* informationP = kjLookup(rciP->regTree, "information");

  rciP->mode = (modeP != NULL)? registrationMode(modeP->value.s) : RegModeInclusive;

  if (operationsP != NULL)
    rciP->opMask = distOpTypeMask(operationsP);

  if (informationP != NULL)
  {
    if (regCacheIdPatternRegexCompile(rciP, informationP) == false)
      LM_X(1, ("Internal Error (if this happens it's a bug of Orion-LD - the idPattern was checked in pcheckEntityInfo and all OK"));
  }

  if (lmTraceIsSet(LmtRegCache))
    regCachePresent();

  //
  // Return 204 if all OK
  //
  orionldState.httpStatusCode = 204;
  return true;
}
