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
*
*/

#include "mongoBackend/MongoCommonUpdate.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/sem.h"
#include "mongoBackend/MongoGlobal.h"

/* ****************************************************************************
*
* smartAttrMatch -
*
* Note this function is not conmutative: empty type2 match whatever type, but type1 only
* match an empty type2. Element 1 is supposed to be the one in database, while element 2
* is the one supposed to be in the NGSI request.
*
* FIXME: probably this code could be useful in other places. It should be moved to
* (and used from) a global place, maybe as part as the class-refactoring within
* EntityId or Attribute class methods.
*/
bool smartAttrMatch(std::string name1, std::string type1, std::string id1, std::string name2, std::string type2, std::string id2) {
    if (type2 == "") {
        return ((name1 == name2) && (id1 == id2));
    }
    else {
        return ((name1 == name2) && (type1 == type2) && (id1 == id2));
    }
}

/* ****************************************************************************
*
* checkAndUpdate -
*
* Add the 'attr' attribute to the BSONObjBuilder (which is passed by reference), doing
* the actual update in the case of ContextAttribute matches.
*
* Returns true if the update was done, false if only a "attribute copy" was done. If true,
* the "actualUpdate" argument (passed by reference) is set to true in the case that the
* original value of the attribute was different that the one used in the update (this is
* important for ONCHANGE notifications).
*/
static bool checkAndUpdate (BSONObjBuilder* newAttr, BSONObj attr, ContextAttribute ca, bool* actualUpdate) {

    bool updated = false;
    *actualUpdate = false;

    newAttr->append(ENT_ATTRS_NAME, STR_FIELD(attr, ENT_ATTRS_NAME));
    newAttr->append(ENT_ATTRS_TYPE, STR_FIELD(attr, ENT_ATTRS_TYPE));
    /* The hasField() check is needed to preserve compatibility with entities that were created
     * in database by a CB instance previous to the support of creation and modification dates */
    if (attr.hasField(ENT_ATTRS_CREATION_DATE)) {
        newAttr->append(ENT_ATTRS_CREATION_DATE, attr.getIntField(ENT_ATTRS_CREATION_DATE));
    }
    if (STR_FIELD(attr, ENT_ATTRS_ID) != "") {
        newAttr->append(ENT_ATTRS_ID, STR_FIELD(attr, ENT_ATTRS_ID));
    }
    if (smartAttrMatch(STR_FIELD(attr, ENT_ATTRS_NAME), STR_FIELD(attr, ENT_ATTRS_TYPE), STR_FIELD(attr, ENT_ATTRS_ID),
                       ca.name, ca.type, ca.getId())) {
        /* Attribute match: update value */
        newAttr->append(ENT_ATTRS_VALUE, ca.value);
        updated = true;

        /* It was an actual update? */
        if (!attr.hasField(ENT_ATTRS_VALUE) || STR_FIELD(attr, ENT_ATTRS_VALUE) != ca.value) {            
            *actualUpdate = true;
        }
    }
    else {
        /* Attribute doesn't match: value is included "as is" */
        if (attr.hasField(ENT_ATTRS_VALUE)) {
            newAttr->append(ENT_ATTRS_VALUE, STR_FIELD(attr, ENT_ATTRS_VALUE));                     
        }
    }

    /* In the case of actual update, we update also the modification date; otherwise we include the previous existing one */
    if (*actualUpdate) {
        newAttr->append(ENT_ATTRS_MODIFICATION_DATE, getCurrentTime());
    }
    else {
        /* The hasField() check is needed to preserve compatibility with entities that were created
         * in database by a CB instance previous to the support of creation and modification dates */
        if (attr.hasField(ENT_ATTRS_MODIFICATION_DATE)) {
            newAttr->append(ENT_ATTRS_MODIFICATION_DATE, attr.getIntField(ENT_ATTRS_MODIFICATION_DATE));
        }
    }

    return updated;
}

/* ****************************************************************************
*
* checkAndDelete -
*
* Add the 'attr' attribute to the BSONObjBuilder (which is passed by reference), doing
* the actual delete in the case of ContextAttribute matches.
*
* Returns true if the delete was done, false if only a "attribute copy" was done.
*/
static bool checkAndDelete (BSONObjBuilder* newAttr, BSONObj attr, ContextAttribute ca) {

    bool deleted = true;
    if (!smartAttrMatch(STR_FIELD(attr, ENT_ATTRS_NAME), STR_FIELD(attr, ENT_ATTRS_TYPE), STR_FIELD(attr, ENT_ATTRS_ID),
                        ca.name, ca.type, ca.getId())) {
        /* Attribute doesn't match: value is included "as is" */
        newAttr->append(ENT_ATTRS_NAME, STR_FIELD(attr, ENT_ATTRS_NAME));
        newAttr->append(ENT_ATTRS_TYPE, STR_FIELD(attr, ENT_ATTRS_TYPE));
        newAttr->append(ENT_ATTRS_VALUE, STR_FIELD(attr, ENT_ATTRS_VALUE));
        /* The hasField() check is needed to preserve compatibility with entities that were created
         * in database by a CB instance previous to the support of creation and modification dates */
        if (attr.hasField(ENT_ATTRS_CREATION_DATE)) {
            newAttr->append(ENT_ATTRS_CREATION_DATE, attr.getIntField(ENT_ATTRS_CREATION_DATE));
        }
        if (attr.hasField(ENT_ATTRS_MODIFICATION_DATE)) {
            newAttr->append(ENT_ATTRS_MODIFICATION_DATE, attr.getIntField(ENT_ATTRS_MODIFICATION_DATE));
        }
        string id = STR_FIELD(attr, ENT_ATTRS_ID);
        if (id != "") {
            newAttr->append(ENT_ATTRS_ID, STR_FIELD(attr, ENT_ATTRS_ID));
        }
        deleted = false;
    }

    return deleted;
}

/* ****************************************************************************
*
* updateAttribute -
*
* Returns true if an attribute was found and replaced, false otherwise. If true,
* the "actualUpdate" argument (passed by reference) is set to true in the case that the
* original value of the attribute was different that the one used in the update (this is
* important for ONCHANGE notifications)
*
*/
static bool updateAttribute(BSONObj* attrs, BSONObj* newAttrs, ContextAttribute* caP, bool* actualUpdate) {

    BSONArrayBuilder newAttrsBuilder;
    *actualUpdate = false;
    bool updated = false;
    for( BSONObj::iterator i = attrs->begin(); i.more(); ) {

        BSONObjBuilder newAttr;
        bool unitActualUpdate = false;
        if (checkAndUpdate(&newAttr, i.next().embeddedObject(), *caP, &unitActualUpdate) && !updated) {
            updated = true;
        }
        /* If at least one actual update was done at checkAndUpdate() level, then updateAttribute()
         * actual update is true */
        if (unitActualUpdate == true) {
            *actualUpdate = true;
        }

        newAttrsBuilder.append(newAttr.obj());
    }
    *newAttrs = newAttrsBuilder.arr();

    return updated;

}

/* ****************************************************************************
*
* appendAttribute -
*
* In the case of "actual append", it always return true
* In the case of "append as update", it returns true in the case of actual update,
* false othewise (i.e. when the new value equals to the existing one)
*
*/
static bool appendAttribute(BSONObj* attrs, BSONObj* newAttrs, ContextAttribute* caP) {

    /* In the current version (and until we close old issue 33)
     * APPEND with existing attribute equals to UPDATE */
    BSONArrayBuilder newAttrsBuilder;
    bool updated = false;
    bool actualUpdate = false;
    for( BSONObj::iterator i = attrs->begin(); i.more(); ) {

        BSONObjBuilder newAttr;
        bool attrActualUpdate;
        updated = checkAndUpdate(&newAttr, i.next().embeddedObject(), *caP, &attrActualUpdate) || updated;
        actualUpdate = attrActualUpdate || actualUpdate;
        newAttrsBuilder.append(newAttr.obj());
    }

    /* If not updated, then append */
    if (!updated) {
        BSONObjBuilder newAttr;
        newAttr.append(ENT_ATTRS_NAME, caP->name);
        newAttr.append(ENT_ATTRS_TYPE, caP->type);
        newAttr.append(ENT_ATTRS_VALUE, caP->value);
        if (caP->getId() != "") {
            newAttr.append(ENT_ATTRS_ID, caP->getId());
        }
        int now = getCurrentTime();
        newAttr.append(ENT_ATTRS_CREATION_DATE, now);
        newAttr.append(ENT_ATTRS_MODIFICATION_DATE, now);
        newAttrsBuilder.append(newAttr.obj());

        *newAttrs = newAttrsBuilder.arr();
        return true;
    }
    else {
        *newAttrs = newAttrsBuilder.arr();
        return actualUpdate;
    }


}

/* ****************************************************************************
*
* legalIdUsage -
*
* Check that the client is not trying to mix attributes ID and no ID for the same
* name
*
*/
static bool legalIdUsage(BSONObj* attrs, ContextAttribute* caP) {

    if (caP->getId() == "") {
        /* Attribute attempting to append hasn't ID. Thus, no attribute with same name can have ID in attrs */
        for( BSONObj::iterator i = attrs->begin(); i.more(); ) {
            BSONObj attr = i.next().embeddedObject();
            if (STR_FIELD(attr, ENT_ATTRS_NAME) == caP->name && STR_FIELD(attr, ENT_ATTRS_TYPE) == caP->type && STR_FIELD(attr, ENT_ATTRS_ID) != "") {
                return false;
            }
        }
        return true;
    }
    else {
        /* Attribute attempting to append has ID. Thus, no attribute with same name cannot have ID in attrs */
        for( BSONObj::iterator i = attrs->begin(); i.more(); ) {
            BSONObj attr = i.next().embeddedObject();
            if (STR_FIELD(attr, ENT_ATTRS_NAME) == caP->name && STR_FIELD(attr, ENT_ATTRS_TYPE) == caP->type && STR_FIELD(attr, ENT_ATTRS_ID) == "") {
                return false;
            }
        }
        return true;
    }
}

/* ****************************************************************************
*
* legalIdUsage -
*
* Check that the client is not trying to mix attributes ID and no ID for the same
* name
*
*/
static bool legalIdUsage(ContextAttributeVector caV) {

    for (unsigned int ix = 0; ix < caV.size(); ++ix) {
        std::string attrName = caV.get(ix)->name;
        std::string attrType = caV.get(ix)->type;
        std::string attrId = caV.get(ix)->getId();
        if (attrId == "") {
            /* Search for attribute with same name and type, but with actual ID to detect inconsistency */
            for (unsigned int jx = 0; jx < caV.size(); ++jx) {
                ContextAttribute* ca = caV.get(jx);
                if (attrName == ca->name && attrType == ca->type && ca->getId() != "") {
                    return false;
                }
            }
        }
    }

    return true;

}


/* ****************************************************************************
*
* deleteAttribute -
*
* Returns true if an attribute was deleted, false otherwise
*
*/
static bool deleteAttribute(BSONObj* attrs, BSONObj* newAttrs, ContextAttribute* caP) {
    BSONArrayBuilder newAttrsBuilder;
    bool deleted = false;
    for( BSONObj::iterator i = attrs->begin(); i.more(); ) {

        BSONObjBuilder newAttr;
        if (checkAndDelete(&newAttr, i.next().embeddedObject(), *caP) && !deleted) {
            deleted = true;
        }
        else {
            newAttrsBuilder.append(newAttr.obj());
        }
    }
    *newAttrs = newAttrsBuilder.arr();

    return deleted;
}

/* ****************************************************************************
*
* addTriggeredSubscriptions
*
*/
static bool addTriggeredSubscriptions(std::string entityId, std::string entityType, std::string attr, map<string, BSONObj*>* subs, std::string* err) {

    DBClientConnection* connection = getMongoConnection();

    /* Build query */
    std::string entIdQ       = std::string(CSUB_ENTITIES)   + "." + CSUB_ENTITY_ID;
    std::string entTypeQ     = std::string(CSUB_ENTITIES)   + "." + CSUB_ENTITY_TYPE;
    std::string entPatternQ  = std::string(CSUB_ENTITIES)   + "." + CSUB_ENTITY_ISPATTERN;
    std::string condTypeQ    = std::string(CSUB_CONDITIONS) + "." + CSUB_CONDITIONS_TYPE;
    std::string condValueQ   = std::string(CSUB_CONDITIONS) + "." + CSUB_CONDITIONS_VALUE;

    /* Note the $or on entityType, to take into account matching in subscriptions with no entity type */
    BSONObj queryNoPattern = BSON(
                entIdQ << entityId <<
                "$or" << BSON_ARRAY(
                    BSON(entTypeQ << entityType) <<
                    BSON(entTypeQ << BSON("$exists" << false))) <<
                entPatternQ << "false" <<
                condTypeQ << ON_CHANGE_CONDITION <<
                condValueQ << attr <<
                CSUB_EXPIRATION << BSON("$gt" << (long long) getCurrentTime())
                );

    /* This is JavaScript code that runs in MongoDB engine. As far as I know, this is the only
     * way to do a "reverse regex" query in MongoDB (see
     * http://stackoverflow.com/questions/15966991/mongodb-reverse-regex/15989520).
     * Note that although we are using a isPattern=true in the MongoDB query besides $where, we
     * also need to check that in the if statement in the JavaScript function given that a given
     * sub document could include both isPattern=true and isPattern=false documents */
    std::string function = std::string("function()") +
         "{" +
            "for (var i=0; i < this."+CSUB_ENTITIES+".length; i++) {" +
                "if (this."+CSUB_ENTITIES+"[i]."+CSUB_ENTITY_ISPATTERN+" == \"true\" && " +
                    "(this."+CSUB_ENTITIES+"[i]."+CSUB_ENTITY_TYPE+" == \""+entityType+"\" || " +
                        "this."+CSUB_ENTITIES+"[i]."+CSUB_ENTITY_TYPE+" == \"\" || " +
                        "!(\""+CSUB_ENTITY_TYPE+"\" in this."+CSUB_ENTITIES+"[i])) && " +
                    "\""+entityId+"\".match(this."+CSUB_ENTITIES+"[i]."+CSUB_ENTITY_ID+")) {" +
                    "return true; " +
                "}" +
            "}" +
            "return false; " +
         "}";
    LM_T(LmtMongo, ("JS function: %s", function.c_str()));

    BSONObjBuilder queryPattern;
    queryPattern.append(entPatternQ, "true");
    queryPattern.append(condTypeQ, ON_CHANGE_CONDITION);
    queryPattern.append(condValueQ, attr);
    queryPattern.append(CSUB_EXPIRATION, BSON("$gt" << (long long) getCurrentTime()));
    queryPattern.appendCode("$where", function);

    // FIXME: the condTypeQ and condValudeQ part can be "factorized" out of the $or clause
    BSONObj query = BSON("$or" << BSON_ARRAY(queryNoPattern << queryPattern.obj()));

    /* Do the query */
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
        mongoSemGive(__FUNCTION__, "query in SubscribeContextCollection (DBException)");
        *err = std::string("collection: ") + getSubscribeContextCollectionName() +
               " - query(): " + query.toString() +
               " - exception: " + e.what();
        return false;
    }
    catch(...) {
        mongoSemGive(__FUNCTION__, "query in SubscribeContextCollection (Generic Exception)");
        *err = std::string("collection: ") + getSubscribeContextCollectionName() +
               " - query(): " + query.toString() +
               " - exception: " + "generic";
        return false;
    }

    /* For each one of the subscriptions found, add it to the map (if not already there) */
    while (cursor->more()) {

        BSONObj sub = cursor->next();
        std::string subIdStr = sub.getField("_id").OID().str();

        if (subs->count(subIdStr) == 0) {
            LM_T(LmtMongo, ("adding subscription: '%s'", sub.toString().c_str()));
            // FIXME P8: see old issues #90
            // subs->insert(std::pair<string, BSONObj*>(subIdStr, new BSONObj(sub)));

            subs->insert(std::pair<string, BSONObj*>(subIdStr, NULL));
        }
    }

    return true;

}

/* ****************************************************************************
*
* processSubscriptions
*
*/
static bool processSubscriptions(EntityId en, map<string, BSONObj*>* subs, std::string* err) {

    DBClientConnection* connection = getMongoConnection();

    /* For each one of the subscriptions in the map, send notification */
    for (std::map<string, BSONObj*>::iterator it = subs->begin(); it != subs->end(); ++it) {

        //FIXME P8: see old issue #90
        //BSONObj sub = *(it->second);

        std::string  mapSubId = it->first;
        BSONObj      sub;

        try
        {
           mongoSemTake(__FUNCTION__, "findOne in SubscribeContextCollection");
           sub = connection->findOne(getSubscribeContextCollectionName(), BSON("_id" << OID(mapSubId)));
           mongoSemGive(__FUNCTION__, "findOne in SubscribeContextCollection");
        }
        catch (...)
        {
           mongoSemGive(__FUNCTION__, "findOne in SubscribeContextCollection (mongo generic exception)");
           LM_E(("Got an exception during findOne of collection '%s'", getSubscribeContextCollectionName()));

           *err = std::string("collection: ") + getEntitiesCollectionName();
           return false;
        }

        LM_T(LmtMongo, ("retrieved document: '%s'", sub.toString().c_str()));
        OID subId = sub.getField("_id").OID();

        /* Check that throttling is not blocking notication */
        if (sub.hasField(CSUB_THROTTLING) && sub.hasField(CSUB_LASTNOTIFICATION)) {
            long long current = getCurrentTime();
            long long sinceLastNotification = current - sub.getIntField(CSUB_LASTNOTIFICATION);
            if (sub.getField(CSUB_THROTTLING).numberLong() > sinceLastNotification) {
                LM_T(LmtMongo, ("blocked due to throttling, current time is: %l", current));
                continue;
            }
        }

        /* Build entities vector */
        EntityIdVector enV;
        enV.push_back(new EntityId(en.id, en.type, en.isPattern));

        /* Build attribute list vector */
        AttributeList attrL = subToAttributeList(sub);

        /* Get format. If not found in the csubs document (it could happen in the case of updating Orion using an existing database) we use XML */
        Format format = sub.hasField(CSUB_FORMAT) ? stringToFormat(STR_FIELD(sub, CSUB_FORMAT)) : XML;

        /* Send notification */
        if (processOnChangeCondition(enV, attrL, NULL,
                                     subId.str(),
                                     STR_FIELD(sub, CSUB_REFERENCE),
                                     format)) {

            BSONObj query = BSON("_id" << subId);
            BSONObj update = BSON("$set" << BSON(CSUB_LASTNOTIFICATION << getCurrentTime()) << "$inc" << BSON(CSUB_COUNT << 1));
            try {
                LM_T(LmtMongo, ("update() in '%s' collection: {%s, %s}", getSubscribeContextCollectionName(),
                                query.toString().c_str(),
                                update.toString().c_str()));

                mongoSemTake(__FUNCTION__, "update in SubscribeContextCollection");
                connection->update(getSubscribeContextCollectionName(), query, update);
                mongoSemGive(__FUNCTION__, "update in SubscribeContextCollection");
            }
            catch( const DBException &e ) {
                mongoSemGive(__FUNCTION__, "update in SubscribeContextCollection (mongo db exception)");
                *err = std::string("collection: ") + getEntitiesCollectionName() +
                       " - query(): " + query.toString() + " - update(): " + update.toString() + " - exception: " + e.what();
                enV.release();
                attrL.release();
                return false;
            }
            catch(...) {
                mongoSemGive(__FUNCTION__, "update in SubscribeContextCollection (mongo generic exception)");
                *err = std::string("collection: ") + getEntitiesCollectionName() +
                       " - query(): " + query.toString() + " - update(): " + update.toString() + " - exception: " + "generic";
                enV.release();
                attrL.release();
                return false;
            }
        }

        /* Release object created dynamically (including the value in the map created by
         * addTriggeredSubscriptions */
        attrL.release();
        enV.release();
        // delete it->second; FIXME P8: see old issue #90
    }

    return true;
}

/* ****************************************************************************
*
* buildGeneralErrorReponse -
*
*/
static void buildGeneralErrorReponse(ContextElement* ceP, ContextAttribute* ca, UpdateContextResponse* responseP, HttpStatusCode code, std::string details = "") {

    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->contextElement.entityId = ceP->entityId;
    if (ca != NULL) {
        cerP->contextElement.contextAttributeVector.push_back(ca);
    }
    cerP->statusCode.fill(code, details);
    responseP->contextElementResponseVector.push_back(cerP);

}

/* ****************************************************************************
*
* processContextAttributeVector -
*
* Returns true if entity was actually modified, false otherwise (including fail cases)
*
*/
static bool processContextAttributeVector (ContextElement* ceP, std::string action, std::map<string, BSONObj*>* subsToNotify, BSONObj* attrs, BSONObj* newAttrs, ContextElementResponse* cerP, UpdateContextResponse* responseP) {

    EntityId*   eP         = &cerP->contextElement.entityId;
    std::string entityId   = cerP->contextElement.entityId.id;
    std::string entityType = cerP->contextElement.entityId.type;

    bool entityModified = false;

    for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix) {

        /* No matter if success or fail, we have to include the attribute in the response */

        // FIXME P4: ContextAttribute has a constructor with ContextAttribute* as input
        //           This constructor might be useful here ...

        ContextAttribute* ca          = new ContextAttribute();
        ContextAttribute* attributeP  = ceP->contextAttributeVector.get(ix);
        ca->name = attributeP->name;
        ca->type = attributeP->type;

        if (attributeP->compoundValueP != NULL)
        {
          LM_W(("This context attribute has a COMPOUND VALUE - special care is needed (compoundValueP at %p)", attributeP->compoundValueP));
          attributeP->compoundValueP->shortShow("processContextAttributeVector: ");
        }

        if (attributeP->getId() != "") {
            Metadata*  md = new Metadata(NGSI_MD_ID, "string", attributeP->getId());
            ca->metadataVector.push_back(md);
        }
        cerP->contextElement.contextAttributeVector.push_back(ca);

        /* actualUpdate could be changed to false in the "update" case. For "delete" and
         * "append" it would keep the true value untouched */
        bool actualUpdate = true;
        if (strcasecmp(action.c_str(), "update") == 0) {
            if (updateAttribute(attrs, newAttrs, attributeP, &actualUpdate)) {
                entityModified = actualUpdate || entityModified;
                *attrs = *newAttrs;                
            }
            else {
                ContextAttribute* aP = attributeP;

                /* If updateAttribute() returns false, then that particular attribute has not
                 * been found. In this case, we interrupt the processing an early return with
                 * an error StatusCode */
                cerP->statusCode.fill(SccInvalidParameter, 
                                      std::string("action: UPDATE") + 
                                      std::string(" - entity: (") + eP->toString() + ")" +
                                      std::string(" - offending attribute: ") + aP->toString());

                responseP->contextElementResponseVector.push_back(cerP);
                return false;

            }
        }
        else if (strcasecmp(action.c_str(), "append") == 0) {
            if (legalIdUsage(attrs, attributeP)) {
                entityModified = appendAttribute(attrs, newAttrs, attributeP) || entityModified;
                *attrs = *newAttrs;
            }
            else {
                ContextAttribute* aP = ceP->contextAttributeVector.get(ix);

                /* If legalIdUsage() returns false, then that particular attribute can not be appended. In this case,
                 * we interrupt the processing an early return with
                 * a error StatusCode */
                cerP->statusCode.fill(SccInvalidParameter,
                                      std::string("action: APPEND") +
                                      " - entity: (" + eP->toString() + ")" +
                                      " - offending attribute: " + aP->toString());

                responseP->contextElementResponseVector.push_back(cerP);
                return false;
            }
        }
        else if (strcasecmp(action.c_str(), "delete") == 0) {
            if (deleteAttribute(attrs, newAttrs, ceP->contextAttributeVector.get(ix))) {
                entityModified = true;
                *attrs = *newAttrs;
            }
            else {
                ContextAttribute* aP = ceP->contextAttributeVector.get(ix);
                /* If deleteAttribute() returns false, then that particular attribute has not
                 * been found. In this case, we interrupt the processing an early return with
                 * a error StatusCode */
                cerP->statusCode.fill(SccInvalidParameter,
                                      std::string("action: DELETE") +
                                      " - entity: (" + eP->toString() + ")" +
                                      " - offending attribute: " + aP->toString());

                responseP->contextElementResponseVector.push_back(cerP);
                return false;

            }
        }
        else {
            LM_RE(false, ("Unknown updateContext action '%s'. This is a bug in the parsing layer checking!", action.c_str()));
        }

        /* Add those ONCHANGE subscription triggered by the just processed attribute. Note that
         * actualUpdate is always true in the case of  "delete" or "append", so the if statement
         * is "bypassed" */
        if (actualUpdate) {
            std::string err;
            if (!addTriggeredSubscriptions(entityId, entityType, ca->name, subsToNotify, &err)) {
                cerP->statusCode.fill(SccReceiverInternalError, err);
                responseP->contextElementResponseVector.push_back(cerP);
                LM_RE(false, (err.c_str()));
            }
        }

#if 0
        /* DEBUG (see old issue #90) */
        int ix = 0;
        for (std::map<string, BSONObj*>::iterator it = subsToNotify.begin(); it != subsToNotify.end(); ++it) {
            BSONObj b = *(it->second);
            if (b.isEmpty()) {
                LM_T(LmtMongo, ("DEBUG afterTriggeredsubs [%d]: <empty>", ix));
            }
            else {
                LM_T(LmtMongo, ("DEBUG afterTriggeredsubs [%d]: %s", ix, b.toString().c_str()));
            }
            ix++;
        }
#endif

    }

    if (!entityModified) {
        /* In this case, there wasn't any failure, but ceP was not set. We need to do it ourselves, as the function caller will
         * do a 'continue' without setting it. */
        //FIXME P5: this is ugly, our code should be improved to set ceP in a common place for the "happy case"
        cerP->statusCode.fill(SccOk);
        responseP->contextElementResponseVector.push_back(cerP);
    }

    return entityModified;
}

/* ****************************************************************************
*
* createEntity -
*
*/
static bool createEntity(EntityId e, ContextAttributeVector attrsV, std::string* errDetail) {

    DBClientConnection* connection = getMongoConnection();

    LM_T(LmtMongo, ("Entity not found in '%s' collection, creating it", getEntitiesCollectionName()));

    if (!legalIdUsage(attrsV)) {
        *errDetail = "Attributes with same name with ID and not ID at the same time in the same entity are forbidden: entity: (" + e.toString() + ")";
        return false;
    }

    int now = getCurrentTime();
    BSONArrayBuilder attrsToAdd;
    for (unsigned int ix = 0; ix < attrsV.size(); ++ix) {
        std::string attrId = attrsV.get(ix)->getId();        

        BSONObjBuilder bsonAttr;
        bsonAttr.appendElements(BSON(ENT_ATTRS_NAME << attrsV.get(ix)->name <<
                                     ENT_ATTRS_TYPE << attrsV.get(ix)->type <<
                                     ENT_ATTRS_VALUE << attrsV.get(ix)->value <<
                                     ENT_ATTRS_CREATION_DATE << now <<
                                     ENT_ATTRS_MODIFICATION_DATE << now));
        if (attrId.length() == 0) {
            LM_T(LmtMongo, ("new attribute: {name: %s, type: %s, value: %s}",
                            attrsV.get(ix)->name.c_str(),
                            attrsV.get(ix)->type.c_str(),
                            attrsV.get(ix)->value.c_str()));
        }
        else {
            bsonAttr.append(ENT_ATTRS_ID, attrId);
            LM_T(LmtMongo, ("new attribute: {name: %s, type: %s, value: %s, id: %s}",
                            attrsV.get(ix)->name.c_str(),
                            attrsV.get(ix)->type.c_str(),
                            attrsV.get(ix)->value.c_str(),
                            attrId.c_str()));
        }
        attrsToAdd.append(bsonAttr.obj());

    }

    BSONObj bsonId = e.type == "" ? BSON(ENT_ENTITY_ID << e.id) : BSON(ENT_ENTITY_ID << e.id << ENT_ENTITY_TYPE << e.type);
    BSONObj insertedDoc = BSON("_id" << bsonId <<
                               ENT_ATTRS << attrsToAdd.arr() <<
                               ENT_CREATION_DATE << now <<
                               ENT_MODIFICATION_DATE << now);

    try {
        LM_T(LmtMongo, ("insert() in '%s' collection: '%s'", getEntitiesCollectionName(), insertedDoc.toString().c_str()));
        mongoSemTake(__FUNCTION__, "insert into EntitiesCollection");
        connection->insert(getEntitiesCollectionName(), insertedDoc);
        mongoSemGive(__FUNCTION__, "insert into EntitiesCollection");
    }
    catch( const DBException &e ) {
        mongoSemGive(__FUNCTION__, "insert into EntitiesCollection (mongo db exception)");
        *errDetail = std::string("Database Error: collection: ") + getEntitiesCollectionName() +
                " - insert(): " + insertedDoc.toString() +
                " - exception: " + e.what();

        return false;
    }
    catch(...) {
        mongoSemGive(__FUNCTION__, "insert into EntitiesCollection (mongo generic exception)");
        *errDetail = std::string("Database Error: collection: ") + getEntitiesCollectionName() +
                " - insert(): " + insertedDoc.toString() +
                " - exception: " + "generic";

        return false;
    }

    return true;
}

/* ****************************************************************************
*
* processContextElement -
*
*/
void processContextElement(ContextElement* ceP, UpdateContextResponse* responseP, std::string action) {

    DBClientConnection* connection = getMongoConnection();

    /* Getting the entity in the request (helpful in other places) */
    EntityId en = ceP->entityId;

    /* Not supporting isPattern = true currently */
    if (isTrue(en.isPattern)) {
        buildGeneralErrorReponse(ceP, NULL, responseP, SccNotImplemented);
        return;
    }

    /* Check that UPDATE or APPEND is not used with attributes with empty value */
    if (strcasecmp(action.c_str(), "update") == 0 || strcasecmp(action.c_str(), "append") == 0) {
        for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix) {
            if (ceP->contextAttributeVector.get(ix)->value.size() == 0) {

                ContextAttribute* aP = ceP->contextAttributeVector.get(ix);
                ContextAttribute* ca = new ContextAttribute(aP);

                buildGeneralErrorReponse(ceP, ca, responseP, SccInvalidParameter,                                   
                                   std::string("action: ") + action +
                                      " - entity: (" + en.toString(true) + ")" +
                                      " - offending attribute: " + aP->toString());
                return;
            }
        }
    }

    /* Find entities (could be several ones in the case of no type or isPattern=true) */
    const std::string idString = std::string("_id.") + ENT_ENTITY_ID;
    const std::string typeString = std::string("_id.") + ENT_ENTITY_TYPE;
    BSONObj query;
    if (en.type == "") {
        query = BSON(idString << en.id);
    }
    else {
        query = BSON(idString << en.id << typeString << en.type);
    }
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
        buildGeneralErrorReponse(ceP, NULL, responseP, SccReceiverInternalError,                           
                           std::string("collection: ") + getEntitiesCollectionName() +
                              " - query(): " + query.toString() +
                              " - exception: " + e.what());
        return;
    }
    catch(...) {
        mongoSemGive(__FUNCTION__, "query in EntitiesCollection (mongo generic exception)");
        buildGeneralErrorReponse(ceP, NULL, responseP, SccReceiverInternalError,                           
                           std::string("collection: ") + getEntitiesCollectionName() +
                              " - query(): " + query.toString() +
                              " - exception: " + "generic");
        return;
    }

    bool atLeastOneResult = false;
    while (cursor->more()) {
        BSONObj r = cursor->next();
        LM_T(LmtMongo, ("retrieved document: '%s'", r.toString().c_str()));

        std::string entityId = STR_FIELD(r.getField("_id").embeddedObject(), ENT_ENTITY_ID);
        std::string entityType = STR_FIELD(r.getField("_id").embeddedObject(), ENT_ENTITY_TYPE);

        atLeastOneResult = true;

        ContextElementResponse* cerP = new ContextElementResponse();
        cerP->contextElement.entityId.fill(entityId, entityType, "false");

        /* We start with the attrs array in the entity document, which is manipulated by the
         * {update|delete|append}Attrsr() function for each one of the attributes in the
         * contextElement being processed. Then, we replace the resulting attrs array in the
         * BSON document and update the entity document. It would be more efficient to map the
         * entiry attrs to something like and hash map and manipulate there, but it is not seems
         * easy using the Mongod driver BSON API. Note that we need to use newAttrs given that attrs is
         * BSONObj, which is an inmutable type. FIXME P6: try to improve this */
        BSONObj attrs = r.getField(ENT_ATTRS).embeddedObject();
        BSONObj newAttrs;

        /* We accumulate the subscriptions in a map. The key of the map is the string representing
         * subscription id */
        std::map<string, BSONObj*> subsToNotify;

        if (!processContextAttributeVector(ceP, action, &subsToNotify, &attrs, &newAttrs, cerP, responseP)) {
            /* The entity wasn't actually modified, so we don't need to update it and we can continue with next one */
            continue;
        }

        /* Now that newAttrs containts the final status of the attributes after processing the whole
         * list of attributes in the ContextElement, update entity attributes in database */
        BSONObjBuilder updatedEntityBuilder;
        updatedEntityBuilder.appendArray(ENT_ATTRS, attrs);
        updatedEntityBuilder.append(ENT_MODIFICATION_DATE, getCurrentTime());
        /* We use $set to avoid losing the creation date field */
        BSONObj updatedEntity = BSON("$set" << updatedEntityBuilder.obj());
        /* Note that the query that we build for updating is slighty different than the query used
         * for selecting the entities to process. In particular, the "no type" branch in the if
         * sentence selects precisely the entity with no type, using the {$exists: false} clause */
        BSONObj query;
        if (entityType == "") {
            query = BSON(idString << entityId << typeString << BSON("$exists" << false));
        }
        else {
            query = BSON(idString << entityId << typeString << entityType);
        }
        try {
            LM_T(LmtMongo, ("update() in '%s' collection: {%s, %s}", getEntitiesCollectionName(),
                               query.toString().c_str(),
                               updatedEntity.toString().c_str()));
            mongoSemTake(__FUNCTION__, "update in EntitiesCollection");
            connection->update(getEntitiesCollectionName(), query, updatedEntity);
            mongoSemGive(__FUNCTION__, "update in EntitiesCollection");
        }
        catch( const DBException &e ) {
            mongoSemGive(__FUNCTION__, "update in EntitiesCollection (mongo db exception)");
            cerP->statusCode.fill(SccReceiverInternalError,
               std::string("collection: ") + getEntitiesCollectionName() +
               " - update() query: " + query.toString() +
               " - update() doc: " + updatedEntity.toString() +
               " - exception: " + e.what());

            responseP->contextElementResponseVector.push_back(cerP);
            return;
        }
        catch(...) {
            mongoSemGive(__FUNCTION__, "update in EntitiesCollection (mongo generic exception)");
            cerP->statusCode.fill(SccReceiverInternalError,
               std::string("collection: ") + getEntitiesCollectionName() +
               " - update() query: " + query.toString() +
               " - update() doc: " + updatedEntity.toString() +
               " - exception: " + "generic");

            responseP->contextElementResponseVector.push_back(cerP);
            return;
        }

#if 0
        /* DEBUG (see issue #90) */
        int ix = 0;
        for (std::map<string, BSONObj*>::iterator it = subsToNotify.begin(); it != subsToNotify.end(); ++it) {
            BSONObj b = *(it->second);
            if (b.isEmpty()) {
                LM_T(LmtMongo, ("DEBUG before addTriggeredSubscriptions [%d]: <empty>", ix));
            }
            else {
                LM_T(LmtMongo, ("DEBUG before addTriggeredSubscriptions [%d]: %s", ix, b.toString().c_str() ));
            }
            ix++;
        }
#endif

        /* Send notifications for each one of the ONCHANGE subscriptions accumulated by
         * previous addTriggeredSubscriptions() invocations */
        std::string err;
        processSubscriptions(en, &subsToNotify, &err);

        /* To finish with this entity processing, add the corresponding ContextElementResponse to
         * the global response */
        cerP->statusCode.fill(SccOk);
        responseP->contextElementResponseVector.push_back(cerP);

    }

    /* If the entity didn't already exist, we create it. Note that alternatively, we could do a count()
     * before the query() to check this. However this would add a second interaction with MongoDB */
    if (!atLeastOneResult) {

        if (strcasecmp(action.c_str(), "append") != 0) {
            /* Only APPEND can create entities, thus error is returned in UPDATE or DELETE cases */
            buildGeneralErrorReponse(ceP, NULL, responseP, SccContextElementNotFound, en.id);
        }
        else {            

            /* Creating the part of the response that doesn't depend on success or failure */
            ContextElementResponse* cerP = new ContextElementResponse();
            cerP->contextElement.entityId.fill(en.id, en.type, "false");
            for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix) {
                ContextAttribute* caP = ceP->contextAttributeVector.get(ix);
                ContextAttribute* ca = new ContextAttribute(caP->name, caP->type);

                if (caP->getId().length() != 0) {
                    Metadata* md = new Metadata(NGSI_MD_ID, "string", caP->getId());
                    ca->metadataVector.push_back(md);
                }

                cerP->contextElement.contextAttributeVector.push_back(ca);
            }

            std::string errReason, errDetail;
            if (!createEntity(en, ceP->contextAttributeVector, &errDetail)) {
               cerP->statusCode.fill(SccInvalidParameter, errDetail);
            }
            else {
               cerP->statusCode.fill(SccOk);

                /* Successful creation: send potential notifications */
                std::map<string, BSONObj*> subsToNotify;
                for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix) {
                    std::string err;
                    if (!addTriggeredSubscriptions(en.id, en.type, ceP->contextAttributeVector.get(ix)->name, &subsToNotify, &err)) {
                        cerP->statusCode.fill(SccReceiverInternalError, err);
                        responseP->contextElementResponseVector.push_back(cerP);
                        LM_RVE((err.c_str()));
                    }
                }
                processSubscriptions(en, &subsToNotify, &errReason);
            }
            responseP->contextElementResponseVector.push_back(cerP);
        }

    }

}
