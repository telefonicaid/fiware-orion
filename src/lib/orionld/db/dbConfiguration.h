#ifndef SRC_LIB_ORIONLD_DB_DBCONFIGURATION_H_
#define SRC_LIB_ORIONLD_DB_DBCONFIGURATION_H_

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
#include "kjson/KjNode.h"                                        // KjNode
}

#include "orionld/common/QNode.h"                                // QNode
#include "orionld/types/OrionldProblemDetails.h"                 // OrionldProblemDetails



// -----------------------------------------------------------------------------
//
// DB_DRIVER_MONGO_CPP_LEGACY - Use the mongo C++ Legacy driver
//
#define DB_DRIVER_MONGO_CPP_LEGACY 1



// -----------------------------------------------------------------------------
//
// DB_DRIVER_MONGOC - Use the "newest" mongo C driver
//
// #define DB_DRIVER_MONGOC 1



// -----------------------------------------------------------------------------
//
// Callback types for the DB interface
//
typedef bool    (*DbSubscriptionMatchCallback)(const char* entityId, KjNode* subscriptionTree, KjNode* currentEntityTree, KjNode* incomingRequestTree);



// -----------------------------------------------------------------------------
//
// Function pointer types for the DB interface
//
typedef KjNode* (*DbEntityLookupFunction)(const char* entityId);
typedef KjNode* (*DbEntityRetrieveFunction)(const char* entityId, char** attrs, bool attrMandatory, bool sysAttrs, bool keyValues, const char* datasetId);
typedef KjNode* (*DbEntityLookupManyFunction)(KjNode* requestTree);
typedef KjNode* (*DbEntityAttributeLookupFunction)(const char* entityId, const char* attributeName);
typedef bool    (*DbEntityAttributesDeleteFunction)(const char* entityId, char** attrNameV, int vecSize);
typedef bool    (*DbEntityUpdateFunction)(const char* entityId, KjNode* requestTree);
typedef bool    (*DbEntityFieldReplaceFunction)(const char* entityId, const char* fieldName, KjNode* fieldValeNodeP);
typedef bool    (*DbEntityDeleteFunction)(const char* entityId);
typedef bool    (*DbEntitiesDeleteFunction)(KjNode* entityIdsArray);
typedef KjNode* (*DbDataToKjTreeFunction)(const void* dbData, char** titleP, char** detailsP);
typedef void    (*DbDataFromKjTreeFunction)(KjNode* nodeP, void* dbDataP);
typedef void    (*DbSubscriptionMatchEntityIdAndAttributes)(const char* entityId, KjNode* currentEntityTree, KjNode* incomingRequestTree, DbSubscriptionMatchCallback callback);
typedef KjNode* (*DbEntityListLookupWithIdTypeCreDate)(KjNode* entityIdsArray, bool attrNames);
typedef KjNode* (*DbRegistrationLookup)(const char* entityId, const char* attribute, int* noOfRegsP);
typedef bool    (*DbRegistrationExists)(const char* registrationId);
typedef bool    (*DbRegistrationDelete)(const char* registrationId);
typedef KjNode* (*DbSubscriptionGet)(const char* subscriptionId);
typedef bool    (*DbSubscriptionReplace)(const char* subscriptionId, KjNode* dbSubscriptionP);
typedef bool    (*DbSubscriptionDelete)(const char* subscriptionId);
typedef KjNode* (*DbRegistrationGet)(const char* registrationId);
typedef bool    (*DbRegistrationReplace)(const char* registrationId, KjNode* dbRegistrationP);
typedef KjNode* (*DbEntitiesGet)(char** fieldV, int fields);
typedef KjNode* (*DbEntityTypesFromRegistrationsGet)(void);
typedef bool    (*DbGeoIndexCreate)(const char* tenant, const char* attrName);
typedef KjNode* (*DbEntitiesQuery)(KjNode* entityInfoArrayP, KjNode* attrsP, QNode* qP, KjNode* geoqP, int limit, int offset, int* countP);



// -----------------------------------------------------------------------------
//
// Function pointers for the DB interface
//
extern DbEntityLookupFunction                    dbEntityLookup;
extern DbEntityRetrieveFunction                  dbEntityRetrieve;
extern DbEntityLookupManyFunction                dbEntityLookupMany;
extern DbEntityAttributeLookupFunction           dbEntityAttributeLookup;
extern DbEntityAttributesDeleteFunction          dbEntityAttributesDelete;
extern DbEntityUpdateFunction                    dbEntityUpdate;
extern DbEntityFieldReplaceFunction              dbEntityFieldReplace;
extern DbEntityDeleteFunction                    dbEntityDelete;
extern DbEntitiesDeleteFunction                  dbEntitiesDelete;
extern DbDataToKjTreeFunction                    dbDataToKjTree;
extern DbDataFromKjTreeFunction                  dbDataFromKjTree;
extern DbSubscriptionMatchEntityIdAndAttributes  dbSubscriptionMatchEntityIdAndAttributes;
extern DbEntityListLookupWithIdTypeCreDate       dbEntityListLookupWithIdTypeCreDate;
extern DbRegistrationLookup                      dbRegistrationLookup;
extern DbRegistrationExists                      dbRegistrationExists;
extern DbRegistrationDelete                      dbRegistrationDelete;
extern DbSubscriptionGet                         dbSubscriptionGet;
extern DbSubscriptionReplace                     dbSubscriptionReplace;
extern DbSubscriptionDelete                      dbSubscriptionDelete;
extern DbRegistrationGet                         dbRegistrationGet;
extern DbRegistrationReplace                     dbRegistrationReplace;
extern DbEntityTypesFromRegistrationsGet         dbEntityTypesFromRegistrationsGet;
extern DbEntitiesGet                             dbEntitiesGet;
extern DbGeoIndexCreate                          dbGeoIndexCreate;
extern DbEntitiesQuery                           dbEntitiesQuery;

#endif  // SRC_LIB_ORIONLD_DB_DBCONFIGURATION_H_
