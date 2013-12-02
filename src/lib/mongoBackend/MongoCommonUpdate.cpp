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
        return (name1 == name2 && id1 == id2);
    }
    else {
        return (name1 == name2 && type1 == type2 && id1 == id2);
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
*/
static void appendAttribute(BSONObj* attrs, BSONObj* newAttrs, ContextAttribute* caP) {

    /* In the current version (and until we close old issue 33)
     * APPEND with existing attribute equals to UPDATE */
    BSONArrayBuilder newAttrsBuilder;
    bool updated = false;
    for( BSONObj::iterator i = attrs->begin(); i.more(); ) {

        BSONObjBuilder newAttr;
        /* We need to use some bool variable to match the checkAndUpdate() signature, but in
         * appendAttribute() we don't use this information */
        bool actualUpdate;
        if (checkAndUpdate(&newAttr, i.next().embeddedObject(), *caP, &actualUpdate) && !updated) {
            updated = true;
        }
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
        newAttr.append(ENT_ATTRS_MODIFICATION_DATE, getCurrentTime());
        newAttrsBuilder.append(newAttr.obj());
    }
    *newAttrs = newAttrsBuilder.arr();

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
                CSUB_EXPIRATION << BSON("$gt" << getCurrentTime())
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
    queryPattern.append(CSUB_EXPIRATION, BSON("$gt" << getCurrentTime()));
    queryPattern.appendCode("$where", function);

    // FIXME: the condTypeQ and condValudeQ part can be "factorized" out of the $or clause
    BSONObj query = BSON("$or" << BSON_ARRAY(queryNoPattern << queryPattern.obj()));

    /* Do the query */
    auto_ptr<DBClientCursor> cursor;
    try {
        LM_T(LmtMongo, ("query() in '%s' collection: '%s'", getSubscribeContextCollectionName(), query.toString().c_str()));
        cursor = connection->query(getSubscribeContextCollectionName(), query);
        /* We have observed that in some cases of DB errors (e.g. the database daemon is down) instead of
         * raising an exceiption the query() method set the cursos to NULL. In this case, we raise the
         * exception ourselves */
        if (cursor.get() == NULL) {
            throw DBException("Null cursor", 0);
        }
    }
    catch( const DBException &e ) {
        *err = std::string("collection: ") + getSubscribeContextCollectionName() +
               " - query(): " + query.toString() +
               " - exception: " + e.what();
        return false;
    }

    /* For each one of the subscriptions found, add it to the map (if not already there) */
    while (cursor->more()) {

        BSONObj sub = cursor->next();
        std::string subIdStr = sub.getField("_id").OID().str();

        if (subs->count(subIdStr) == 0) {
            LM_T(LmtMongo, ("adding subscription: '%s'", sub.toString().c_str()));
            //FIXME P8: see old issues #90
            //subs->insert(std::pair<string, BSONObj*>(subIdStr, new BSONObj(sub)));
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
        std::string mapSubId = it->first;
        BSONObj sub = connection->findOne(getSubscribeContextCollectionName(), BSON("_id" << OID(mapSubId)));
        LM_T(LmtMongo, ("retrieved document: '%s'", sub.toString().c_str()));
        OID subId = sub.getField("_id").OID();

        /* Check that throttling is not blocking notication */
        if (sub.hasField(CSUB_THROTTLING) && sub.hasField(CSUB_LASTNOTIFICATION)) {
            int current = getCurrentTime();
            int sinceLastNotification = current - sub.getIntField(CSUB_LASTNOTIFICATION);
            if (sub.getIntField(CSUB_THROTTLING) > sinceLastNotification) {
                LM_T(LmtMongo, ("blocked due to throttling, current time is: %d", current));
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

                connection->update(getSubscribeContextCollectionName(), query, update);
            }
            catch( const DBException &e ) {
                *err = std::string("collection: ") + getEntitiesCollectionName() +
                       " - query(): " + query.toString() + " - update(): " + update.toString() + " - exception: " + e.what();
                enV.release();
                attrL.release();
                return false;
            }
        }

        /* Release object created dynamically (including the value in the map created by
         * addTriggeredSubscriptions */
        attrL.release();
        enV.release();
        //delete it->second; FIXME P8: see old issue #90
    }

    return true;
}

/* ****************************************************************************
*
* buildGeneralErrorReponse -
*
*/
static void buildGeneralErrorReponse(ContextElement* ceP, ContextAttribute* ca, UpdateContextResponse* responseP, HttpStatusCode code, std::string reasonPhrase, std::string details = "") {

    ContextElementResponse* cerP = new ContextElementResponse();
    cerP->contextElement.entityId = ceP->entityId;
    if (ca != NULL) {
        cerP->contextElement.contextAttributeVector.push_back(ca);
    }
    cerP->statusCode.fill(code, reasonPhrase, details);
    responseP->contextElementResponseVector.push_back(cerP);

}

/* ****************************************************************************
*
* processContextAttributeVector -
*
* Returns true if processing was ok, false otherwise
*
*/
static bool processContextAttributeVector (ContextElement* ceP, std::string action, std::map<string, BSONObj*>* subsToNotify, BSONObj* attrs, BSONObj* newAttrs, ContextElementResponse* cerP, UpdateContextResponse* responseP) {

    std::string entityId = cerP->contextElement.entityId.id;
    std::string entityType = cerP->contextElement.entityId.type;

    for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix) {

        /* No matter if success or fail, we have to include the attribute in the response */
        ContextAttribute* ca = new ContextAttribute();
        ca->name = ceP->contextAttributeVector.get(ix)->name;
        ca->type = ceP->contextAttributeVector.get(ix)->type;
        if (ceP->contextAttributeVector.get(ix)->getId() != "") {
            Metadata*  md = new Metadata(METADATA_ID, "string", ceP->contextAttributeVector.get(ix)->getId());
            ca->metadataVector.push_back(md);
        }
        cerP->contextElement.contextAttributeVector.push_back(ca);

        /* actualUpdate could be changed to false in the "update" case. For "delete" and
         * "append" it would keep the true value untouched */
        bool actualUpdate = true;
        if (strcasecmp(action.c_str(), "update") == 0) {
            if (updateAttribute(attrs, newAttrs, ceP->contextAttributeVector.get(ix), &actualUpdate)) {
                *attrs = *newAttrs;
            }
            else {
                /* If updateAttribute() returns false, then that particular attribute has not
                 * been found. In this case, we interrupt the processing an early return with
                 * a error StatusCode */
                cerP->statusCode.fill(SccInvalidParameter,
                                      "Not Found Attribute in UPDATE",
                                      std::string("action: UPDATE") +
                                          // FIXME: use toString once EntityID and ContextAttribute becomes objects
                                          " - entity: (" + entityId + ", " + entityType + ")" +
                                          " - offending attribute: " + ceP->contextAttributeVector.get(ix)->name);
                responseP->contextElementResponseVector.push_back(cerP);
                return false;

            }
        }
        else if (strcasecmp(action.c_str(), "append") == 0) {
            if (legalIdUsage(attrs, ceP->contextAttributeVector.get(ix))) {
                appendAttribute(attrs, newAttrs, ceP->contextAttributeVector.get(ix));
                *attrs = *newAttrs;
            }
            else {
                /* If legalIdUsage() returns false, then that particular attribute can not be appended. In this case,
                 * we interrupt the processing an early return with
                 * a error StatusCode */
                cerP->statusCode.fill(SccInvalidParameter,
                                      "It is not allowed to APPEND an attribute with ID when another with the same name is in place or viceversa",
                                      std::string("action: APPEND") +
                                          // FIXME: use toString once EntityID and ContextAttribute becomes objects
                                          " - entity: (" + entityId + ", " + entityType + ")" +
                                          " - offending attribute: " + ceP->contextAttributeVector.get(ix)->name);
                responseP->contextElementResponseVector.push_back(cerP);
                return false;
            }
        }
        else if (strcasecmp(action.c_str(), "delete") == 0) {
            if (deleteAttribute(attrs, newAttrs, ceP->contextAttributeVector.get(ix))) {
                *attrs = *newAttrs;
            }
            else {
                /* If deleteAttribute() returns false, then that particular attribute has not
                 * been found. In this case, we interrupt the processing an early return with
                 * a error StatusCode */
                cerP->statusCode.fill(SccInvalidParameter,
                                      "Not Found Attribute in DELETE",
                                      std::string("action: DELETE") +
                                      // FIXME: use toString once EntityID and ContextAttribute becomes objects
                                         " - entity: (" + entityId + ", " + entityType + ")" +
                                         " - offending attribute: " + ceP->contextAttributeVector.get(ix)->name);
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
                cerP->statusCode.fill(SccReceiverInternalError, "Database Error", err );
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

    return true;
}

/* ****************************************************************************
*
* createEntity -
*
*/
static bool createEntity(EntityId e, ContextAttributeVector attrsV, std::string* errReason, std::string* errDetail) {

    DBClientConnection* connection = getMongoConnection();

    LM_T(LmtMongo, ("Entity not found in '%s' collection, creating it", getEntitiesCollectionName()));

    if (!legalIdUsage(attrsV)) {
        *errReason = "It is not allowed to create an entity with attribute of same name with ID an not ID";
        // FIXME: use toString once EntityID and ContextAttribute becomes objects
        *errDetail = "entity: (" + e.id + ", " + e.type + ")";
        return false;
    }

    BSONArrayBuilder attrsToAdd;
    for (unsigned int ix = 0; ix < attrsV.size(); ++ix) {
        std::string attrId = attrsV.get(ix)->getId();
        // FIXME P6: I don't like this approach, it is not DRY. It would be better to create the
        // base attribute, then append the last item (id) only in the case it is used)
        if (attrId.length() == 0) {
            attrsToAdd.append(BSON(ENT_ATTRS_NAME << attrsV.get(ix)->name <<
                                   ENT_ATTRS_TYPE << attrsV.get(ix)->type <<
                                   ENT_ATTRS_VALUE << attrsV.get(ix)->value));
            LM_T(LmtMongo, ("new attribute: {name: %s, type: %s, value: %s}",
                            attrsV.get(ix)->name.c_str(),
                            attrsV.get(ix)->type.c_str(),
                            attrsV.get(ix)->value.c_str()));
        }
        else {
            attrsToAdd.append(BSON(ENT_ATTRS_NAME << attrsV.get(ix)->name <<
                                    ENT_ATTRS_TYPE << attrsV.get(ix)->type <<
                                    ENT_ATTRS_VALUE << attrsV.get(ix)->value <<
                                    ENT_ATTRS_ID << attrId));
            LM_T(LmtMongo, ("new attribute: {name: %s, type: %s, value: %s, id: %s}",
                            attrsV.get(ix)->name.c_str(),
                            attrsV.get(ix)->type.c_str(),
                            attrsV.get(ix)->value.c_str(),
                            attrId.c_str()));
        }

    }
    BSONObj insertedDoc;
    if (e.type == "") {
        insertedDoc = BSON("_id" << BSON(ENT_ENTITY_ID << e.id) << ENT_ATTRS << attrsToAdd.arr() << ENT_CREATION_DATE << getCurrentTime());
    }
    else {
        insertedDoc = BSON("_id" << BSON(ENT_ENTITY_ID << e.id << ENT_ENTITY_TYPE << e.type) << ENT_ATTRS << attrsToAdd.arr() << ENT_CREATION_DATE << getCurrentTime());
    }
    try {
        LM_T(LmtMongo, ("insert() in '%s' collection: '%s'", getEntitiesCollectionName(), insertedDoc.toString().c_str()));
        connection->insert(getEntitiesCollectionName(), insertedDoc);
    }
    catch( const DBException &e ) {
        *errReason = "Database Error";
        *errDetail = std::string("collection: ") + getEntitiesCollectionName() +
                " - insert(): " + insertedDoc.toString() +
                " - exception: " + e.what();

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
        buildGeneralErrorReponse(ceP, NULL, responseP, SccNotImplemented, "Not Implemented");
        return;
    }

    /* Check that UPDATE or APPEND is not used with attributes with empty value */
    if (strcasecmp(action.c_str(), "update") == 0 || strcasecmp(action.c_str(), "append") == 0) {
        for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix) {
            if (ceP->contextAttributeVector.get(ix)->value.size() == 0) {

                ContextAttribute* ca = new ContextAttribute();
                ca->name = ceP->contextAttributeVector.get(ix)->name;
                ca->type = ceP->contextAttributeVector.get(ix)->type;
                buildGeneralErrorReponse(ceP, ca, responseP, SccInvalidParameter,
                                   "Empty Attribute in UPDATE or APPEND",
                                   std::string("action: ") + action +
                                      // FIXME: use toString once EntityID and ContextAttribute becomes objects
                                      " - entity: (" + en.id + ", " + en.type + ", " + en.isPattern + ")" +
                                      " - offending attribute: " + ceP->contextAttributeVector.get(ix)->name);
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
        cursor = connection->query(getEntitiesCollectionName(), query);
        /* We have observed that in some cases of DB errors (e.g. the database daemon is down) instead of
         * raising an exceiption the query() method set the cursos to NULL. In this case, we raise the
         * exception ourselves */
        if (cursor.get() == NULL) {
            throw DBException("Null cursor", 0);
        }
    }
    catch( const DBException &e ) {
        buildGeneralErrorReponse(ceP, NULL, responseP, SccReceiverInternalError,
                           "Database Error",
                           std::string("collection: ") + getEntitiesCollectionName() +
                              " - query(): " + query.toString() +
                              " - exception: " + e.what());
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
            /* Something was wrong processing attributes on this entity, continue with next one */
            continue;
        }

        /* Now that newAttrs containts the final status of the attributes after processing the whole
         * list of attributes in the ContextElement, update entity attributes in database */
        BSONObjBuilder updatedEntityBuilder;
        updatedEntityBuilder.appendArray(ENT_ATTRS, attrs);        
        updatedEntityBuilder.append(ENT_MODIFICATION_DATE, getCurrentTime());
        BSONObj updatedEntity = updatedEntityBuilder.obj();
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
            connection->update(getEntitiesCollectionName(), query, updatedEntity);
        }
        catch( const DBException &e ) {
            cerP->statusCode.fill(
                SccReceiverInternalError,
                "Database Error",
                std::string("collection: ") + getEntitiesCollectionName() +
                    " - update() query: " + query.toString() +
                    " - update() doc: " + updatedEntity.toString() +
                    " - exception: " + e.what());
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
        cerP->statusCode.fill(SccOk, "OK");
        responseP->contextElementResponseVector.push_back(cerP);

    }

    /* If the entity didn't already exist, we create it. Note that alternatively, we could do a count()
     * before the query() to check this and early return. However this would add a sencond interaction with MongoDB */
    if (!atLeastOneResult) {

        if (strcasecmp(action.c_str(), "delete") == 0) {
            buildGeneralErrorReponse(ceP, NULL, responseP, SccContextElementNotFound, "Entity Not Found", en.id);
        }
        else {            

            /* Creating the part of the response that doesn't depend on success or failure */
            ContextElementResponse* cerP = new ContextElementResponse();
            cerP->contextElement.entityId.fill(en.id, en.type, "false");
            for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix) {
                ContextAttribute* caP = ceP->contextAttributeVector.get(ix);
                ContextAttribute* ca = new ContextAttribute(caP->name, caP->type);

                if (caP->getId().length() != 0) {
                    Metadata* md = new Metadata(METADATA_ID, "string", caP->getId());
                    ca->metadataVector.push_back(md);
                }

                cerP->contextElement.contextAttributeVector.push_back(ca);
            }

            std::string errReason, errDetail;
            if (!createEntity(en, ceP->contextAttributeVector, &errReason, &errDetail)) {
                cerP->statusCode.fill(SccInvalidParameter, errReason, errDetail);
            }
            else {
                cerP->statusCode.fill(SccOk, "OK");

                /* Successfull creation: send potential notifications */
                std::map<string, BSONObj*> subsToNotify;
                for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix) {
                    std::string err;
                    if (!addTriggeredSubscriptions(en.id, en.type, ceP->contextAttributeVector.get(ix)->name, &subsToNotify, &err)) {
                        cerP->statusCode.fill(SccReceiverInternalError, "Database Error", err );
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
