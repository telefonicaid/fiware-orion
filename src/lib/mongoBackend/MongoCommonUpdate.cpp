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
#include "common/string.h"
#include "common/sem.h"
#include "mongoBackend/MongoGlobal.h"

using std::string;
using std::map;
using std::auto_ptr;

/* ****************************************************************************
 * Forward declarations
 */
static void compoundValueBson(std::vector<orion::CompoundValueNode*> children, BSONObjBuilder& b);

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
static bool smartAttrMatch(std::string name1, std::string type1, std::string id1, std::string name2, std::string type2, std::string id2) {
    if (type2 == "") {
        return ((name1 == name2) && (id1 == id2));
    }
    else {
        return ((name1 == name2) && (type1 == type2) && (id1 == id2));
    }
}

/* ****************************************************************************
*
* compoundValueBson (for arrays) -
*
*/
static void compoundValueBson(std::vector<orion::CompoundValueNode*> children, BSONArrayBuilder& b)
{

    for (unsigned int ix = 0; ix < children.size(); ++ix) {
        orion::CompoundValueNode* child = children[ix];
        if (child->type == orion::CompoundValueNode::String) {
            b.append(child->value);
        }
        else if (child->type == orion::CompoundValueNode::Vector) {
            BSONArrayBuilder ba;
            compoundValueBson(child->childV, ba);
            b.append(ba.arr());
        }
        else if (child->type == orion::CompoundValueNode::Object) {
            BSONObjBuilder bo;
            compoundValueBson(child->childV, bo);
            b.append(bo.obj());
        }
        else
        {
          LM_T(LmtError, ("Unknown type in compound value"));
        }
    }

}

/* ****************************************************************************
*
* compoundValueBson -
*
*/
static void compoundValueBson(std::vector<orion::CompoundValueNode*> children, BSONObjBuilder& b)
{
    for (unsigned int ix = 0; ix < children.size(); ++ix) {
        orion::CompoundValueNode* child = children[ix];
        if (child->type == orion::CompoundValueNode::String) {
            b.append(child->name, child->value);
        }
        else if (child->type == orion::CompoundValueNode::Vector) {
            BSONArrayBuilder ba;
            compoundValueBson(child->childV, ba);
            b.append(child->name, ba.arr());
        }
        else if (child->type == orion::CompoundValueNode::Object) {
            BSONObjBuilder bo;
            compoundValueBson(child->childV, bo);
            b.append(child->name, bo.obj());
        }
        else
        {
          LM_T(LmtError, ("Unknown type in compound value"));
        }
    }
}

/* ****************************************************************************
*
* valueBson -
*
*/
static void valueBson(ContextAttribute* ca, BSONObjBuilder& bsonAttr) {

    if (ca->compoundValueP == NULL) {
        bsonAttr.append(ENT_ATTRS_VALUE, ca->value);
    }
    else {
        if (ca->compoundValueP->type == orion::CompoundValueNode::Vector) {
            BSONArrayBuilder b;
            compoundValueBson(ca->compoundValueP->childV, b);
            bsonAttr.append(ENT_ATTRS_VALUE, b.arr());
        }
        else if (ca->compoundValueP->type == orion::CompoundValueNode::Object) {
            BSONObjBuilder b;
            compoundValueBson(ca->compoundValueP->childV, b);
            bsonAttr.append(ENT_ATTRS_VALUE, b.obj());
        }
        else if (ca->compoundValueP->type == orion::CompoundValueNode::String) {
            // FIXME P4: this is somehow redundant. See https://github.com/telefonicaid/fiware-orion/issues/271
            bsonAttr.append(ENT_ATTRS_VALUE, ca->compoundValueP->value);
        }
        else
        {
          LM_T(LmtError, ("Unknown type in compound value"));
        }
    }

}

/* ****************************************************************************
*
* isNotCustomMetadata -
*
* Check that the parameter is a not custom metadata, i.e. one metadata without
* an special semantic to be interpreted by the context broker itself
*
* FIXME P2: this function probably could be moved to another place "closer" to metadata
*/
static bool isNotCustomMetadata(std::string md) {
    if (md != NGSI_MD_ID &&
        md != NGSI_MD_LOCATION &&
        md != NGSI_MD_CREDATE &&
        md != NGSI_MD_MODDATE) {
        return false;
    }    
    return true;
}

/* ****************************************************************************
*
* hasMetadata -
*
* Check if a metadata is included in a (request) ContextAttribute
*/
static bool hasMetadata(std::string name, std::string type, ContextAttribute ca) {

    for (unsigned int ix = 0; ix < ca.metadataVector.size() ; ++ix) {
        Metadata* md = ca.metadataVector.get(ix);
        /* Note that, different from entity types or attribute types, for attribute metadata we don't consider empty type
         * as a wildcard in order to keep it simpler */
        if ((md->name == name) && (md->type == type)) {
            return true;
        }
    }
    return false;

}


/* ****************************************************************************
*
* matchMetadata -
*
* Returns if two metadata elements have the same name and type, taking into account that
* the type field could not be present (it is optional in NGSI)
*/
static bool matchMetadata(BSONObj& md1, BSONObj& md2) {

    if (md1.hasField(ENT_ATTRS_MD_TYPE)) {
        return md2.hasField(ENT_ATTRS_MD_TYPE) &&
               STR_FIELD(md1, ENT_ATTRS_MD_TYPE) == STR_FIELD(md2, ENT_ATTRS_MD_TYPE) &&
               STR_FIELD(md1, ENT_ATTRS_MD_NAME) == STR_FIELD(md2, ENT_ATTRS_MD_NAME);
    }
    else {
        return !md2.hasField(ENT_ATTRS_MD_TYPE) &&
               STR_FIELD(md1, ENT_ATTRS_MD_NAME) == STR_FIELD(md2, ENT_ATTRS_MD_NAME);
    }

}

/* ****************************************************************************
*
* equalMetadataVectors -
*
* Given two vectors with the same metadata names-types, check that all the values
* are equal, returning false otherwise.
*
* Arguments are passed by reference to avoid "heavy" copies
*/
static bool equalMetadataVectors(BSONObj& mdV1, BSONObj& mdV2) {


    bool found = false;
    for( BSONObj::iterator i1 = mdV1.begin(); i1.more(); ) {
        BSONObj md1 = i1.next().embeddedObject();
        for( BSONObj::iterator i2 = mdV2.begin(); i2.more(); ) {
            BSONObj md2 = i2.next().embeddedObject();
            /* Check metadata match */
            if (matchMetadata(md1, md2)) {
                if (STR_FIELD(md1, ENT_ATTRS_MD_VALUE) != STR_FIELD(md2, ENT_ATTRS_MD_VALUE)) {
                    return false;
                }
                else {
                    found = true;
                    break;  // loop in i2
                }
            }
        }
        if (found) {
            /* Shortcut for early passing to the next attribute in i1 */
            found = false;
            continue; // loop in i1
        }
    }
    return true;

}

/* ****************************************************************************
*
* bsonCustomMetadataToBson -
*
* Generates the BSON for metadata vector to be inserted in database for a given attribute.
* If there is no custom metadata, then it returns false (true otherwise).
*
*/
static bool bsonCustomMetadataToBson(BSONObj& newMdV, BSONObj& attr) {

    if (!attr.hasField(ENT_ATTRS_MD)) {
        return false;
    }

    BSONArrayBuilder mdNewVBuilder;
    BSONObj mdV = attr.getField(ENT_ATTRS_MD).embeddedObject();

    for( BSONObj::iterator i = mdV.begin(); i.more(); ) {
        BSONObj md = i.next().embeddedObject();

        if (md.hasField(ENT_ATTRS_MD_TYPE)) {
            mdNewVBuilder.append(BSON(ENT_ATTRS_MD_NAME << md.getStringField(ENT_ATTRS_MD_NAME) <<
                                      ENT_ATTRS_MD_TYPE << md.getStringField(ENT_ATTRS_MD_TYPE) <<
                                      ENT_ATTRS_MD_VALUE << md.getStringField(ENT_ATTRS_MD_VALUE)));
         }
         else {
            mdNewVBuilder.append(BSON(ENT_ATTRS_MD_NAME << md.getStringField(ENT_ATTRS_MD_NAME) <<
                                      ENT_ATTRS_MD_VALUE << md.getStringField(ENT_ATTRS_MD_VALUE)));
         }
     }

     if (mdNewVBuilder.arrSize() > 0) {
         newMdV = mdNewVBuilder.arr();
         return true;
     }
     return false;

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
static bool checkAndUpdate (BSONObjBuilder& newAttr, BSONObj attr, ContextAttribute ca, bool* actualUpdate) {

    bool updated = false;
    *actualUpdate = false;

    newAttr.append(ENT_ATTRS_NAME, STR_FIELD(attr, ENT_ATTRS_NAME));
    newAttr.append(ENT_ATTRS_TYPE, STR_FIELD(attr, ENT_ATTRS_TYPE));
    /* The hasField() check is needed to preserve compatibility with entities that were created
     * in database by a CB instance previous to the support of creation and modification dates */
    if (attr.hasField(ENT_ATTRS_CREATION_DATE)) {
        newAttr.append(ENT_ATTRS_CREATION_DATE, attr.getIntField(ENT_ATTRS_CREATION_DATE));
    }
    if (STR_FIELD(attr, ENT_ATTRS_ID) != "") {
        newAttr.append(ENT_ATTRS_ID, STR_FIELD(attr, ENT_ATTRS_ID));
    }
    if (smartAttrMatch(STR_FIELD(attr, ENT_ATTRS_NAME), STR_FIELD(attr, ENT_ATTRS_TYPE), STR_FIELD(attr, ENT_ATTRS_ID),
                       ca.name, ca.type, ca.getId())) {

        /* Attribute match: update value */
        valueBson(&ca, newAttr);
        updated = true;

        /* Update metadata */
        BSONArrayBuilder mdNewVBuilder;

        /* First add the metadata elements coming in the request */
        for (unsigned int ix = 0; ix < ca.metadataVector.size() ; ++ix) {
            Metadata* md = ca.metadataVector.get(ix);
            /* Skip not custom metadata */
            if (isNotCustomMetadata(md->name)) {
                continue;
            }
            if (md->type == "") {
                mdNewVBuilder.append(BSON(ENT_ATTRS_MD_NAME << md->name << ENT_ATTRS_MD_VALUE << md->value));
            }
            else {
                mdNewVBuilder.append(BSON(ENT_ATTRS_MD_NAME << md->name << ENT_ATTRS_MD_TYPE << md->type << ENT_ATTRS_MD_VALUE << md->value));
            }
        }

        /* Second, for each metadata previously in the metadata vector but *not included in the request*, add it as is */
        int mdVSize = 0;
        BSONObj mdV;
        if (attr.hasField(ENT_ATTRS_MD)) {
            mdV = attr.getField(ENT_ATTRS_MD).embeddedObject();
            for( BSONObj::iterator i = mdV.begin(); i.more(); ) {
                BSONObj md = i.next().embeddedObject();
                mdVSize++;
                if (!hasMetadata(md.getStringField(ENT_ATTRS_MD_NAME), md.getStringField(ENT_ATTRS_MD_TYPE), ca)) {
                    if (md.hasField(ENT_ATTRS_MD_TYPE)) {
                        mdNewVBuilder.append(BSON(ENT_ATTRS_MD_NAME << md.getStringField(ENT_ATTRS_MD_NAME) <<
                                           ENT_ATTRS_MD_TYPE << md.getStringField(ENT_ATTRS_MD_TYPE) <<
                                           ENT_ATTRS_MD_VALUE << md.getStringField(ENT_ATTRS_MD_VALUE)));
                    }
                    else {
                        mdNewVBuilder.append(BSON(ENT_ATTRS_MD_NAME << md.getStringField(ENT_ATTRS_MD_NAME) <<
                                           ENT_ATTRS_MD_VALUE << md.getStringField(ENT_ATTRS_MD_VALUE)));
                    }
                }
            }
        }

        BSONObj mdNewV = mdNewVBuilder.arr();
        if (mdNewVBuilder.arrSize() > 0) {
            newAttr.appendArray(ENT_ATTRS_MD, mdNewV);
        }

        /* It was an actual update? */
        if (ca.compoundValueP == NULL) {
            /* In the case of simple value, we check if the value of the attribute changed or the metadata changed (the later
             * one is done checking if the size of the original and final metadata vectors is different and, if they are of the
             * same size, checking if the vectors are not equal) */
            if (!attr.hasField(ENT_ATTRS_VALUE) || STR_FIELD(attr, ENT_ATTRS_VALUE) != ca.value || mdNewVBuilder.arrSize() != mdVSize || !equalMetadataVectors(mdV, mdNewV)) {
                *actualUpdate = true;
            }
        }
        else {
            // FIXME P6: in the case of compound value, it's more difficult to know if an attribute
            // has really changed its value (many levels have to be traversed). Until we can develop the
            // matching logic, we consider actualUpdate always true.
            *actualUpdate = true;
        }

    }
    else {
        /* Attribute doesn't match */

        /* Value is included "as is", taking into account it can be a compound value */
        if (attr.hasField(ENT_ATTRS_VALUE)) {
            BSONElement value = attr.getField(ENT_ATTRS_VALUE);
            if (value.type() == String) {
                newAttr.append(ENT_ATTRS_VALUE, value.String());
            }
            else if (attr.getField(ENT_ATTRS_VALUE).type() == Object) {
                newAttr.append(ENT_ATTRS_VALUE, value.embeddedObject());
            }
            else if (attr.getField(ENT_ATTRS_VALUE).type() == Array) {
                newAttr.appendArray(ENT_ATTRS_VALUE, value.embeddedObject());
            }
            else
            {
              LM_T(LmtError, ("unknown BSON type"));
            }

        }

        /* Metadata is included "as is" */
        BSONObj mdV;
        if (bsonCustomMetadataToBson(mdV, attr)) {
            newAttr.appendArray(ENT_ATTRS_MD, mdV);
        }
    }

    /* In the case of actual update, we update also the modification date; otherwise we include the previous existing one */
    if (*actualUpdate) {
        newAttr.append(ENT_ATTRS_MODIFICATION_DATE, getCurrentTime());
    }
    else {
        /* The hasField() check is needed to preserve compatibility with entities that were created
         * in database by a CB instance previous to the support of creation and modification dates */
        if (attr.hasField(ENT_ATTRS_MODIFICATION_DATE)) {
            newAttr.append(ENT_ATTRS_MODIFICATION_DATE, attr.getIntField(ENT_ATTRS_MODIFICATION_DATE));
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

        /* Custom metadata */
        BSONObj mdV;
        if (bsonCustomMetadataToBson(mdV, attr)) {
            newAttr->appendArray(ENT_ATTRS_MD, mdV);
        }

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
static bool updateAttribute(BSONObj& attrs, BSONObj& newAttrs, ContextAttribute* caP, bool& actualUpdate) {

    BSONArrayBuilder newAttrsBuilder;
    actualUpdate = false;
    bool updated = false;
    for( BSONObj::iterator i = attrs.begin(); i.more(); ) {

        BSONObjBuilder newAttr;
        bool unitActualUpdate = false;
        if (checkAndUpdate(newAttr, i.next().embeddedObject(), *caP, &unitActualUpdate) && !updated) {
            updated = true;
        }
        /* If at least one actual update was done at checkAndUpdate() level, then updateAttribute()
         * actual update is true */
        if (unitActualUpdate == true) {
            actualUpdate = true;
        }

        newAttrsBuilder.append(newAttr.obj());
    }
    newAttrs = newAttrsBuilder.arr();

    return updated;

}

/* ****************************************************************************
*
* contextAttributeCustomMetadataToBson -
*
* Generates the BSON for metadata vector to be inserted in database for a given atribute.
* If there is no custom metadata, then it returns false (true otherwise).
*
*/
static bool contextAttributeCustomMetadataToBson(BSONObj& mdV, ContextAttribute* ca) {

    BSONArrayBuilder mdToAdd;

    for (unsigned int ix = 0; ix < ca->metadataVector.size(); ++ix) {
        Metadata* md = ca->metadataVector.get(ix);
        if (!isNotCustomMetadata(md->name)) {
            mdToAdd.append(BSON("name" << md->name << "type" << md->type << "value" << md->value));
            LM_T(LmtMongo, ("new custom metadata: {name: %s, type: %s, value: %s}",
                            md->name.c_str(), md->type.c_str(), md->value.c_str()));
        }
    }

    if (mdToAdd.arrSize() > 0) {
        mdV = mdToAdd.arr();
        return true;
    }   
    return false;
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
static bool appendAttribute(BSONObj& attrs, BSONObj& newAttrs, ContextAttribute* caP) {

    /* APPEND with existing attribute equals to UPDATE */
    BSONArrayBuilder newAttrsBuilder;
    bool updated = false;
    bool actualUpdate = false;
    for( BSONObj::iterator i = attrs.begin(); i.more(); ) {

        BSONObjBuilder newAttr;
        bool attrActualUpdate;
        updated = checkAndUpdate(newAttr, i.next().embeddedObject(), *caP, &attrActualUpdate) || updated;
        actualUpdate = attrActualUpdate || actualUpdate;
        newAttrsBuilder.append(newAttr.obj());
    }

    /* If not updated, then append */
    if (!updated) {
        BSONObjBuilder newAttr;
        newAttr.append(ENT_ATTRS_NAME, caP->name);
        newAttr.append(ENT_ATTRS_TYPE, caP->type);
        valueBson(caP, newAttr);
        if (caP->getId() != "") {
            newAttr.append(ENT_ATTRS_ID, caP->getId());
        }
        BSONObj mdV;
        if (contextAttributeCustomMetadataToBson(mdV, caP)) {
            newAttr.appendArray(ENT_ATTRS_MD, mdV);
        }

        int now = getCurrentTime();
        newAttr.append(ENT_ATTRS_CREATION_DATE, now);
        newAttr.append(ENT_ATTRS_MODIFICATION_DATE, now);
        newAttrsBuilder.append(newAttr.obj());

        newAttrs = newAttrsBuilder.arr();

        return true;
    }
    else {
        newAttrs = newAttrsBuilder.arr();
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
static bool legalIdUsage(BSONObj& attrs, ContextAttribute* caP) {

    if (caP->getId() == "") {
        /* Attribute attempting to append hasn't ID. Thus, no attribute with same name can have ID in attrs */
        for( BSONObj::iterator i = attrs.begin(); i.more(); ) {
            BSONObj attr = i.next().embeddedObject();
            if (STR_FIELD(attr, ENT_ATTRS_NAME) == caP->name && STR_FIELD(attr, ENT_ATTRS_TYPE) == caP->type && STR_FIELD(attr, ENT_ATTRS_ID) != "") {
                return false;
            }
        }
        return true;
    }
    else {
        /* Attribute attempting to append has ID. Thus, no attribute with same name cannot have ID in attrs */
        for( BSONObj::iterator i = attrs.begin(); i.more(); ) {
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
* processLocation -
*
* This function process the context attribute vector, searching for an attribute marked with
* the location metadata. In that case, it fills locAttr, coordLat and coordLOng. If a location
* attribute is not found, then locAttr is filled with an empty string, i.e. "".
*
* This function always return true (no matter if the attribute was found or not), except in an
* error situation, in which case errorDetail is filled. This can be due to two reasons: ilegal
* usage of the metadata or parsing error in the attribute value.
*
*/

static bool processLocation(ContextAttributeVector caV, std::string& locAttr, double& coordLat, double& coordLong, std::string* errDetail) {

    locAttr = "";
    for (unsigned ix = 0; ix < caV.size(); ++ix) {
        ContextAttribute* caP = caV.get(ix);
        for (unsigned jx = 0; jx < caP->metadataVector.size(); ++jx) {
            Metadata* mdP = caP->metadataVector.get(jx);
            if (mdP->name == NGSI_MD_LOCATION) {
                if (locAttr.length() > 0) {
                    *errDetail = "You cannot use more than one location attribute when creating an entity (see Orion user manual)";
                    return false;
                }
                else {
                    if (mdP->value != LOCATION_WSG84) {
                        *errDetail = "only WSG84 are supported, found: " + mdP->value;
                        return false;
                    }

                    if (!string2coords(caP->value, coordLat, coordLong)) {
                        *errDetail = "coordinate format error (see Orion user manual): " + caP->value;
                        return false;
                    }
                    locAttr = caP->name;
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
static bool deleteAttribute(BSONObj& attrs, BSONObj& newAttrs, ContextAttribute* caP) {
    BSONArrayBuilder newAttrsBuilder;
    bool deleted = false;
    for( BSONObj::iterator i = attrs.begin(); i.more(); ) {

        BSONObjBuilder newAttr;
        if (checkAndDelete(&newAttr, i.next().embeddedObject(), *caP) && !deleted) {
            deleted = true;
        }
        else {
            newAttrsBuilder.append(newAttr.obj());
        }
    }
    newAttrs = newAttrsBuilder.arr();

    return deleted;
}

/* ****************************************************************************
*
* addTriggeredSubscriptions
*
*/
static bool addTriggeredSubscriptions(std::string entityId, std::string entityType, std::string attr, map<string, BSONObj*>* subs, std::string* err, std::string tenant) {

    DBClientConnection* connection = getMongoConnection();

    /* Build query */
    std::string entIdQ       = CSUB_ENTITIES   "." CSUB_ENTITY_ID;
    std::string entTypeQ     = CSUB_ENTITIES   "." CSUB_ENTITY_TYPE;
    std::string entPatternQ  = CSUB_ENTITIES   "." CSUB_ENTITY_ISPATTERN;
    std::string condTypeQ    = CSUB_CONDITIONS "." CSUB_CONDITIONS_TYPE;
    std::string condValueQ   = CSUB_CONDITIONS "." CSUB_CONDITIONS_VALUE;

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
        LM_T(LmtMongo, ("query() in '%s' collection: '%s'", getSubscribeContextCollectionName(tenant).c_str(), query.toString().c_str()));

        mongoSemTake(__FUNCTION__, "query in SubscribeContextCollection");
        cursor = connection->query(getSubscribeContextCollectionName(tenant).c_str(), query);

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
        *err = std::string("collection: ") + getSubscribeContextCollectionName(tenant).c_str() +
               " - query(): " + query.toString() +
               " - exception: " + e.what();
        return false;
    }
    catch(...) {
        mongoSemGive(__FUNCTION__, "query in SubscribeContextCollection (Generic Exception)");
        *err = std::string("collection: ") + getSubscribeContextCollectionName(tenant).c_str() +
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
            // FIXME P8: see issue #371
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
static bool processSubscriptions(const EntityId* enP, map<string, BSONObj*>* subs, std::string* err, std::string tenant) {

    DBClientConnection* connection = getMongoConnection();

    /* For each one of the subscriptions in the map, send notification */
    for (std::map<string, BSONObj*>::iterator it = subs->begin(); it != subs->end(); ++it) {

        //FIXME P8: see issue #371
        //BSONObj sub = *(it->second);

        std::string  mapSubId = it->first;
        BSONObj      sub;

        try
        {
           mongoSemTake(__FUNCTION__, "findOne in SubscribeContextCollection");
           sub = connection->findOne(getSubscribeContextCollectionName(tenant).c_str(), BSON("_id" << OID(mapSubId)));
           mongoSemGive(__FUNCTION__, "findOne in SubscribeContextCollection");
        }
        catch (...)
        {
           mongoSemGive(__FUNCTION__, "findOne in SubscribeContextCollection (mongo generic exception)");
           LM_E(("Database Error ('findOne tenant=%s, id=%s', 'exception during findOne of collection '%s''", tenant.c_str(), mapSubId.c_str(), getSubscribeContextCollectionName(tenant).c_str()));

           *err = std::string("collection: ") + getEntitiesCollectionName(tenant).c_str();
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
        enV.push_back(new EntityId(enP->id, enP->type, enP->isPattern));

        /* Build attribute list vector */
        AttributeList attrL = subToAttributeList(sub);

        /* Get format. If not found in the csubs document (it could happen in the case of updating Orion using an existing database) we use XML */
        Format format = sub.hasField(CSUB_FORMAT) ? stringToFormat(STR_FIELD(sub, CSUB_FORMAT)) : XML;

        /* Send notification */
        if (processOnChangeCondition(enV, attrL, NULL,
                                     subId.str(),
                                     STR_FIELD(sub, CSUB_REFERENCE),
                                     format,
                                     tenant)) {

            BSONObj query = BSON("_id" << subId);
            BSONObj update = BSON("$set" << BSON(CSUB_LASTNOTIFICATION << getCurrentTime()) << "$inc" << BSON(CSUB_COUNT << 1));
            try {
                LM_T(LmtMongo, ("update() in '%s' collection: {%s, %s}", getSubscribeContextCollectionName(tenant).c_str(),
                                query.toString().c_str(),
                                update.toString().c_str()));

                mongoSemTake(__FUNCTION__, "update in SubscribeContextCollection");
                connection->update(getSubscribeContextCollectionName(tenant).c_str(), query, update);
                mongoSemGive(__FUNCTION__, "update in SubscribeContextCollection");
            }
            catch( const DBException &e ) {
                mongoSemGive(__FUNCTION__, "update in SubscribeContextCollection (mongo db exception)");
                *err = std::string("collection: ") + getEntitiesCollectionName(tenant).c_str() +
                       " - query(): " + query.toString() + " - update(): " + update.toString() + " - exception: " + e.what();
                enV.release();
                attrL.release();
                return false;
            }
            catch(...) {
                mongoSemGive(__FUNCTION__, "update in SubscribeContextCollection (mongo generic exception)");
                *err = std::string("collection: ") + getEntitiesCollectionName(tenant).c_str() +
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
        // delete it->second; FIXME P8: see issue #371
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
* setResponseMetadata -
*
* Common method to create the metadata elements in updateContext responses
*
*/
static void setResponseMetadata(ContextAttribute* caReq, ContextAttribute* caRes) {

    Metadata* md;

    /* Not custom */
    if (caReq->getId().length() > 0) {
        md = new Metadata(NGSI_MD_ID, "string", caReq->getId());
        caRes->metadataVector.push_back(md);
    }
    if (caReq->getLocation().length() > 0) {
        md = new Metadata(NGSI_MD_LOCATION, "string", caReq->getLocation());
        caRes->metadataVector.push_back(md);
    }

    /* Custom (just "mirroring" in the response) */
    for (unsigned int ix = 0; ix < caReq->metadataVector.size(); ++ix) {
        Metadata* mdReq = caReq->metadataVector.get(ix);
        if (!isNotCustomMetadata(mdReq->name)) {
            md = new Metadata(mdReq->name, mdReq->type, mdReq->value);
            caRes->metadataVector.push_back(md);
        }
    }

}

/* ****************************************************************************
*
* processContextAttributeVector -
*
* Returns true if entity was actually modified, false otherwise (including fail cases)
*
*/
static bool processContextAttributeVector (ContextElement*               ceP,
                                           std::string                   action,
                                           std::map<string, BSONObj*>*   subsToNotify,
                                           BSONObj&                      attrs,
                                           ContextElementResponse*       cerP,
                                           std::string&                  locAttr,
                                           double&                       coordLat,
                                           double&                       coordLong,
                                           std::string                   tenant)
{

    EntityId*   eP         = &cerP->contextElement.entityId;
    std::string entityId   = cerP->contextElement.entityId.id;
    std::string entityType = cerP->contextElement.entityId.type;

    bool entityModified = false;
    BSONObj newAttrs;

    for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix) {

        ContextAttribute* targetAttr = ceP->contextAttributeVector.get(ix);

        /* No matter if success or fail, we have to include the attribute in the response */
        ContextAttribute* ca = new ContextAttribute(targetAttr->name, targetAttr->type);
        setResponseMetadata(targetAttr, ca);
        cerP->contextElement.contextAttributeVector.push_back(ca);

        /* actualUpdate could be changed to false in the "update" case. For "delete" and
         * "append" it would keep the true value untouched */
        bool actualUpdate = true;
        if (strcasecmp(action.c_str(), "update") == 0) {
            if (updateAttribute(attrs, newAttrs, targetAttr, actualUpdate)) {
                entityModified = actualUpdate || entityModified;
                attrs = newAttrs;
            }
            else {
                /* If updateAttribute() returns false, then that particular attribute has not
                 * been found. In this case, we interrupt the processing and early return with
                 * an error StatusCode */
                cerP->statusCode.fill(SccInvalidParameter, 
                                      std::string("action: UPDATE") + 
                                      std::string(" - entity: (") + eP->toString() + ")" +
                                      std::string(" - offending attribute: ") + targetAttr->toString());
                return false;

            }

            /* Check aspects related with location */
            if (targetAttr->getLocation().length() > 0 && targetAttr->name != locAttr) {
                cerP->statusCode.fill(SccInvalidParameter,
                                      std::string("action: UPDATE") +
                                      std::string(" - entity: (") + eP->toString() + ")" +
                                      std::string(" - offending attribute: ") + targetAttr->toString() +
                                      std::string(" - location nature of an attribute has to be defined at creation time, with APPEND"));
                return false;
            }

            if (locAttr == targetAttr->name) {
                if (!string2coords(targetAttr->value, coordLat, coordLong)) {
                        cerP->statusCode.fill(SccInvalidParameter,
                                              std::string("action: UPDATE") +
                                              std::string(" - entity: (") + eP->toString() + ")" +
                                              std::string(" - offending attribute: ") + targetAttr->toString() +
                                              std::string(" - error parsing location attribute, value: <" + targetAttr->value + ">"));
                        return false;
                }

            }


        }
        else if (strcasecmp(action.c_str(), "append") == 0) {
            if (legalIdUsage(attrs, targetAttr)) {
                entityModified = appendAttribute(attrs, newAttrs, targetAttr) || entityModified;
                attrs = newAttrs;

                /* Check aspects related with location */
                if (targetAttr->getLocation().length() > 0 ) {
                    if (locAttr.length() > 0 && targetAttr->name != locAttr) {
                        cerP->statusCode.fill(SccInvalidParameter,
                                              std::string("action: APPEND") +
                                              std::string(" - entity: (") + eP->toString() + ")" +
                                              std::string(" - offending attribute: ") + targetAttr->toString() +
                                              std::string(" - attemp to define a location attribute (" + targetAttr->name + ") when another one has been previously defined (" + locAttr + ")"));
                        return false;
                    }

                    if (targetAttr->getLocation() != LOCATION_WSG84) {
                        cerP->statusCode.fill(SccInvalidParameter,
                                              std::string("action: APPEND") +
                                              std::string(" - entity: (") + eP->toString() + ")" +
                                              std::string(" - offending attribute: ") + targetAttr->toString() +
                                              std::string(" - only WSG84 is supported for location, found: <" + targetAttr->getLocation() + ">"));
                        return false;
                    }

                    if (!string2coords(targetAttr->value, coordLat, coordLong)) {
                            cerP->statusCode.fill(SccInvalidParameter,
                                                  std::string("action: APPEND") +
                                                  std::string(" - entity: (") + eP->toString() + ")" +
                                                  std::string(" - offending attribute: ") + targetAttr->toString() +
                                                  std::string(" - error parsing location attribute, value: <" + targetAttr->value + ">"));
                            return false;
                    }
                    locAttr = targetAttr->name;

                }

            }
            else {

                /* If legalIdUsage() returns false, then that particular attribute can not be appended. In this case,
                 * we interrupt the processing and early return with
                 * a error StatusCode */
                cerP->statusCode.fill(SccInvalidParameter,
                                      std::string("action: APPEND") +
                                      " - entity: (" + eP->toString() + ")" +
                                      " - offending attribute: " + targetAttr->toString());
                return false;
            }
        }
        else if (strcasecmp(action.c_str(), "delete") == 0) {
            if (deleteAttribute(attrs, newAttrs, targetAttr)) {
                entityModified = true;
                attrs = newAttrs;

                /* Check aspects related with location */
                if (targetAttr->getLocation().length() > 0 ) {
                    cerP->statusCode.fill(SccInvalidParameter,
                                          std::string("action: DELETE") +
                                          std::string(" - entity: (") + eP->toString() + ")" +
                                          std::string(" - offending attribute: ") + targetAttr->toString() +
                                          std::string(" - location attribute has to be defined at creation time, with APPEND"));
                    return false;
                }

                /* Check aspects related with location. "Nullining" locAttr is the way of specifying
                 * that location field is no longer used */
                if (locAttr == targetAttr->name) {
                    locAttr = "";
                }

            }
            else {
                /* If deleteAttribute() returns false, then that particular attribute has not
                 * been found. In this case, we interrupt the processing and early return with
                 * a error StatusCode */
                cerP->statusCode.fill(SccInvalidParameter,
                                      std::string("action: DELETE") +
                                      " - entity: (" + eP->toString() + ")" +
                                      " - offending attribute: " + targetAttr->toString());
                return false;

            }
        }
        else {
            cerP->statusCode.fill(SccInvalidParameter, std::string("unknown actionType: '") + action + "'");
            LM_RE(false, ("Unknown actionType '%s'. This is a bug in the parsing layer checking!", action.c_str()));
        }

        /* Add those ONCHANGE subscription triggered by the just processed attribute. Note that
         * actualUpdate is always true in the case of  "delete" or "append", so the if statement
         * is "bypassed" */
        if (actualUpdate) {
            std::string err;
            if (!addTriggeredSubscriptions(entityId, entityType, ca->name, subsToNotify, &err, tenant)) {
                cerP->statusCode.fill(SccReceiverInternalError, err);
                LM_RE(false, (err.c_str()));
            }
        }

#if 0
        /* DEBUG (see issue #371) */
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
        //FIXME P5: this is ugly, our code should be improved to set cerP in a common place for the "happy case"
        cerP->statusCode.fill(SccOk);
    }

    return entityModified;
}

/* ****************************************************************************
*
* createEntity -
*
*/
static bool createEntity(EntityId* eP, ContextAttributeVector attrsV, std::string* errDetail, std::string tenant, const std::vector<std::string>& servicePathV) {

    DBClientConnection* connection = getMongoConnection();

    LM_T(LmtMongo, ("Entity not found in '%s' collection, creating it", getEntitiesCollectionName(tenant).c_str()));

    /* Actually we don't know if this is the first entity (thus, the collection is being created) or not. However, we can
     * invoke ensureLocationIndex() in anycase, given that it is harmless in the case the collection and index already
     * exits (see docs.mongodb.org/manual/reference/method/db.collection.ensureIndex/)*/
    ensureLocationIndex(tenant);

    if (!legalIdUsage(attrsV)) {
        *errDetail = "Attributes with same name with ID and not ID at the same time in the same entity are forbidden: entity: (" + eP->toString() + ")";
        return false;
    }

    /* Search for a potential location attribute */
    std::string locAttr;
    double coordLat;
    double coordLong;
    if (!processLocation(attrsV, locAttr, coordLat, coordLong, errDetail))
      return false;


    int now = getCurrentTime();    
    BSONArrayBuilder attrsToAdd;

    for (unsigned int ix = 0; ix < attrsV.size(); ++ix) {
        std::string attrId = attrsV.get(ix)->getId();

        BSONObjBuilder bsonAttr;

        bsonAttr.appendElements(BSON(ENT_ATTRS_NAME << attrsV.get(ix)->name <<
                                     ENT_ATTRS_TYPE << attrsV.get(ix)->type <<                                     
                                     ENT_ATTRS_CREATION_DATE << now <<
                                     ENT_ATTRS_MODIFICATION_DATE << now));

        valueBson(attrsV.get(ix), bsonAttr);

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

        /* Custom metadata */
        BSONObj mdV;
        if (contextAttributeCustomMetadataToBson(mdV, attrsV.get(ix)))
            bsonAttr.appendArray(ENT_ATTRS_MD, mdV);

        attrsToAdd.append(bsonAttr.obj());
    }

    BSONObj bsonId;

    if (servicePathV.size() == 0)
    {
      LM_T(LmtServicePath, ("Empty service path"));
      bsonId = eP->type == "" ? BSON(ENT_ENTITY_ID << eP->id) : BSON(ENT_ENTITY_ID << eP->id << ENT_ENTITY_TYPE << eP->type);
    }
    else
    {
      LM_T(LmtServicePath, ("Service path string: %s", servicePathV[0].c_str()));
      bsonId = eP->type == "" ? BSON(ENT_ENTITY_ID << eP->id) : BSON(ENT_ENTITY_ID << eP->id << ENT_ENTITY_TYPE << eP->type << ENT_SERVICE_PATH << servicePathV[0]);
    } 

    BSONObjBuilder insertedDocB;
    insertedDocB.append("_id", bsonId);
    insertedDocB.append(ENT_ATTRS, attrsToAdd.arr());
    insertedDocB.append(ENT_CREATION_DATE, now);
    insertedDocB.append(ENT_MODIFICATION_DATE,now);

    /* Add location information in the case it was found */
    if (locAttr.length() > 0) {
        insertedDocB.append(ENT_LOCATION, BSON(ENT_LOCATION_ATTRNAME << locAttr <<
                                              ENT_LOCATION_COORDS << BSON_ARRAY(coordLat << coordLong)));
    }

    BSONObj insertedDoc = insertedDocB.obj();
    try {
        LM_T(LmtMongo, ("insert() in '%s' collection: '%s'", getEntitiesCollectionName(tenant).c_str(), insertedDoc.toString().c_str()));
        mongoSemTake(__FUNCTION__, "insert into EntitiesCollection");
        connection->insert(getEntitiesCollectionName(tenant).c_str(), insertedDoc);
        mongoSemGive(__FUNCTION__, "insert into EntitiesCollection");
    }
    catch( const DBException &e ) {
        mongoSemGive(__FUNCTION__, "insert into EntitiesCollection (mongo db exception)");
        *errDetail = std::string("Database Error: collection: ") + getEntitiesCollectionName(tenant).c_str() +
                " - insert(): " + insertedDoc.toString() +
                " - exception: " + e.what();

        return false;
    }
    catch(...) {
        mongoSemGive(__FUNCTION__, "insert into EntitiesCollection (mongo generic exception)");
        *errDetail = std::string("Database Error: collection: ") + getEntitiesCollectionName(tenant).c_str() +
                " - insert(): " + insertedDoc.toString() +
                " - exception: " + "generic";

        return false;
    }

    return true;
}

/* ****************************************************************************
*
* removeEntity -
*
*/
static bool removeEntity
(
  const std::string&       entityId,
  const std::string&       entityType,
  ContextElementResponse*  cerP,
  const std::string&       tenant,
  const std::string&       servicePath
)
{
    const std::string    idString          = "_id." ENT_ENTITY_ID;
    const std::string    typeString        = "_id." ENT_ENTITY_TYPE;
    const std::string    servicePathString = "_id." ENT_SERVICE_PATH;
    DBClientConnection*  connection        = getMongoConnection();
    BSONObjBuilder       bob;
    BSONObj              query;

    bob.append(idString, entityId);
    if (entityType == "")
      bob.append(typeString, BSON("$exists" << false));
    else
      bob.append(typeString, entityType);

    if (servicePath == "")
      bob.append(servicePathString, BSON("$exists" << false));
    else
      bob.append(servicePathString, servicePath);

    query = bob.obj();

    try {
        LM_T(LmtMongo, ("remove() in '%s' collection: {%s}", getEntitiesCollectionName(tenant).c_str(),
                           query.toString().c_str()));
        mongoSemTake(__FUNCTION__, "remove in EntitiesCollection");
        connection->remove(getEntitiesCollectionName(tenant).c_str(), query);
        mongoSemGive(__FUNCTION__, "remove in EntitiesCollection");
    }
    catch( const DBException &e ) {
        mongoSemGive(__FUNCTION__, "remove in EntitiesCollection (mongo db exception)");
        cerP->statusCode.fill(SccReceiverInternalError,
           std::string("collection: ") + getEntitiesCollectionName(tenant).c_str() +
           " - remove() query: " + query.toString() +
           " - exception: " + e.what());

        return false;
    }
    catch(...) {
        mongoSemGive(__FUNCTION__, "update in EntitiesCollection (mongo generic exception)");
        cerP->statusCode.fill(SccReceiverInternalError,
           std::string("collection: ") + getEntitiesCollectionName(tenant).c_str() +
           " - remove() query: " + query.toString() +
           " - exception: " + "generic");

        return false;
    }

    cerP->statusCode.fill(SccOk);
    return true;
} 

/* ****************************************************************************
*
* processContextElement -
*
* 0. Preparations
* 1. Preconditions
* 2. Get the complete list of entities from mongo
*/
void processContextElement(ContextElement* ceP, UpdateContextResponse* responseP, const std::string& action, const std::string& tenant, const std::vector<std::string>& servicePathV) {

    DBClientConnection* connection = getMongoConnection();

    /* Getting the entity in the request (helpful in other places) */
    EntityId* enP = &ceP->entityId;

    /* Not supporting isPattern = true currently */
    if (isTrue(enP->isPattern)) {
        buildGeneralErrorReponse(ceP, NULL, responseP, SccNotImplemented);
        return;
    }

    /* Check that UPDATE or APPEND is not used with attributes with empty value */
    if (strcasecmp(action.c_str(), "update") == 0 || strcasecmp(action.c_str(), "append") == 0) {
        for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix) {
            if (ceP->contextAttributeVector.get(ix)->value.size() == 0 && ceP->contextAttributeVector.get(ix)->compoundValueP == NULL) {

                ContextAttribute* aP = ceP->contextAttributeVector.get(ix);
                ContextAttribute* ca = new ContextAttribute(aP);

                buildGeneralErrorReponse(ceP, ca, responseP, SccInvalidParameter,                                   
                                   std::string("action: ") + action +
                                      " - entity: (" + enP->toString(true) + ")" +
                                      " - offending attribute: " + aP->toString() +
                                      " - empty attribute not allowed in APPEND or UPDATE");
                return;
            }
        }
    }

    /* Find entities (could be several, in the case of no type or isPattern=true) */
    const std::string  idString          = "_id." ENT_ENTITY_ID;
    const std::string  typeString        = "_id." ENT_ENTITY_TYPE;
    const std::string  servicePathString = "_id." ENT_SERVICE_PATH;

    BSONObjBuilder     bob;

    bob.append(idString, enP->id);

    if (enP->type != "")
      bob.append(typeString, enP->type);
    
    if (servicePathV.size() == 0)
    {
      bob.append(servicePathString, BSON("$exists" << false));
      LM_T(LmtServicePath, ("Updating entity '%s' (no Service Path), action '%s'", ceP->entityId.id.c_str(), action.c_str()));
    }
    else
    {
      LM_T(LmtServicePath, ("Updating entity '%s' for Service Path: '%s', action '%s'", ceP->entityId.id.c_str(), servicePathV[0].c_str(), action.c_str()));
      char               path[MAX_SERVICE_NAME_LEN];
      slashEscape(servicePathV[0].c_str(), path, sizeof(path));
      const std::string  servicePathValue  = std::string("^") + path + "$|" + "^" + path + "\\/.*";
      bob.appendRegex(servicePathString, servicePathValue);

    }
    
    BSONObj query = bob.obj();

    auto_ptr<DBClientCursor> cursor;

    try {
        LM_T(LmtMongo, ("query() in '%s' collection: '%s'", getEntitiesCollectionName(tenant).c_str(), query.toString().c_str()));
        mongoSemTake(__FUNCTION__, "query in EntitiesCollection");
        cursor = connection->query(getEntitiesCollectionName(tenant).c_str(), query);

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
                           std::string("collection: ") + getEntitiesCollectionName(tenant).c_str() +
                              " - query(): " + query.toString() +
                              " - exception: " + e.what());
        return;
    }
    catch(...) {
        mongoSemGive(__FUNCTION__, "query in EntitiesCollection (mongo generic exception)");
        buildGeneralErrorReponse(ceP, NULL, responseP, SccReceiverInternalError,                           
                           std::string("collection: ") + getEntitiesCollectionName(tenant).c_str() +
                              " - query(): " + query.toString() +
                              " - exception: " + "generic");
        return;
    }


    //
    // Going through the list of found entities.
    // As ServicePath cannot be modified, inside this loop nothing will be done
    // about ServicePath (The ServicePath was present in the mongo query to obtain the list)
    //
    // FIXME P6: Once we allow for ServicePath to be modified, this loop must be looked at.
    //
    int docs = 0;
    
    while (cursor->more()) {
        BSONObj r = cursor->next();
        LM_T(LmtMongo, ("retrieved document: '%s'", r.toString().c_str()));
        ++docs;
        std::string entityId    = STR_FIELD(r.getField("_id").embeddedObject(), ENT_ENTITY_ID);
        std::string entityType  = STR_FIELD(r.getField("_id").embeddedObject(), ENT_ENTITY_TYPE);
        std::string entitySPath = STR_FIELD(r.getField("_id").embeddedObject(), ENT_SERVICE_PATH);

        LM_T(LmtServicePath, ("Found entity '%s' in ServicePath '%s'", entityId.c_str(), entitySPath.c_str()));

        ContextElementResponse* cerP = new ContextElementResponse();
        cerP->contextElement.entityId.fill(entityId, entityType, "false");

        /* If the vector of Context Attributes is empty and the operation was DELETE, then delete the entity */
        if (strcasecmp(action.c_str(), "delete") == 0 && ceP->contextAttributeVector.size() == 0) {
            LM_T(LmtServicePath, ("Removing entity"));
            removeEntity(entityId, entityType, cerP, tenant, entitySPath);
            responseP->contextElementResponseVector.push_back(cerP);
            continue;
        }

        LM_T(LmtServicePath, ("ceP->contextAttributeVector.size: %d", ceP->contextAttributeVector.size()));
        /* We start with the attrs array in the entity document, which is manipulated by the
         * {update|delete|append}Attrsr() function for each one of the attributes in the
         * contextElement being processed. Then, we replace the resulting attrs array in the
         * BSON document and the entity document is updated. It would be more efficient to map the
         * entity attrs to something like a hash map and manipulate it there, but it does not seem
         * easy, using the mongo driver BSON API. Note that we need to use newAttrs, given that attrs is
         * BSONObj, which is an inmutable type. FIXME P6: try to improve this */
        BSONObj attrs = r.getField(ENT_ATTRS).embeddedObject();

        /* We accumulate the subscriptions in a map. The key of the map is the string representing
         * subscription id */
        std::map<string, BSONObj*> subsToNotify;

        /* Is the entity using location? In that case, we fill the locAttr, coordLat and coordLong attributes with that information, otherwise
         * we fill an empty locAttrs. Any case, processContextAttributeVector uses that information (and eventually modifies) while it
         * processes the attributes in the updateContext */
        std::string locAttr = "";
        double coordLat, coordLong;
        if (r.hasField(ENT_LOCATION)) {
            // FIXME P2: potentially, assertion error will happen if the field is not as expected. Although this shouldn't happen
            // (if it happens, it means that somebody has manipulated the DB out-of-band of the context broker), a safer way of parsing BSON object
            // will be needed. This is a general comment, applicable to many places in the mongoBackend code
            BSONObj loc = r.getObjectField(ENT_LOCATION);
            locAttr  = loc.getStringField(ENT_LOCATION_ATTRNAME);
            coordLat  = loc.getField(ENT_LOCATION_COORDS).Array()[0].Double();
            coordLong = loc.getField(ENT_LOCATION_COORDS).Array()[1].Double();
        }

        if (!processContextAttributeVector(ceP, action, &subsToNotify, attrs, cerP, locAttr, coordLat, coordLong, tenant)) {
            /* The entity wasn't actually modified, so we don't need to update it and we can continue with next one */
            responseP->contextElementResponseVector.push_back(cerP);
            continue;
        }

        /* Now that attrs contains the final status of the attributes after processing the whole
         * list of attributes in the ContextElement, update entity attributes in database */
        LM_T(LmtServicePath, ("Updating the attributes of the ContextElement"));
        BSONObjBuilder updateSet, updateUnset;
        updateSet.appendArray(ENT_ATTRS, attrs);
        updateSet.append(ENT_MODIFICATION_DATE, getCurrentTime());
        if (locAttr.length() > 0) {
            updateSet.append(ENT_LOCATION, BSON(ENT_LOCATION_ATTRNAME << locAttr <<
                                                           ENT_LOCATION_COORDS << BSON_ARRAY(coordLat << coordLong)));
        } else {
            updateUnset.append(ENT_LOCATION, 1);
        }

        /* We use $set/$unset to avoid doing a global update that will lose the creation date field. Set if always
           present, unset only if locAttr was erased */
        BSONObj updatedEntity;
        if (locAttr.length() > 0) {
            updatedEntity = BSON("$set" << updateSet.obj());
        }
        else {
            updatedEntity = BSON("$set" << updateSet.obj() << "$unset" << updateUnset.obj());
        }

        /* Note that the query that we build for updating is slighty different than the query used
         * for selecting the entities to process. In particular, the "no type" branch in the if
         * sentence selects precisely the entity with no type, using the {$exists: false} clause */
        BSONObjBuilder bob;

        // idString, typeString from earlier in this function
        bob.append(idString, entityId);

        if (entityType == "")
          bob.append(typeString, BSON("$exists" << false));
        else
          bob.append(typeString, entityType);
        
        // The servicePath of THIS object is entitySPath        
        char espath[MAX_SERVICE_NAME_LEN];
        slashEscape(entitySPath.c_str(), espath, sizeof(espath));

        // servicePathString from earlier in this function
        if (servicePathV.size() == 0)
          bob.append(servicePathString, BSON("$exists" << false));
        else
          bob.appendRegex(servicePathString, espath);
        
        BSONObj query = bob.obj();

        try
        {
            LM_T(LmtMongo, ("update() in '%s' collection: {%s, %s}", getEntitiesCollectionName(tenant).c_str(),
                            query.toString().c_str(),
                            updatedEntity.toString().c_str()));
            mongoSemTake(__FUNCTION__, "update in EntitiesCollection");
            connection->update(getEntitiesCollectionName(tenant).c_str(), query, updatedEntity);
            mongoSemGive(__FUNCTION__, "update in EntitiesCollection");
        }
        catch( const DBException &e ) {
            mongoSemGive(__FUNCTION__, "update in EntitiesCollection (mongo db exception)");
            cerP->statusCode.fill(SccReceiverInternalError,
               std::string("collection: ") + getEntitiesCollectionName(tenant).c_str() +
               " - update() query: " + query.toString() +
               " - update() doc: " + updatedEntity.toString() +
               " - exception: " + e.what());

            responseP->contextElementResponseVector.push_back(cerP);
            continue;
        }
        catch(...) {
            mongoSemGive(__FUNCTION__, "update in EntitiesCollection (mongo generic exception)");
            cerP->statusCode.fill(SccReceiverInternalError,
               std::string("collection: ") + getEntitiesCollectionName(tenant).c_str() +
               " - update() query: " + query.toString() +
               " - update() doc: " + updatedEntity.toString() +
               " - exception: " + "generic");

            responseP->contextElementResponseVector.push_back(cerP);
            continue;
        }

#if 0
        /* DEBUG (see issue #371) */
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
        processSubscriptions(enP, &subsToNotify, &err, tenant);

        /* To finish with this entity processing, add the corresponding ContextElementResponse to
         * the global response */
        cerP->statusCode.fill(SccOk);
        responseP->contextElementResponseVector.push_back(cerP);
    }
    LM_T(LmtServicePath, ("Docs found: %d", docs));



    /*
     * If the entity doesn't already exist, we create it. Note that alternatively, we could do a count()
     * before the query() to check this. However this would add a second interaction with MongoDB.
     *
     * Here we set the ServicePath if set in the request (if APPEND, of course).
     * Actually, the 'slash-escaped' ServicePath (variable: 'path') is sent to the function createEntity
     * which sets the ServicePath for the entity.
     */
    if (docs == 0) {      

        if (strcasecmp(action.c_str(), "append") != 0) {
            /* Only APPEND can create entities, thus error is returned in UPDATE or DELETE cases */
            buildGeneralErrorReponse(ceP, NULL, responseP, SccContextElementNotFound, enP->id);
        }
        else {            

            /* Creating the part of the response that doesn't depend on success or failure */
            ContextElementResponse* cerP = new ContextElementResponse();
            cerP->contextElement.entityId.fill(enP->id, enP->type, "false");
            for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix) {
                ContextAttribute* caP = ceP->contextAttributeVector.get(ix);
                ContextAttribute* ca = new ContextAttribute(caP->name, caP->type);                
                setResponseMetadata(caP, ca);
                cerP->contextElement.contextAttributeVector.push_back(ca);
            }

            std::string errReason, errDetail;
            if (!createEntity(enP, ceP->contextAttributeVector, &errDetail, tenant, servicePathV)) {
               cerP->statusCode.fill(SccInvalidParameter, errDetail);
            }
            else {
               cerP->statusCode.fill(SccOk);

                /* Successful creation: send potential notifications */
                std::map<string, BSONObj*> subsToNotify;
                for (unsigned int ix = 0; ix < ceP->contextAttributeVector.size(); ++ix) {
                    std::string err;
                    if (!addTriggeredSubscriptions(enP->id, enP->type, ceP->contextAttributeVector.get(ix)->name, &subsToNotify, &err, tenant)) {
                        cerP->statusCode.fill(SccReceiverInternalError, err);
                        responseP->contextElementResponseVector.push_back(cerP);
                        LM_RVE((err.c_str()));
                    }
                }
                processSubscriptions(enP, &subsToNotify, &errReason, tenant);
            }
            responseP->contextElementResponseVector.push_back(cerP);
        }
    }
}
