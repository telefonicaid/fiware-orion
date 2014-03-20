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

#include <regex.h>
#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/sem.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoOntimeintervalOperations.h"

#include "ngsi/EntityIdVector.h"
#include "ngsi/AttributeList.h"
#include "ngsi/ContextElementResponseVector.h"
#include "ngsi/Duration.h"
#include "parse/CompoundValueNode.h"

#include "ngsiNotify/Notifier.h"

using namespace mongo;

/* ****************************************************************************
*
* Globals
*/
static DBClientConnection*  connection;
static char*                entitiesCollectionName                      = NULL;
static char*                registrationsCollectionName                 = NULL;
static char*                subscribeContextCollectionName              = NULL;
static char*                subscribeContextAvailabilityCollectionName  = NULL;
static char*                assocationsCollectionName                   = NULL;
static Notifier*            notifier                                    = NULL;

/* ****************************************************************************
*
* Forward declarations
*/
static void compoundVectorResponse(orion::CompoundValueNode* cvP, const BSONElement& be);

/* ****************************************************************************
*
* mongoConnect -
*/
bool mongoConnect(const char* host, const char* db, const char* username, const char* passwd) {

    std::string err;

    mongoSemTake(__FUNCTION__, "connecting to mongo");

    /* The first argument to true is to use autoreconnect */
    connection = new DBClientConnection(true);

    if (!connection->connect(host, err)) {
        mongoSemGive(__FUNCTION__, "connecting to mongo failed");
        LM_RE(false, ("MongoDB connection fails: '%s'", err.c_str()));
    }

    if (strlen(db) != 0 && strlen(username) != 0 && strlen(passwd) != 0) {
        if (!connection->auth(std::string(db), std::string(username), std::string(passwd), err)) {
            mongoSemGive(__FUNCTION__, "connecting to mongo failed during authentication");
            LM_RE(false, ("Auth error (db=%s, username=%s, pswd=%s): %s", db, username, passwd, err.c_str()));
        }
    }

    mongoSemGive(__FUNCTION__, "connecting to mongo");
    return true;
}

/* ****************************************************************************
*
* mongoConnect -
*
* Version of the functions that doesn't uses authentication parameters
*
*/
bool mongoConnect(const char* host) {

    return mongoConnect(host, "", "", "");
}

/* ****************************************************************************
*
* mongoDisconnect -
*
* This method is intended for unit testing, that needs the DBClientConnection
* object to be mocked.
*
*/
void mongoDisconnect() {
    /* Safety check of null before releasing */
    if (connection != NULL) {
        delete connection;
    }
    connection = NULL;
}

/* ****************************************************************************
*
* mongoConnect -
*
* This method is intended for unit testing, that needs the DBClientConnection
* object to be mocked.
*
*/
#ifdef UNIT_TEST
bool mongoConnect(DBClientConnection* c) {

    connection = c;

    return true;
}
#endif

/*****************************************************************************
*
* getNotifier -
*/
Notifier* getNotifier() {
    return notifier;
}

/*****************************************************************************
*
* setNotifier -
*/
void setNotifier(Notifier* n) {
    notifier = n;
}

/* ****************************************************************************
*
* getMongoConnection -
*
* I would prefer to have per-collection methods, to have a better encapsulation, but
* the Mongo C++ API doesn't seem to work that way
*/
DBClientConnection* getMongoConnection(void) {
    return connection;
}

/*****************************************************************************
*
* setEntitiesCollectionName -
*/
void setEntitiesCollectionName(std::string name) {

    if (entitiesCollectionName != NULL) {
        free(entitiesCollectionName);
    }

    entitiesCollectionName = strdup(name.c_str());
}

/*****************************************************************************
*
* setRegistrationsCollectionName -
*/
void setRegistrationsCollectionName(std::string name) {

    if (registrationsCollectionName != NULL) {
        free(registrationsCollectionName);
    }

    registrationsCollectionName = strdup(name.c_str());
}

/*****************************************************************************
*
* setSubscribeContextCollectionName -
*/
void setSubscribeContextCollectionName(std::string name) {

    if (subscribeContextCollectionName != NULL) {
        free(subscribeContextCollectionName);
    }

    subscribeContextCollectionName = strdup(name.c_str());
}

/*****************************************************************************
*
* setSubscribeContextAvailabilityCollectionName -
*/
void setSubscribeContextAvailabilityCollectionName(std::string name) {

    if (subscribeContextAvailabilityCollectionName != NULL) {
        free(subscribeContextAvailabilityCollectionName);
    }

    subscribeContextAvailabilityCollectionName = strdup(name.c_str());
}

/*****************************************************************************
*
* setAssociationsCollectionName -
*/
extern void setAssociationsCollectionName(std::string name) {
    if (assocationsCollectionName != NULL) {
        free(assocationsCollectionName);
    }

    assocationsCollectionName = strdup(name.c_str());
}

/*****************************************************************************
*
* getEntitiesCollectionName -
*/
const char* getEntitiesCollectionName(void) {
    return entitiesCollectionName;
}

/*****************************************************************************
*
* getRegistrationsCollectionName -
*/
const char* getRegistrationsCollectionName(void) {
    return registrationsCollectionName;
}

/*****************************************************************************
*
* getSubscribeContextCollectionName -
*/
const char* getSubscribeContextCollectionName(void) {
    return subscribeContextCollectionName;
}

/*****************************************************************************
*
* getSubscribeContextAvailabilityCollectionName -
*/
const char* getSubscribeContextAvailabilityCollectionName(void) {
    return subscribeContextAvailabilityCollectionName;
}

/*****************************************************************************
*
* getAssociationsCollectionName -
*/
const char* getAssociationsCollectionName(void) {
    return assocationsCollectionName;
}

/* ****************************************************************************
*
* recoverOntimeIntervalThreads -
*/
void recoverOntimeIntervalThreads() {

    /* Look for ONTIMEINTERVAL subscriptions in database */
    std::string condType= std::string(CSUB_CONDITIONS) + "."  + CSUB_CONDITIONS_TYPE;
    BSONObj query = BSON(condType << ON_TIMEINTERVAL_CONDITION);

    DBClientConnection* connection = getMongoConnection();
    auto_ptr<DBClientCursor> cursor;
    try {
        LM_T(LmtMongo, ("query() in '%s' collection: '%s'", getSubscribeContextCollectionName(), query.toString().c_str()));
        mongoSemTake(__FUNCTION__, "query in SubscribeContextCollection");
        cursor = connection->query(getSubscribeContextCollectionName(), query);

        /*
         * We have observed that in some cases of DB errors (e.g. the database daemon is down) instead of
         * raising an exception, the query() method sets the cursor to NULL. In this case, we raise the
         * exception ourselves
         */
        if (cursor.get() == NULL) {
            throw DBException("Null cursor from mongo (details on this is found in the source code)", 0);
        }
        mongoSemGive(__FUNCTION__, "query in SubscribeContextCollection");
    }
    catch( const DBException &e ) {
        mongoSemGive(__FUNCTION__, "query in SubscribeContextCollection (mongp db exception)");
        LM_RVE(("Mongo DBException: %s", e.what()));
    }
    catch (...) {
        mongoSemGive(__FUNCTION__, "query in SubscribeContextCollection (mongp generic exception)");
        LM_RVE(("Caugth Mongo Generic Exception"));
    }

    /* For each one of the subscriptions found, create threads */
    while (cursor->more()) {

        BSONObj sub = cursor->next();
        std::string subId = sub.getField("_id").OID().str();

        std::vector<BSONElement> condV = sub.getField(CSUB_CONDITIONS).Array();
        for (unsigned int ix = 0; ix < condV.size(); ++ix) {
            BSONObj condition = condV[ix].embeddedObject();
            if (strcmp(STR_FIELD(condition, CSUB_CONDITIONS_TYPE).c_str(), ON_TIMEINTERVAL_CONDITION) == 0) {
               int interval = condition.getIntField(CSUB_CONDITIONS_VALUE);
               LM_T(LmtNotifier, ("creating ONTIMEINTERVAL for subscription %s with interval %d", subId.c_str(), interval));
               processOntimeIntervalCondition(subId, interval);
            }
        }
    }
}


/* ****************************************************************************
*
* includedEntity -
*/
bool includedEntity(EntityId en, EntityIdVector* entityIdV) {

    for (unsigned int ix = 0; ix < entityIdV->size(); ++ix) {
        EntityId* en2 = entityIdV->get(ix);

        bool idMatch;
        if (isTrue(en2->isPattern)) {
            regex_t regex;
            if (regcomp(&regex, en2->id.c_str(), 0) != 0) {
                LM_E(("error compiling regex: '%s'", en2->id.c_str()));
                continue;
            }
            if (regexec(&regex, en.id.c_str(), 0, NULL, 0) == 0) {
                idMatch = true;
            }
            else {
                idMatch = false;
            }
            regfree(&regex);
        }
        else {  /* isPattern=false */
            idMatch = (en2->id == en.id);
        }

        /* Note that type == "" is like a * wildcard */
        if (idMatch && (en.type == "" || en2->type == "" || en2->type == en.type)) {
            return true;
        }
    }
    return false;
}

/* ****************************************************************************
*
* includedAttribute -
*/
bool includedAttribute(ContextRegistrationAttribute attr, AttributeList* attrsV) {

    /* That's the case in which the discoverAvailabilityRequest doesn't include attributes, so all the
       attributes are included in the response*/
    if (attrsV->size() == 0) {
        return true;
    }

    for (unsigned int ix = 0; ix < attrsV->size(); ++ix) {
        if (attrsV->get(ix) == attr.name) {
            return true;
        }
    }
    return false;
}

/* ****************************************************************************
*
* includedAttribute -
*
* FIXME: note that in the current implementation, in which we only use 'name' to
* compare, this function is equal to the one for ContextRegistrationAttrribute.
* However, we keep them separated, as isDomain (present in ContextRegistrationAttribute
* but not in ContextRegistration could mean a difference). To review once domain attributes
* get implemented.
*
*/
bool includedAttribute(ContextAttribute attr, AttributeList* attrsV) {

    /* That's the case in which the queryContextRequest doesn't include attributes, so all the
       attributes are included in the response*/
    if (attrsV->size() == 0) {
        return true;
    }

    for (unsigned int ix = 0; ix < attrsV->size(); ++ix) {
        if (attrsV->get(ix) == attr.name) {
            return true;
        }
    }
    return false;
}

/* ****************************************************************************
*
* processEntitityPatternFalse -
*/
static void processEntitityPatternFalse(BSONArrayBuilder* arrayP, EntityId* enP, bool withType = true) {

    if (withType) {
        arrayP->append(BSON(ENT_ENTITY_ID << enP->id << ENT_ENTITY_TYPE << enP->type));
        LM_T(LmtMongo, ("Entity query token (isPattern=false): {id: %s, type: %s}", enP->id.c_str(), enP->type.c_str()));
    }
    else {
        arrayP->append(enP->id);
        LM_T(LmtMongo, ("Entity query token (isPattern=false): {id: %s}", enP->id.c_str()));
    }

}

/* ****************************************************************************
*
* processEntitityPatternFalse -
*/
static void processEntitityPatternTrue(BSONArrayBuilder* arrayP, EntityId* enP) {

    BSONObjBuilder enObjB;

    const std::string idString = std::string("_id.") + ENT_ENTITY_ID;
    const std::string typeString = std::string("_id.") + ENT_ENTITY_TYPE;

    enObjB.appendRegex(idString, enP->id);
    if (enP->type != "") {
        enObjB.append(typeString, enP->type);
    }

    BSONObj enObj = enObjB.obj();
    arrayP->append(enObj);

    LM_T(LmtMongo, ("Entity query token (isPattern=true): '%s'", enObj.toString().c_str()));

}

/* ****************************************************************************
*
* compoundObjectResponse -
*
*/
static void compoundObjectResponse(orion::CompoundValueNode* cvP, const BSONElement& be) {
    BSONObj obj = be.embeddedObject();
    cvP->type = orion::CompoundValueNode::Struct;
    for( BSONObj::iterator i = obj.begin(); i.more(); ) {
        orion::CompoundValueNode* child = new orion::CompoundValueNode();
        BSONElement e = i.next();
        if (e.type() == String) {
            child->name = e.fieldName();
            child->type = orion::CompoundValueNode::Leaf;
            child->value = e.String();
            cvP->add(child);
        }
        else if (e.type() == Object) {
            child->name = e.fieldName();
            compoundObjectResponse(child, e);
            cvP->add(child);
        }
        else if (e.type() == Array) {
            child->name = e.fieldName();
            compoundVectorResponse(child, e);
            cvP->add(child);
        }
        else {
            LM_E(("unknown BSON type"));;
        }

    }

}

/* ****************************************************************************
*
* compoundVectorResponse -
*/
static void compoundVectorResponse(orion::CompoundValueNode* cvP, const BSONElement& be) {
    std::vector<BSONElement> vec = be.Array();
    cvP->type = orion::CompoundValueNode::Vector;
    for( unsigned int ix = 0; ix < vec.size(); ++ix) {
        orion::CompoundValueNode* child = new orion::CompoundValueNode();
        BSONElement e = vec[ix];
        if (e.type() == String) {
            child->name = e.fieldName();
            child->type = orion::CompoundValueNode::Leaf;
            child->value = e.String();
            cvP->add(child);
        }
        else if (e.type() == Object) {
            child->name = e.fieldName();
            compoundObjectResponse(child, e);
            cvP->add(child);
        }
        else if (e.type() == Array) {
            child->name = e.fieldName();
            compoundVectorResponse(child, e);
            cvP->add(child);
        }
        else {
            LM_E(("unknown BSON type"));;
        }

    }
}


/* ****************************************************************************
*
* entitiesQuery -
*
* This method is used by queryContext and subscribeContext (ONCHANGE conditions). It takes
* a vector with entities and a vector with attributes as input and returns the corresponding
* ContextElementResponseVector or error.
*
* Note thte includeEmpty argument. This is used if we don't want the result to include empty
* attributes, i.e. the ones that cause '<contextValue></contextValue>'. This is aimed at
* subscribeContext case, as empty values can cause problems in the case of federating Context
* Brokers (the notifyContext is processed as an updateContext and in the latter case, an
* empty value causes an error)
*
*/
bool entitiesQuery(EntityIdVector enV, AttributeList attrL, ContextElementResponseVector* cerV, std::string* err, bool includeEmpty) {

    DBClientConnection* connection = getMongoConnection();

    /* Build query (entities part) */
    // FIXME P2: this implementation need to be refactored for cleanup

    /* We store isPattern=true and isPattern=false entities in different arrays, that will
     * be used in the final composition of the query */
    BSONArrayBuilder entitiesPatternFalseWithoutType;
    BSONArrayBuilder entitiesPatternFalseWithType;
    BSONArrayBuilder entitiesPatternTrue;
    for (unsigned int ix = 0; ix < enV.size(); ++ix) {
        if (isTrue(enV.get(ix)->isPattern)) {
            processEntitityPatternTrue(&entitiesPatternTrue, enV.get(ix));
        }
        else {
            if (enV.get(ix)->type == "") {
                processEntitityPatternFalse(&entitiesPatternFalseWithoutType, enV.get(ix), false);
            }
            else {
                processEntitityPatternFalse(&entitiesPatternFalseWithType, enV.get(ix), true);
            }
        }
    }

    /* Build query (attributes part) */

    BSONArrayBuilder attrs;
    for (unsigned int ix = 0; ix < attrL.size(); ++ix) {
        std::string attrName = attrL.get(ix);
        attrs.append(attrName);
        LM_T(LmtMongo, ("Attribute query token: '%s'", attrName.c_str()));
    }
    std::string attrNames = std::string(ENT_ATTRS) + "." + ENT_ATTRS_NAME;

    /* Compose final query */
    BSONObjBuilder queryBuilder;

    if (entitiesPatternFalseWithType.arrSize() > 0) {
        /* We need this checking to avoid a ugly "{ _id: { $in: {} } }" in the $or array
         * that would make the query fail */
        entitiesPatternTrue.append(BSON("_id" << BSON("$in" << entitiesPatternFalseWithType.arr())));
    }
    if (entitiesPatternFalseWithoutType.arrSize() > 0) {
        /* We need this checking to avoid a ugly "{ _id.id: { $in: {} } }" in the $or array
         * that would make the query fail */
        std::string idId = std::string("_id.") + ENT_ENTITY_ID;
        entitiesPatternTrue.append(BSON(idId << BSON("$in" << entitiesPatternFalseWithoutType.arr())));
    }

    /* The $or clause could be omitted if entitiesPatternTrue is empty, but we can assume that
     * it has no impact on MongoDB query optimizer */
    queryBuilder.append("$or", entitiesPatternTrue.arr());
    if (attrs.arrSize() > 0) {
        /* If we don't do this checking, the {$in: [] } in the attribute name part will
         * make the query fail*/
        queryBuilder.append(attrNames, BSON("$in" << attrs.arr()));
    }
    BSONObj query = queryBuilder.obj();

    /* Do the query on MongoDB */
    auto_ptr<DBClientCursor> cursor;
    try {
        LM_T(LmtMongo, ("query() in '%s' collection: '%s'", getEntitiesCollectionName(), query.toString().c_str()));
        mongoSemTake(__FUNCTION__, "query in EntitiesCollection");
        cursor = connection->query(getEntitiesCollectionName(), query);

        /*
         * We have observed that in some cases of DB errors (e.g. the database daemon is down) instead of
         * raising an exception, the query() method sets the cursor to NULL. In this case, we raise the
         * exception ourselves
         */
        if (cursor.get() == NULL) {
           throw DBException("Null cursor from mongo (details on this is found in the source code)", 0);
        }
        mongoSemGive(__FUNCTION__, "query in EntitiesCollection");
    }
    catch( const DBException &e ) {

        mongoSemGive(__FUNCTION__, "query in EntitiesCollection (mongo db exception)");
        *err = std::string("collection: ") + getEntitiesCollectionName() +
                " - query(): " + query.toString() +
                " - exception: " + e.what();

        LM_RE(false,(err->c_str()));
    }
    catch(...) {

        mongoSemGive(__FUNCTION__, "query in EntitiesCollection (mongo generic exception)");
        *err = std::string("collection: ") + getEntitiesCollectionName() +
                " - query(): " + query.toString() +
                " - exception: " + "generic";

        LM_RE(false, (err->c_str()));
    }

    /* Process query result */
    while (cursor->more()) {

        BSONObj r = cursor->next();
        LM_T(LmtMongo, ("retrieved document: '%s'", r.toString().c_str()));
        ContextElementResponse* cer = new ContextElementResponse();
        cer->statusCode.fill(SccOk);

        /* Entity part */

        BSONObj queryEntity = r.getObjectField("_id");

        cer->contextElement.entityId.id = STR_FIELD(queryEntity, ENT_ENTITY_ID);
        cer->contextElement.entityId.type = STR_FIELD(queryEntity, ENT_ENTITY_TYPE);
        cer->contextElement.entityId.isPattern = "false";

        /* Attributes part */

        std::vector<BSONElement> queryAttrV = r.getField(ENT_ATTRS).Array();
        for (unsigned int ix = 0; ix < queryAttrV.size(); ++ix) {

            ContextAttribute ca;

            BSONObj queryAttr = queryAttrV[ix].embeddedObject();

            ca.name = STR_FIELD(queryAttr, ENT_ATTRS_NAME);
            ca.type = STR_FIELD(queryAttr, ENT_ATTRS_TYPE);

            /* Note that includedAttribute decission is based on name and type. Value is set only if
             * decission is positive */
            if (includedAttribute(ca, &attrL)) {

                ContextAttribute* caP;
                if (queryAttr.getField(ENT_ATTRS_VALUE).type() == String) {
                    ca.value = STR_FIELD(queryAttr, ENT_ATTRS_VALUE);
                    if (!includeEmpty && ca.value.length() == 0) {
                        continue;
                    }
                    caP = new ContextAttribute(ca.name, ca.type, ca.value);
                }
                else if (queryAttr.getField(ENT_ATTRS_VALUE).type() == Object) {
                    caP = new ContextAttribute(ca.name, ca.type);
                    caP->compoundValueP = new orion::CompoundValueNode();
                    compoundObjectResponse(caP->compoundValueP, queryAttr.getField(ENT_ATTRS_VALUE));
                }
                else if (queryAttr.getField(ENT_ATTRS_VALUE).type() == Array) {
                    caP = new ContextAttribute(ca.name, ca.type);
                    caP->compoundValueP = new orion::CompoundValueNode();
                    compoundVectorResponse(caP->compoundValueP, queryAttr.getField(ENT_ATTRS_VALUE));
                }
                else {
                    LM_E(("unknown BSON type"));
                }

                /* Setting ID (if found) */
                if (STR_FIELD(queryAttr, ENT_ATTRS_ID) != "") {
                    Metadata* md = new Metadata(NGSI_MD_ID, "string", STR_FIELD(queryAttr, ENT_ATTRS_ID));
                    caP->metadataVector.push_back(md);
                }

                cer->contextElement.contextAttributeVector.push_back(caP);
            }

        }

        cer->statusCode.fill(SccOk);

        cerV->push_back(cer);
    }

    return true;

}

/*****************************************************************************
*
* processEntity -
*/
static void processEntity(ContextRegistrationResponse* crr, EntityIdVector enV, BSONObj entity) {

    EntityId en;

    en.id = STR_FIELD(entity, REG_ENTITY_ID);
    en.type = STR_FIELD(entity, REG_ENTITY_TYPE);
    /* isPattern = true is not allowed in registrations so it is not in the
     * document retrieved with the query; however we will set it to be formally correct
     * with NGSI spec */
    en.isPattern = std::string("false");

    if (includedEntity(en, &enV)) {       
        EntityId* enP = new EntityId(en.id, en.type, en.isPattern);
        crr->contextRegistration.entityIdVector.push_back(enP);
    }
}

/*****************************************************************************
*
* processAttribute -
*/
static void processAttribute(ContextRegistrationResponse* crr, AttributeList attrL, BSONObj attribute) {

    ContextRegistrationAttribute attr(
                STR_FIELD(attribute, REG_ATTRS_NAME),
                STR_FIELD(attribute, REG_ATTRS_TYPE),
                STR_FIELD(attribute, REG_ATTRS_ISDOMAIN));

   // FIXME: we don't take metadata into account at the moment
   //attr.metadataV = ..

   if (includedAttribute(attr, &attrL)) {

      ContextRegistrationAttribute* attrP = new ContextRegistrationAttribute(attr.name, attr.type, attr.isDomain);
      crr->contextRegistration.contextRegistrationAttributeVector.push_back(attrP);
   }

}

/*****************************************************************************
*
* processContextRegistrationElement -
*/
static void processContextRegistrationElement (BSONObj cr, EntityIdVector enV, AttributeList attrL, ContextRegistrationResponseVector* crrV) {

    ContextRegistrationResponse crr;

    crr.contextRegistration.providingApplication.set(STR_FIELD(cr, REG_PROVIDING_APPLICATION));

    std::vector<BSONElement> queryEntityV = cr.getField(REG_ENTITIES).Array();
    for (unsigned int ix = 0; ix < queryEntityV.size(); ++ix) {
        processEntity(&crr, enV, queryEntityV[ix].embeddedObject());
    }

    /* Note that attributes can be included only if at least one entity has been found */
    if (crr.contextRegistration.entityIdVector.size() > 0) {
        std::vector<BSONElement> queryAttrV = cr.getField(REG_ATTRS).Array();
        for (unsigned int ix = 0; ix < queryAttrV.size(); ++ix) {
            processAttribute(&crr, attrL, queryAttrV[ix].embeddedObject());
        }
    }

    // FIXME: we don't take metadata into account at the moment
    //crr.contextRegistration.registrationMetadataV = ..

    /* Note that the context registration element is only included in one of the following cases:
    * - The number of entities and attributes included are both greater than 0
    * - The number of entities is greater than 0, the number of attributes is 0 but the discover
    *   doesn't use attributes in the request
    */
    if (crr.contextRegistration.entityIdVector.size() == 0) {
        return;
    }

    if ( crr.contextRegistration.contextRegistrationAttributeVector.size() > 0  ||
        (crr.contextRegistration.contextRegistrationAttributeVector.size() == 0 && attrL.size() == 0)) {

        ContextRegistrationResponse* crrP = new ContextRegistrationResponse();

        crrP->contextRegistration = crr.contextRegistration;

        crrV->push_back(crrP);

    }

}

/* ****************************************************************************
*
* registrationsQuery -
*
* This method is used by discoverContextAvailabililty and subscribeContextAvailability. It takes
* a vector with entities and a vector with attributes as input and returns the corresponding
* ContextRegistrationResponseVector or error.
*
*/
bool registrationsQuery(EntityIdVector enV, AttributeList attrL, ContextRegistrationResponseVector* crrV, std::string* err) {

    DBClientConnection* connection = getMongoConnection();

    /* Build query based on arguments */
    // FIXME P2: this implementation need to be refactored for cleanup
    std::string contextRegistrationEntities = std::string(REG_CONTEXT_REGISTRATION) + "." + REG_ENTITIES;
    std::string contextRegistrationEntitiesId = std::string(REG_CONTEXT_REGISTRATION) + "." + REG_ENTITIES +
            "." + REG_ENTITY_ID;
    std::string contextRegistrationEntitiesType = std::string(REG_CONTEXT_REGISTRATION) + "." + REG_ENTITIES +
            "." + REG_ENTITY_TYPE;
    std::string contextRegistrationAttrsNames = std::string(REG_CONTEXT_REGISTRATION) + "." + REG_ATTRS +
            "." + REG_ATTRS_NAME;

    BSONArrayBuilder entityOr;
    BSONArrayBuilder entitiesWithType;
    BSONArrayBuilder entitiesWithoutType;    
    for (unsigned int ix = 0; ix < enV.size(); ++ix) {        
        EntityId* en = enV.get(ix);
        if (isTrue(en->isPattern)) {
            BSONObjBuilder b;
            b.appendRegex(contextRegistrationEntitiesId, en->id);
            if (en->type != "") {
                b.append(contextRegistrationEntitiesType, en->type);
            }
            entityOr.append(b.obj());
        }
        else { /* isPattern = false */
            if (en->type == "") {
                entitiesWithoutType.append(en->id);
                LM_T(LmtMongo, ("Entity discovery without type: id '%s'", en->id.c_str()));
            }
            else {
                /* We have detected that sometimes mongo stores { id: ..., type ...} and others { type: ..., id: ...},
                   so we have to take both them into account */
                entitiesWithType.append(BSON(REG_ENTITY_ID << en->id << REG_ENTITY_TYPE << en->type));
                entitiesWithType.append(BSON(REG_ENTITY_TYPE << en->type << REG_ENTITY_ID << en->id));
                LM_T(LmtMongo, ("Entity discovery: {id: %s, type: %s}", en->id.c_str(), en->type.c_str()));
            }
        }
    }
    BSONArrayBuilder attrs;
    for (unsigned int ix = 0; ix < attrL.size(); ++ix) {
        std::string attrName = attrL.get(ix);
        attrs.append(attrName);
        LM_T(LmtMongo, ("Attribute discovery: '%s'", attrName.c_str()));
    }

    entityOr.append(BSON(contextRegistrationEntities << BSON("$in" << entitiesWithType.arr())));
    entityOr.append(BSON(contextRegistrationEntitiesId << BSON("$in" <<entitiesWithoutType.arr())));

    BSONObjBuilder queryBuilder;
    /* The $or clause could be omitted if it contains only one element, but we can assume that
     * it has no impact on MongoDB query optimizer */
    queryBuilder.append("$or", entityOr.arr());
    queryBuilder.append(REG_EXPIRATION, BSON("$gt" << (long long) getCurrentTime()));
    if (attrs.arrSize() > 0) {
        /* If we don't do this checking, the {$in: [] } in the attribute name part will
         * make the query fail*/
        queryBuilder.append(contextRegistrationAttrsNames, BSON("$in" << attrs.arr()));
    }
    BSONObj query = queryBuilder.obj();

    /* Do the query on MongoDB */
    //FIXME P2: use field selector to include the only relevant field: contextRegistration array (e.g. "expiration" is not needed)
    auto_ptr<DBClientCursor> cursor;

    try {
        LM_T(LmtMongo, ("query() in '%s' collection: '%s'", getRegistrationsCollectionName(), query.toString().c_str()));
        mongoSemTake(__FUNCTION__, "query in RegistrationsCollection");
        cursor = connection->query(getRegistrationsCollectionName(), query);
        mongoSemGive(__FUNCTION__, "query in RegistrationsCollection");
    }
    catch( const DBException &e ) {

        mongoSemGive(__FUNCTION__, "query in RegistrationsCollection (mongo db exception)");
        *err = std::string("collection: ") + getRegistrationsCollectionName() +
                " - query(): " + query.toString() +
                " - exception: " + e.what();

        return false;
    }
    catch(...) {

        mongoSemGive(__FUNCTION__, "query in RegistrationsCollection (mongo generic exception)");
        *err = std::string("collection: ") + getRegistrationsCollectionName() +
                " - query(): " + query.toString() +
                " - exception: " + "generic";

        return false;
    }

    /* Process query result */
    while (cursor->more()) {
        BSONObj r = cursor->next();
        LM_T(LmtMongo, ("retrieved document: '%s'", r.toString().c_str()));

        std::vector<BSONElement> queryContextRegistrationV = r.getField(REG_CONTEXT_REGISTRATION).Array();
        for (unsigned int ix = 0 ; ix < queryContextRegistrationV.size(); ++ix) {
            processContextRegistrationElement(queryContextRegistrationV[ix].embeddedObject(), enV, attrL, crrV);
        }

        /* FIXME: note that given the response doesn't distinguish from which registration ID the
         * response comes, it could have that we have same context registration elements, belong to different
         * registrations ID at DB level, thus causing a duplicated context element response. Moreover,
         * NGSI doesn't forbid to registry exactly twice the same context registration element in the
         * same registration ID. Thus, it could be interesting to post-process the response vector, to
         * "compact" removing duplicated responses.*/

    }

    return true;
}

/* ****************************************************************************
*
* isCondValueInContextElementResponse -
*/
bool isCondValueInContextElementResponse(ConditionValueList* condValues, ContextElementResponseVector* cerV) {

    for (unsigned int cvlx = 0; cvlx < condValues->size(); ++cvlx) {
        for (unsigned int aclx = 0; aclx < cerV->size(); ++aclx) {
            ContextAttributeVector caV = cerV->get(aclx)->contextElement.contextAttributeVector;
            for (unsigned int kx = 0; kx < caV.size(); ++kx) {
                if (caV.get(kx)->name == condValues->get(cvlx)) {
                    return true;
                }
            }
        }
    }
    return false;

}

/* ****************************************************************************
*
* subToEntityIdVector -
*
* Extract the entity ID vector from a BSON document (in the format of the csubs/casub
* collection)
*
*/
EntityIdVector subToEntityIdVector(BSONObj sub) {
    EntityIdVector enV;
    std::vector<BSONElement> subEnts = sub.getField(CSUB_ENTITIES).Array();
    for (unsigned int ix = 0; ix < subEnts.size() ; ++ix) {
        BSONObj subEnt = subEnts[ix].embeddedObject();
        EntityId* en = new EntityId(STR_FIELD(subEnt, CSUB_ENTITY_ID),
                                    STR_FIELD(subEnt, CSUB_ENTITY_TYPE),
                                    STR_FIELD(subEnt, CSUB_ENTITY_ISPATTERN));
        enV.push_back(en);
    }
    return enV;
}

/* ****************************************************************************
*
* subToAttributeList -
*
* Extract the attribute list from a BSON document (in the format of the csubs/casub
* collection)
*
*/
AttributeList subToAttributeList(BSONObj sub) {
    AttributeList attrL;
    std::vector<BSONElement> subAttrs = sub.getField(CSUB_ATTRS).Array();
    for (unsigned int ix = 0; ix < subAttrs.size() ; ++ix) {
        std::string subAttr = subAttrs[ix].String();
        attrL.push_back(subAttr);
    }
    return attrL;
}

/* ****************************************************************************
*
* processOnChangeCondition -
*
* This function is called from two places:
*
* 1) initial processing of an ONCHANGE condition in processConditionVector (used from
*   subscribeContext and updateContextSubscription), so an "initial"
*   notification for all the entites/attributes included in the entity in the case
*   that some of them are within the ones in the condValues.
* 2) updateContext processing logic when attributes under an ONCHANGE condition are
*   updated
*
* The argument enV is the entities and attributes in the subscribeContext
* request (case 1) or the "triggering" entity (case 2). The argument attrL is the
* attributes in the subscribeContext request. Note that is condValues is NULL, the checking
* on condValues is omitted (this is the case when this function is called from updateContext,
* where the previous query on csubs ensures that that condition is true)
*
* This method returns true if the notification was actually send. Otherwise, false
* is returned. This is used in the caller to know if lastNotification field in the
* subscription document in csubs collection has to be modified or not.
*/
bool processOnChangeCondition(EntityIdVector enV, AttributeList attrL, ConditionValueList* condValues, std::string subId, std::string notifyUrl, Format format) {

    std::string err;
    NotifyContextRequest ncr;

    if (!entitiesQuery(enV, attrL, &ncr.contextElementResponseVector, &err, false)) {
        ncr.contextElementResponseVector.release();
        LM_RE(false, (err.c_str()));
    }

    if (ncr.contextElementResponseVector.size() > 0) {

        /* Complete the fields in NotifyContextRequest */
        ncr.subscriptionId.set(subId);
        //FIXME: we use a proper origin name
        ncr.originator.set("localhost");

        if (condValues != NULL) {
            /* Check if some of the attributes in the NotifyCondition values list are in the entity.
             * Note that in this case we do a query for all the attributes, not restricted to attrV */
            ContextElementResponseVector allCerV;
            AttributeList emptyList;
            if (!entitiesQuery(enV, emptyList, &allCerV, &err, false)) {
                allCerV.release();
                ncr.contextElementResponseVector.release();
                LM_RE(false, (err.c_str()));
            }

            if (isCondValueInContextElementResponse(condValues, &allCerV)) {
                /* Send notification */
                getNotifier()->sendNotifyContextRequest(&ncr, notifyUrl, format);
                allCerV.release();
                ncr.contextElementResponseVector.release();
                return true;
            }

            allCerV.release();
        }
        else {
            getNotifier()->sendNotifyContextRequest(&ncr, notifyUrl, format);
            ncr.contextElementResponseVector.release();
            return true;
        }
    }

    ncr.contextElementResponseVector.release();
    return false;
}

/* ****************************************************************************
*
* processOntimeIntervalCondition -
*/
void processOntimeIntervalCondition(std::string subId, int interval) {

    getNotifier()->createIntervalThread(subId, interval);

}

/* ****************************************************************************
*
* processConditionVector -
*
*/
BSONArray processConditionVector(NotifyConditionVector* ncvP, EntityIdVector enV, AttributeList attrL, std::string subId, std::string url, bool* notificationDone, Format format) {

    BSONArrayBuilder conds;
    *notificationDone = false;

    for (unsigned int ix = 0; ix < ncvP->size(); ++ix) {
        NotifyCondition* nc = ncvP->get(ix);
        if (nc->type == ON_TIMEINTERVAL_CONDITION) {

            Duration interval;
            interval.set(nc->condValueList.get(0));
            interval.parse();

            conds.append(BSON(CSUB_CONDITIONS_TYPE << ON_TIMEINTERVAL_CONDITION <<
                              CSUB_CONDITIONS_VALUE << interval.seconds));

            processOntimeIntervalCondition(subId, interval.seconds);
        }
        else if (nc->type == ON_CHANGE_CONDITION) {

            /* Create an array holding the list of condValues */
            BSONArrayBuilder condValues;
            for (unsigned int jx = 0; jx < nc->condValueList.size(); ++jx) {
                condValues.append(nc->condValueList.get(jx));
            }

            conds.append(BSON(CSUB_CONDITIONS_TYPE << ON_CHANGE_CONDITION <<
                              CSUB_CONDITIONS_VALUE << condValues.arr()));

            if (processOnChangeCondition(enV,
                                     attrL,
                                     &(nc->condValueList),
                                     subId,
                                     url,
                                     format)) {

                *notificationDone = true;
            }

        }
        else {  // ON_VALUE_CONDITION
            // FIXME: not implemented
        }
    }
    return conds.arr();
}

/* ****************************************************************************
*
* mongoUpdateCasubNewNotification -
*
* This methos is pretty similar to the mongoUpdateCsubNewNotification in mongoOntimeintervalOperations module.
* However, it doesn't take semaphore
*
*/
static HttpStatusCode mongoUpdateCasubNewNotification(std::string subId, std::string* err) {

    LM_T(LmtMongo, ("Update NGSI9 Subscription New Notification"));

    DBClientConnection* connection = getMongoConnection();

    /* Update the document */
    try {
        BSONObj query = BSON("_id" << OID(subId));
        BSONObj update = BSON("$set" << BSON(CASUB_LASTNOTIFICATION << getCurrentTime()) << "$inc" << BSON(CASUB_COUNT << 1));
        LM_T(LmtMongo, ("update() in '%s' collection: (%s,%s)", getSubscribeContextAvailabilityCollectionName(),
                        query.toString().c_str(),
                        update.toString().c_str()));

        mongoSemTake(__FUNCTION__, "update in SubscribeContextAvailabilityCollection");
        connection->update(getSubscribeContextAvailabilityCollectionName(), query, update);
        mongoSemGive(__FUNCTION__, "update in SubscribeContextAvailabilityCollection");
    }
    catch( const DBException &e ) {
        mongoSemGive(__FUNCTION__, "update in SubscribeContextAvailabilityCollection (mongo db exception)");
        *err = e.what();
        LM_RE(SccOk, ("Database error '%s'", err->c_str()));
    }
    catch(...) {
        mongoSemGive(__FUNCTION__, "update in SubscribeContextAvailabilityCollection (mongo generic exception)");
        *err = "Database error - exception thrown";
        LM_RE(SccOk, ("Database error - exception thrown"));
    }

    return SccOk;
}

/* ****************************************************************************
*
* processAvailabilitySubscription -
*
* This function is called from two places:
*
* 1) initial processing of subscribeContextAvailability (and updateContextAvailabilitySubscription),
*   so an "initial" notification for all matching context registrations is sent
* 2) registerContext processing logic when the new (or updated) context registration
*   matches an availability subscription
*
* The enV arguments is set with all the entities included in the subscription (case 1) or
* with only the triggering entities (case 2).
*
* This method returns true if the notification was actually send. Otherwise, false
* is returned.
*/
bool processAvailabilitySubscription(EntityIdVector enV, AttributeList attrL, std::string subId, std::string notifyUrl, Format format) {

    std::string err;
    NotifyContextAvailabilityRequest ncar;

    if (!registrationsQuery(enV, attrL, &ncar.contextRegistrationResponseVector, &err)) {
       ncar.contextRegistrationResponseVector.release();
       LM_RE(false, (err.c_str()));
    }

    if (ncar.contextRegistrationResponseVector.size() > 0) {

        /* Complete the fields in NotifyContextRequest */
        ncar.subscriptionId.set(subId);

        getNotifier()->sendNotifyContextAvailabilityRequest(&ncar, notifyUrl, format);
        ncar.contextRegistrationResponseVector.release();

        /* Update database fields due to new notification */
        if (mongoUpdateCasubNewNotification(subId, &err) != SccOk) {
            LM_RE(false, ("error invoking mongoUpdateCasubNewNotification: '%s'", err.c_str()));
        }

        return true;
    }

    ncar.contextRegistrationResponseVector.release();
    return false;
}
