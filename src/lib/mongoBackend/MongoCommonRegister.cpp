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

#include "MongoCommonRegister.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/string.h"
#include "common/globals.h"
#include "common/statistics.h"
#include "mongoBackend/MongoGlobal.h"


/*****************************************************************************
*
* processAssociations -
*
*/
static bool processAssociations(MetadataVector mdV, std::string* err) {

    // FIXME: first version of associations doesn't support association update

    DBClientConnection* connection = getMongoConnection();

    for (unsigned int ix = 0; ix < mdV.size(); ++ix) {

        Metadata* md = mdV.get(ix);
        if (md->type != "Association") {
            continue;
        }

        LM_T(LmtMongo, ("Processing association metadata"));

        std::string name      = md->name;
        std::string srcEnId   = md->association.entityAssociation.source.id;
        std::string srcEnType = md->association.entityAssociation.source.type;
        std::string tgtEnId   = md->association.entityAssociation.target.id;
        std::string tgtEnType = md->association.entityAssociation.target.type;

        BSONObj srcEn;
        if (srcEnType == "") {
            srcEn = BSON(ASSOC_ENT_ID << srcEnId);
        }
        else {
            srcEn = BSON(ASSOC_ENT_ID << srcEnId << ASSOC_ENT_TYPE << srcEnType);
        }

        BSONObj tgtEn;
        if (tgtEnType == "") {
            tgtEn = BSON(ASSOC_ENT_ID << tgtEnId);
        }
        else {
            tgtEn = BSON(ASSOC_ENT_ID << tgtEnId << ASSOC_ENT_TYPE << tgtEnType);
        }

        BSONArrayBuilder attrs;
        for (unsigned int jx = 0; jx < md->association.attributeAssociationList.size(); ++jx) {
            std::string srcAtt = md->association.attributeAssociationList.get(jx)->source;
            std::string tgtAtt = md->association.attributeAssociationList.get(jx)->target;
            attrs.append(BSON(ASSOC_ATTRS_SOURCE << srcAtt << ASSOC_ATTRS_TARGET << tgtAtt));
        }

        BSONObj doc = BSON("_id" << name << ASSOC_SOURCE_ENT << srcEn << ASSOC_TARGET_ENT << tgtEn << ASSOC_ATTRS << attrs.arr());
        try {
            LM_T(LmtMongo, ("insert() in '%s' collection: '%s'", getAssociationsCollectionName(), doc.toString().c_str()));
            connection->insert(getAssociationsCollectionName(), doc);
        }
        catch( const DBException &e ) {
            *err = e.what();
            LM_RE(false,("Database error '%s'", err->c_str()));
        }
    }
    return true;
}

/* ****************************************************************************
*
* processSubscriptions
*
* FIXME: this function is pretty similar to the one with the same name in
* mongoUpdateContext.cpp, so maybe it makes to factorize it.
*
*/
static bool processSubscriptions(EntityIdVector triggerEntitiesV, map<string, BSONObj*>* subs, std::string* err) {

    DBClientConnection* connection = getMongoConnection();

    /* For each one of the subscriptions in the map, send notification */
    bool ret = true;
    for (std::map<string, BSONObj*>::iterator it = subs->begin(); it != subs->end(); ++it) {

        //FIXME P8: see old issue #90
        //BSONObj sub = *(it->second);
        std::string mapSubId = it->first;
        BSONObj sub = connection->findOne(getSubscribeContextAvailabilityCollectionName(), BSON("_id" << OID(mapSubId)));
        LM_T(LmtMongo, ("retrieved document: '%s'", sub.toString().c_str()));
        OID subId = sub.getField("_id").OID();

        /* Build attribute list vector */
        AttributeList attrL = subToAttributeList(sub);

        /* Get format. If not found in the csubs document (it could happen in the case of updating Orion using an existing database) we use XML */
        Format format = sub.hasField(CASUB_FORMAT) ? stringToFormat(STR_FIELD(sub, CASUB_FORMAT)) : XML;

        /* Send notification */
        if (!processAvailabilitySubscription(triggerEntitiesV, attrL, subId.str(), STR_FIELD(sub, CSUB_REFERENCE), format)) {
            ret = false;
        }

        /* Release object created dynamically (including the value in the map created by
         * addTriggeredSubscriptions */
        attrL.release();
        delete it->second;

    }

    return ret;
}

/* ****************************************************************************
*
* addTriggeredSubscriptions
*
*/
static bool addTriggeredSubscriptions(ContextRegistration cr, map<string, BSONObj*>* subs, std::string* err) {

    DBClientConnection* connection = getMongoConnection();

    BSONArrayBuilder entitiesNoPatternA;
    std::vector<std::string> idJsV;
    std::vector<std::string> typeJsV;

    for (unsigned int ix = 0; ix < cr.entityIdVector.size(); ++ix ) {
        //FIXME: take into account subscriptions with no type
        EntityId* enP = cr.entityIdVector.get(ix);
        /* The registration of isPattern=true entities is not supported, so we don't include them here */
        if (enP->isPattern == "false") {
            entitiesNoPatternA.append(BSON(CASUB_ENTITY_ID << enP->id << CASUB_ENTITY_TYPE << enP->type << CASUB_ENTITY_ISPATTERN << "false"));
            idJsV.push_back(enP->id);
            typeJsV.push_back(enP->type);
        }
    }
    BSONArrayBuilder attrA;
    for (unsigned int ix = 0; ix < cr.contextRegistrationAttributeVector.size(); ++ix) {
        ContextRegistrationAttribute* craP = cr.contextRegistrationAttributeVector.get(ix);
        attrA.append(craP->name);
    }

    BSONObjBuilder queryNoPattern;
    queryNoPattern.append(CASUB_ENTITIES, BSON("$in" << entitiesNoPatternA.arr()));
    if (attrA.arrSize() > 0) {
        /* If we don't do this checking, the {$in: [] } in the attribute name part will
         * make the query fail*/
        //queryB.append(CASUB_ATTRS, BSON("$in" << attrA.arr()));
        queryNoPattern.append("$or", BSON_ARRAY(
                          BSON(CASUB_ATTRS << BSON("$in" << attrA.arr())) <<
                          BSON(CASUB_ATTRS << BSON("$size" << 0))
                          ));
    }
    else {
        queryNoPattern.append(CASUB_ATTRS, BSON("$size" << 0));
    }
    queryNoPattern.append(CASUB_EXPIRATION, BSON("$gt" << getCurrentTime()));

    /* This is JavaScript code that runs in MongoDB engine. As far as I know, this is the only
     * way to do a "reverse regex" query in MongoDB (see
     * http://stackoverflow.com/questions/15966991/mongodb-reverse-regex/15989520).
     * Note that although we are using a isPattern=true in the MongoDB query besides $where, we
     * also need to check that in the if statement in the JavaScript function given that a given
     * sub document could include both isPattern=true and isPattern=false documents */
    std::string idJsString = "[ ";
    for (unsigned int ix = 0; ix < idJsV.size(); ++ix ) {
        if (ix != idJsV.size()-1) {
            idJsString += "\""+idJsV[ix]+ "\" ,";
        }
        else {
            idJsString += "\"" +idJsV[ix]+ "\"";
        }
    }
    idJsString += " ]";

    std::string typeJsString = "[ ";
    for (unsigned int ix = 0; ix < typeJsV.size(); ++ix ) {
        if (ix != typeJsV.size()-1) {
            typeJsString += "\"" +typeJsV[ix] + "\" ,";
        }
        else {
            typeJsString += "\"" + typeJsV[ix] + "\"";
        }
    }
    typeJsString += " ]";

    std::string function = std::string("function()") +
         "{" +
            "enId = "+idJsString+ ";" +
            "enType = "+typeJsString+ ";" +
            "for (var i=0; i < this."+CASUB_ENTITIES+".length; i++) {" +
                "if (this."+CASUB_ENTITIES+"[i]."+CASUB_ENTITY_ISPATTERN+" == \"true\") {" +
                    "for (var j=0; j < enId.length; j++) {" +
                       "if (enId[j].match(this."+CASUB_ENTITIES+"[i]."+CASUB_ENTITY_ID+") && this."+CASUB_ENTITIES+"[i]."+CASUB_ENTITY_TYPE+" == enType[j]) {" +
                          "return true; " +
                       "}" +
                    "}" +
                "}" +
            "}" +
            "return false; " +
         "}";
    LM_T(LmtMongo, ("JS function: %s", function.c_str()));

    std::string entPatternQ = std::string(CSUB_ENTITIES) + "." + CSUB_ENTITY_ISPATTERN;

    BSONObjBuilder queryPattern;
    queryPattern.append(entPatternQ, "true");
    queryPattern.append(CASUB_EXPIRATION, BSON("$gt" << getCurrentTime()));
    queryPattern.appendCode("$where", function);

    BSONObj query = BSON("$or" << BSON_ARRAY(queryNoPattern.obj() << queryPattern.obj()));

    /* Do the query */
    auto_ptr<DBClientCursor> cursor;
    try {
        LM_T(LmtMongo, ("query() in '%s' collection: '%s'", getSubscribeContextAvailabilityCollectionName(), query.toString().c_str()));
        cursor = connection->query(getSubscribeContextAvailabilityCollectionName(), query);
        /* We have observed that in some cases of DB errors (e.g. the database daemon is down) instead of
         * raising an exceiption the query() method set the cursos to NULL. In this case, we raise the
         * exception ourselves */
        if (cursor.get() == NULL) {
            throw DBException("Null cursor", 0);
        }
    }
    catch( const DBException &e ) {
        *err = std::string("collection: ") + getSubscribeContextAvailabilityCollectionName() +
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
            subs->insert(std::pair<string, BSONObj*>(subIdStr, new BSONObj(sub)));
        }

    }

    return true;

}

/* ****************************************************************************
*
* processRegisterContext -
*
* This function has a slight different behaviour depending on whether and id
* parameter is null (new registration case) or not null (update case), in
* particular:
*
* - In the new registration case, the _id is generated and insert() is used to
*   put the document in the DB
* - In the update case, the _id is set with the one in the argument and update() is
*   used to put the document in the DB
*
*/
HttpStatusCode processRegisterContext(RegisterContextRequest* requestP, RegisterContextResponse* responseP, OID* id)
{

    DBClientConnection* connection = getMongoConnection();

    /* If expiration is not present, then use a default one */
    if (requestP->duration.isEmpty()) {
        requestP->duration.set(DEFAULT_DURATION);
    }

    /* Calculate expiration (using the current time and the duration field in the request) */
    int expiration = getCurrentTime() + requestP->duration.parse();
    LM_T(LmtMongo, ("Registration expiration: %d", expiration));

    /* Create the mongoDB registration document */
    BSONObjBuilder reg;
    OID oid;
    if (id == NULL) {
        oid.init();
    }
    else {
        oid = *id;
    }
    reg.append("_id", oid);
    reg.append(REG_EXPIRATION, expiration);

    /* We accumulate the subscriptions in a map. The key of the map is the string representing
     * subscription id */
    std::map<string, BSONObj*> subsToNotify;

    /* This vector is used to define which entities to include in notifications */
    EntityIdVector triggerEntitiesV;

    BSONArrayBuilder contextRegistration;
    for (unsigned int ix = 0; ix < requestP->contextRegistrationVector.size(); ++ix) {

        ContextRegistration* cr = requestP->contextRegistrationVector.get(ix);

        BSONArrayBuilder entities;
        for (unsigned int jx = 0; jx < cr->entityIdVector.size(); ++jx) {

            EntityId* en = cr->entityIdVector.get(jx);
            triggerEntitiesV.push_back(en);

            if (en->type == "") {
                entities.append(BSON(REG_ENTITY_ID << en->id));
                LM_T(LmtMongo, ("Entity registration: {id: %s}", en->id.c_str()));
            }
            else {
                entities.append(BSON(REG_ENTITY_ID << en->id << REG_ENTITY_TYPE << en->type));
                LM_T(LmtMongo, ("Entity registration: {id: %s, type: %s}", en->id.c_str(), en->type.c_str()));
            }

        }

        BSONArrayBuilder attrs;
        for (unsigned int jx = 0; jx < cr->contextRegistrationAttributeVector.size(); ++jx) {

           ContextRegistrationAttribute* cra = cr->contextRegistrationAttributeVector.get(jx);
            attrs.append(BSON(REG_ATTRS_NAME << cra->name << REG_ATTRS_TYPE << cra->type << "isDomain" << cra->isDomain));
            LM_T(LmtMongo, ("Attribute registration: {name: %s, type: %s, isDomain: %s}",
                               cra->name.c_str(),
                               cra->type.c_str(),
                               cra->isDomain.c_str())
            );

            for (unsigned int kx = 0; kx < requestP->contextRegistrationVector.get(ix)->contextRegistrationAttributeVector.get(jx)->metadataVector.size(); ++kx) {
                // FIXME: metadata not supported at the moment
            }
        }

        contextRegistration.append(BSON(
            REG_ENTITIES << entities.arr() <<
            REG_ATTRS << attrs.arr() <<
            REG_PROVIDING_APPLICATION << requestP->contextRegistrationVector.get(ix)->providingApplication.get())
        );
        LM_T(LmtMongo, ("providingApplication registration: %s", requestP->contextRegistrationVector.get(ix)->providingApplication.c_str()));

        std::string err;
        if (!processAssociations(cr->registrationMetadataVector, &err)) {
            responseP->errorCode.fill(SccReceiverInternalError, "Database Error", err );
            LM_RE(SccOk, (err.c_str()));
        }
        if (!addTriggeredSubscriptions(*cr, &subsToNotify, &err)) {
            responseP->errorCode.fill(SccReceiverInternalError, "Database Error", err );
            LM_RE(SccOk, (err.c_str()));
        }

    }
    reg.append(REG_CONTEXT_REGISTRATION, contextRegistration.arr());

    BSONObj regDoc = reg.obj();
    try {
        /* Note the four parameter to "true". This means "upsert", so if the document doesn't previously
         * exist in colleciton, it is created at the same time. Thus, this way is ok with both uses of
         * registerContext (either new registration or updating an existing one) */
        LM_T(LmtMongo, ("upsert update() in '%s' collection: '%s'", getRegistrationsCollectionName(), regDoc.toString().c_str()));
        connection->update(getRegistrationsCollectionName(), BSON("_id" << oid), regDoc, true);
    }
    catch( const DBException &e ) {
        responseP->errorCode.fill(
            SccReceiverInternalError,
            "Database Error",
            std::string("collection: ") + getRegistrationsCollectionName() +
                " - upsert update(): " + regDoc.toString() +
                " - exception: " + e.what()
        );

        LM_RE(SccOk,("Database error '%s'", responseP->errorCode.reasonPhrase.c_str()));
    }

    /* Send notifications for each one of the subscriptions accumulated by
     * previous addTriggeredSubscriptions() invocations */
    std::string err;
    processSubscriptions(triggerEntitiesV, &subsToNotify, &err);

    /* Fill the response element */
    responseP->duration = requestP->duration;
    responseP->registrationId.set(oid.str());
    responseP->errorCode.fill(SccOk, "OK");

    return SccOk;
}
