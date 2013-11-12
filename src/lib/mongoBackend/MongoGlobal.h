#ifndef MONGO_GLOBAL_H
#define MONGO_GLOBAL_H

/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* fermin at tid dot es
*
* Author: Fermín Galán
*/

#include "logMsg/logMsg.h"

#include "mongo/client/dbclient.h"

#include "ngsi/EntityId.h"
#include "ngsi/ContextRegistrationAttribute.h"
#include "ngsi/ContextAttribute.h"
#include "ngsi/EntityIdVector.h"
#include "ngsi/AttributeList.h"
#include "ngsi/ContextElementResponseVector.h"
#include "ngsi/ConditionValueList.h"
#include "ngsi/NotifyConditionVector.h"
#include "ngsi10/UpdateContextResponse.h"
#include "ngsi9/RegisterContextRequest.h"
#include "ngsi9/RegisterContextResponse.h"

#include "ngsiNotify/Notifier.h"

using namespace mongo;

/*****************************************************************************
* Constant string for field names in collection (first characters
* are the code name: REG_, ENT_, ASUB_, CSUB_, ASSOC_ */

#define REG_FWS_REGID               "fwdRegId"
#define REG_CONTEXT_REGISTRATION    "contextRegistration"
#define REG_PROVIDING_APPLICATION   "providingApplication"
#define REG_ENTITIES                "entities"
#define REG_ATTRS                   "attrs"
#define REG_EXPIRATION              "expiration"
#define REG_ENTITY_ID               "id"
#define REG_ENTITY_TYPE             "type"
#define REG_ATTRS_NAME              "name"
#define REG_ATTRS_TYPE              "type"
#define REG_ATTRS_ISDOMAIN          "isDomain"

#define ENT_ATTRS                    "attrs"
#define ENT_ENTITY_ID                "id"
#define ENT_ENTITY_TYPE              "type"
#define ENT_ATTRS_NAME               "name"
#define ENT_ATTRS_TYPE               "type"
#define ENT_ATTRS_VALUE              "value"
#define ENT_ATTRS_MODIFICATION_DATE  "modDate"
#define ENT_CREATION_DATE            "creDate"
#define ENT_MODIFICATION_DATE        "modDate"

#define CSUB_EXPIRATION         "expiration"
#define CSUB_LASTNOTIFICATION   "lastNotification"
#define CSUB_REFERENCE          "reference"
#define CSUB_CONDITIONS         "conditions"
#define CSUB_CONDITIONS_TYPE    "type"
#define CSUB_CONDITIONS_VALUE   "value"
#define CSUB_THROTTLING         "throttling"
#define CSUB_ENTITIES           "entities"
#define CSUB_ATTRS              "attrs"
#define CSUB_ENTITY_ID          "id"
#define CSUB_ENTITY_TYPE        "type"
#define CSUB_ENTITY_ISPATTERN   "isPattern"
#define CSUB_COUNT              "count"
#define CSUB_FORMAT             "format"

#define CASUB_EXPIRATION        "expiration"
#define CASUB_REFERENCE         "reference"
#define CASUB_ENTITIES          "entities"
#define CASUB_ATTRS             "attrs"
#define CASUB_ENTITY_ID         "id"
#define CASUB_ENTITY_TYPE       "type"
#define CASUB_ENTITY_ISPATTERN  "isPattern"
#define CASUB_LASTNOTIFICATION  "lastNotification"
#define CASUB_COUNT             "count"
#define CASUB_FORMAT            "format"

#define ASSOC_SOURCE_ENT        "srcEnt"
#define ASSOC_TARGET_ENT        "tgtEnt"
#define ASSOC_ENT_ID            "id"
#define ASSOC_ENT_TYPE          "type"
#define ASSOC_ATTRS             "attrs"
#define ASSOC_ATTRS_SOURCE      "src"
#define ASSOC_ATTRS_TARGET      "tgt"

/*****************************************************************************
*
* Macro to ease extracting fields from BSON objects
*/
#define STR_FIELD(i, sf) std::string(i.getStringField(sf))
#define C_STR_FIELD(i, sf) i.getStringField(sf)


/* ****************************************************************************
*
* log macros that also free semaphore - 
*/
#define LM_SRE(retVal, s)          \
do {                               \
   LM_E(s);                        \
   semGive();                      \
   return retVal;                  \
} while (0)

#define LM_SRVE(s)                 \
do {                               \
   LM_E(s);                        \
   semGive();                      \
   return;                         \
} while (0)

#define LM_SR(retVal)              \
do {                               \
   semGive();                      \
   return retVal;                  \
} while (0)

#define LM_SRV                     \
do {                               \
   semGive();                      \
   return;                         \
} while (0)

/*****************************************************************************
*
* mongoConnect -
*/
extern bool mongoConnect(const char* host, const char* db, const char* username, const char* passwd);
extern bool mongoConnect(const char* host);
#ifdef UNIT_TEST
extern bool mongoConnect(DBClientConnection* c);
#endif

/*****************************************************************************
*
* getNotifier -
*/
extern Notifier* getNotifier();

/*****************************************************************************
*
* setNotifier -
*/
extern void setNotifier(Notifier* n);

/*****************************************************************************
*
* getMongoConnection -
*/
extern void mongoDisconnect();

/*****************************************************************************
*
* getMongoConnection -
*
* I would prefer to have per-collection methods, to have a better encapsulation, but
* the Mongo C++ API seems not working that way
*/
extern DBClientConnection* getMongoConnection(void);

/*****************************************************************************
*
* setEntitiesCollectionName -
*/
extern void setEntitiesCollectionName(const char* name);

/*****************************************************************************
*
* setRegistrationsCollectionName -
*/
extern void setRegistrationsCollectionName(const char* name);

/*****************************************************************************
*
* setSubscribeContextCollectionName -
*/
extern void setSubscribeContextCollectionName(const char* name);

/*****************************************************************************
*
* setSubscribeContextAvailabilityCollectionName -
*/
extern void setSubscribeContextAvailabilityCollectionName(const char* name);

/*****************************************************************************
*
* setAssociationsCollectionName -
*/
extern void setAssociationsCollectionName(const char* name);

/*****************************************************************************
*
* getEntitiesCollectionName -
*/
extern const char* getEntitiesCollectionName(void);

/*****************************************************************************
*
* getRegistrationsCollectionName -
*/
extern const char* getRegistrationsCollectionName(void);

/*****************************************************************************
*
* getSubscribeContextCollectionName -
*/
extern const char* getSubscribeContextCollectionName(void);

/*****************************************************************************
*
* getSubscribeContextAvailabilityCollectionName -
*/
extern const char* getSubscribeContextAvailabilityCollectionName(void);

/*****************************************************************************
*
* getAssociationsCollectionName -
*/
extern const char* getAssociationsCollectionName(void);

/*****************************************************************************
*
* resetDb -
*/
extern bool resetDb(void);

/* ****************************************************************************
*
* includedEntity -
*/
extern bool includedEntity(EntityId en, EntityIdVector* entityIdV);

/* ****************************************************************************
*
* includedAttribute -
*/
extern bool includedAttribute(ContextRegistrationAttribute attr, AttributeList* attrsV);

/* ****************************************************************************
*
* includedAttribute -
*/
extern bool includedAttribute(ContextAttribute attr, AttributeList* attrsV);

/* ****************************************************************************
*
* entitiesQuery -
*
*/
extern bool entitiesQuery(EntityIdVector enV, AttributeList attrL, ContextElementResponseVector* cerV, std::string* err, bool includeEmpty);

/* ****************************************************************************
*
* registrationsQuery -
*
*/
extern bool registrationsQuery(EntityIdVector enV, AttributeList attrL, ContextRegistrationResponseVector* cerV, std::string* err);

/* ****************************************************************************
*
* subToEntityIdVector -
*
* Extract the entity ID vector from a BSON document (in the format of the csubs
* collection)
*
*/
extern EntityIdVector subToEntityIdVector(BSONObj sub);

/* ****************************************************************************
*
* subToAttributeList -
*
* Extract the attribute list from a BSON document (in the format of the csubs
* collection)
*
*/
extern AttributeList subToAttributeList(BSONObj attrL);

/* ****************************************************************************
*
* processOnChangeCondition -
*
*/
extern void processOntimeIntervalCondition(std::string subId, int interval);

/* ****************************************************************************
*
* processOnChangeCondition -
*
*/
extern bool processOnChangeCondition(EntityIdVector enV, AttributeList attrV, ConditionValueList* condValues, std::string subId, std::string notifyUrl, Format format);

/* ****************************************************************************
*
* processConditionVector -
*
*/
extern BSONArray processConditionVector(NotifyConditionVector* ncvP, EntityIdVector enV, AttributeList attrL, std::string subId, std::string url, bool* notificationDone, Format format);

/* ****************************************************************************
*
* processAvailabilitySubscriptions -
*
*/
extern bool processAvailabilitySubscription(EntityIdVector enV, AttributeList attrL, std::string subId, std::string notifyUrl, Format format);

#endif
