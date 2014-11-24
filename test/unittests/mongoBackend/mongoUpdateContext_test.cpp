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
* Author: Fermin Galan
*/
#include "gtest/gtest.h"
#include "unittest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoUpdateContext.h"
#include "mongoBackend/mongoQueryContext.h"
#include "ngsi/EntityId.h"
#include "ngsi/ContextElementResponse.h"
#include "ngsi10/UpdateContextRequest.h"
#include "ngsi10/UpdateContextResponse.h"
#include "ngsi10/QueryContextRequest.h"
#include "ngsi10/QueryContextResponse.h"

#include "mongo/client/dbclient.h"



/* ****************************************************************************
*
* Tests
*
* With isPattern=false:
*
* - update1Ent1Attr             - UPDATE 1 entity, 1 attribute
* - update1Ent1AttrNoType       - UPDATE 1 entity, 1 attribute (no type)
* - update1EntNotype1Attr       - UPDATE 1 entity (no type), 1 attribute
* - update1EntNotype1AttrNoType - UPDATE 1 entity (no type), 1 attribute (no type)
* - updateNEnt1Attr             - UDPATE N entity, 1 attribute
* - update1EntNAttr             - UPDATE 1 entity, N attributes
* - update1EntNAttrSameName     - UPDATE 1 entity, N attributes with the same name but different type
* - updateNEntNAttr             - UPDATE N entity, N attributes
* - append1Ent1Attr             - APPEND 1 entity, 1 attribute
* - append1Ent1AttrNoType       - APPEND 1 entity, 1 attribute (no type)
* - append1EntNotype1Attr       - APPEND 1 entity (no type), 1 attribute
* - append1EntNotype1AttrNoType - APPEND 1 entity (no type), 1 attribute (no type)
* - appendNEnt1Attr             - APPEND N entity, 1 attribute
* - append1EntNAttr             - APPEND 1 entity, N attributes
* - appendNEntNAttr             - APPEND N entity, N attributes
* - delete1Ent0Attr             - DELETE 1 entity, 0 attribute (actually, entity removal)
* - delete1Ent1Attr             - DELETE 1 entity, 1 attribute
* - delete1Ent1AttrNoType       - DELETE 1 entity, 1 attribute (no type)
* - delete1EntNotype0Attr       - DELETE 1 entity (no type), 0 attribute (actually, entity removal)
* - delete1EntNotype1Attr       - DELETE 1 entity (no type), 1 attribute
* - delete1EntNotype1AttrNoType - DELETE 1 entity (no type), 1 attribute (no type)
* - deleteNEnt1Attr             - DELETE N entity, 1 attribute
* - delete1EntNAttr             - DELETE 1 entity, N attributes
* - deleteNEntNAttr             - DELETE N entity, N attributes
* - updateEntityFails           - trying to uddate a non existing entity, fails
* - createEntity                - a non-existing entity is created
* - createEntityWithId          - a non-existing entity is created (with metadata ID in attributes)
* - createEntityMixIdNoIdFails  - attempt to create entity with same attribute with ID and not ID fails
* - updateEmptyValueFail        - fail due to UPDATE with empty attribute value
* - appendEmptyValueFail        - fail due to APPEND with empty attribute value
* - updateAttrNotFoundFail      - fail due to UPDATE in not existing attribute
* - deleteAttrNotFoundFail      - fail due to DELETE on not existing attribute
* - mixUpdateAndCreate          - mixing a regular update (on an existing entity) and entity creation in same request
* - appendExistingAttr          - treated as UPDATE
*
* With isPattern=false, related with attribute IDs
*
* - updateAttrWithId
* - updateAttrWithAndWithoutId
* - appendAttrWithId
* - appendAttrWithAndWithoutId
* - appendAttrWithIdFails
* - appendAttrWithoutIdFails
* - deleteAttrWithId
* - deleteAttrWithAndWithoutId
*
* With isPattern=true and custom metadata
*
* - appendCreateEntWithMd
* - updateMdAllExisting
* - appendMdAllExisting
* - updateMdAllNew
* - appendMdAllNew
* - updateMdSomeNew
* - updateMdSomeNew
* - appendValueAndMd
* - updateValueAndMd
* - appendMdNoActualChanges
* - updateMdNoActualChanges
*
* (N=2 without loss of generality)
*
* With isPattern=true:
*
* - patternUnsupported
*
* ?!exist=entity::type filter
*
* - notExistFilter
*
* Simulating fails in MongoDB connection:
*
* - mongoDbUpdateFail
* - mongoDbQueryFail
*
* Lastly a few tests with Service Path
*
* - servicePathEntityUpdate_3levels
* - servicePathEntityUpdate_2levels
* - servicePathEntityAppend_3levels
* - servicePathEntityAppend_2levels
* - servicePathEntityCreation_2levels
* - servicePathEntityCreation_3levels
* - servicePathEntityDeletion_2levels
* - servicePathEntityDeletion_3levels
* - servicePathEntityVectorNotAllowed
*
* Note these tests are not "canonical" unit tests. Canon says that in this case we should have
* mocked MongoDB. Actually, we think is very much powerful to check that everything is ok at
* MongoDB layer.
*
*/

/* ****************************************************************************
*
* prepareDatabase -
*
* This function is called before every test, to populate some information in the
* entities collection.
*/
static void prepareDatabase(void) {

  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();

  /* We create the following entities:
   *
   * - E1:
   *     A1: val1
   *     A2: (no value)
   * - E2
   *     A3: val3
   *     A4: (no value)
   * - E3
   *     A5: val5
   *     A6: (no value)
   * - E1*:
   *     A1: val1bis2
   * - E1**:
   *     A1: val1
   *     A2: (no value)
   *
   * (*) Means that entity/type is using same name but different type. This is included to check that type is
   *     taken into account.
   *
   * (**)same name but without type
   *
   */

  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T1") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "A1" << "type" << "TA1" << "value" << "val1") <<
                        BSON("name" << "A2" << "type" << "TA2")
                        )
                    );

  BSONObj en2 = BSON("_id" << BSON("id" << "E2" << "type" << "T2") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "A3" << "type" << "TA3" << "value" << "val3") <<
                        BSON("name" << "A4" << "type" << "TA4")
                        )
                    );

  BSONObj en3 = BSON("_id" << BSON("id" << "E3" << "type" << "T3") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "A5" << "type" << "TA5" << "value" << "val5") <<
                        BSON("name" << "A6" << "type" << "TA6")
                        )
                    );

  BSONObj en4 = BSON("_id" << BSON("id" << "E1" << "type" << "T1bis") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "A1" << "type" << "TA1" << "value" << "val1bis2")
                        )
                    );

  BSONObj en1nt = BSON("_id" << BSON("id" << "E1") <<
                     "attrs" << BSON_ARRAY(
                        BSON("name" << "A1" << "type" << "TA1" << "value" << "val1-nt") <<
                        BSON("name" << "A2" << "type" << "TA2")
                        )
                    );

  connection->insert(ENTITIES_COLL, en1);
  connection->insert(ENTITIES_COLL, en2);
  connection->insert(ENTITIES_COLL, en3);
  connection->insert(ENTITIES_COLL, en4);
  connection->insert(ENTITIES_COLL, en1nt);

}

/* ****************************************************************************
*
* prepareDatabaseMd -
*
*/
static void prepareDatabaseMd(void) {

    /* Set database */
    setupDatabase();

    /* Add an entity with custom metadata */
    DBClientBase* connection = getMongoConnection();
    BSONObj en = BSON("_id" << BSON("id" << "E1" << "type" << "T1") <<
                       "attrs" << BSON_ARRAY(
                          BSON("name" << "A1" << "type" << "TA1" << "value" << "val1" <<
                               "md" << BSON_ARRAY(BSON("name" << "MD1" << "type" << "TMD1" << "value" << "MD1val") <<
                                                  BSON("name" << "MD2" << "type" << "TMD2" << "value" << "MD2val")
                                                 )
                               )
                          )
                      );

    connection->insert(ENTITIES_COLL, en);

}

/* ****************************************************************************
*
* prepareDatabaseWithAttributeIds -
*
* This function is called before every test, to populate some information in the
* entities collection.
*/
static void prepareDatabaseWithAttributeIds(void) {

    /* Start with the base entities */
    prepareDatabase();

    /* Add some entities with metadata ID */

    DBClientBase* connection = getMongoConnection();
    BSONObj en = BSON("_id" << BSON("id" << "E10" << "type" << "T10") <<
                       "attrs" << BSON_ARRAY(
                          BSON("name" << "A1" << "type" << "TA1" << "value" << "val11" << "id" << "ID1") <<
                          BSON("name" << "A1" << "type" << "TA1" << "value" << "val12" << "id" << "ID2") <<
                          BSON("name" << "A2" << "type" << "TA2" << "value" << "val2")
                          )
                      );
    connection->insert(ENTITIES_COLL, en);

}

/* ****************************************************************************
*
* prepareDatabaseWithServicePaths -
*
* This function is called before every test, to populate some information in the
* entities collection.
*/
static void prepareDatabaseWithServicePaths(void) {

    /* Set database */
    setupDatabase();


    /* Create entities with service path */
    DBClientBase* connection = getMongoConnection();
    BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T1" << "servicePath" << "/home/kz/01") <<
                       "attrs" << BSON_ARRAY(
                          BSON("name" << "A1" << "type" << "TA1" << "value" << "kz01")
                          )
                      );

    BSONObj en2 = BSON("_id" << BSON("id" << "E1" << "type" << "T1" << "servicePath" << "/home/kz/02") <<
                       "attrs" << BSON_ARRAY(
                          BSON("name" << "A1" << "type" << "TA1" << "value" << "kz02")
                          )
                      );
    connection->insert(ENTITIES_COLL, en1);
    connection->insert(ENTITIES_COLL, en2);

}

/* ****************************************************************************
*
* getAttr -
*
* We need this function because we can not trust on array index, at mongo will
* not sort the elements within the array. This function assumes that always will
* find a result, that is ok for testing code.
*/
static BSONObj getAttr(std::vector<BSONElement> attrs, std::string name, std::string type, std::string id = "") {

    BSONElement be;
    for (unsigned int ix = 0; ix < attrs.size(); ++ix) {
        BSONObj attr = attrs[ix].embeddedObject();
        std::string attrName = STR_FIELD(attr, "name");
        std::string attrType = STR_FIELD(attr, "type");
        std::string attrId = STR_FIELD(attr, "id");
        if (attrName == name && attrType == type && ( id == "" || attrId == id )) {
            be = attrs[ix];
            break;
        }
    }
    return be.embeddedObject();

}

/* ****************************************************************************
*
* update1Ent1Attr -
*/
TEST(mongoUpdateContextRequest, update1Ent1Attr)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A1", "TA1", "new_val");

    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();   
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));    
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));    
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));    
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");    
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();
}

/* ****************************************************************************
*
* update1Ent1AttrNoType -
*/
TEST(mongoUpdateContextRequest, update1Ent1AttrNoType)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A1", "", "new_val");

    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();    
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->type.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");    
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* update1EntNoType1Attr -
*/
TEST(mongoUpdateContextRequest, update1EntNoType1Attr)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "", "false");
    ContextAttribute ca("A1", "TA1", "new_val");

    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();   
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(3, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E1", RES_CER(1).entityId.id);
    EXPECT_EQ("T1bis", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Context Element response # 3 */
    EXPECT_EQ("E1", RES_CER(2).entityId.id);
    EXPECT_EQ(0, RES_CER(2).entityId.type.size());
    EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(2).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(2, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(2, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(2, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");    
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* update1EntNoType1AttrNoType -
*/
TEST(mongoUpdateContextRequest, update1EntNoType1AttrNoType)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "", "false");
    ContextAttribute ca("A1", "", "new_val");

    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();    
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(3, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->type.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E1", RES_CER(1).entityId.id);
    EXPECT_EQ("T1bis", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->type.size());
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Context Element response # 3 */
    EXPECT_EQ("E1", RES_CER(2).entityId.id);
    EXPECT_EQ(0, RES_CER(2).entityId.type.size());
    EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(2).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(2, 0)->name);
    EXPECT_EQ(0, RES_CER_ATTR(2, 0)->type.size());
    EXPECT_EQ(0, RES_CER_ATTR(2, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(2, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* updateNEnt1Attr -
*/
TEST(mongoUpdateContextRequest, updateNEnt1Attr)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce1, ce2;
    ce1.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "new_val1");
    ce2.entityId.fill("E2", "T2", "false");
    ContextAttribute ca2("A3", "TA3", "new_val3");
    ce1.contextAttributeVector.push_back(&ca1);
    ce2.contextAttributeVector.push_back(&ca2);
    req.contextElementVector.push_back(&ce1);
    req.contextElementVector.push_back(&ce2);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();    
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T2", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern) << "wrong entity isPattern (context element response #2)";
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A3", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val1", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("new_val3", C_STR_FIELD(a3, "value"));
    EXPECT_EQ(1360232700, a3.getIntField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* update1EntNAttr -
*/
TEST(mongoUpdateContextRequest, update1EntNAttr)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "new_val1");
    ContextAttribute ca2("A2", "TA2", "new_val2");
    ce.contextAttributeVector.push_back(&ca1);
    ce.contextAttributeVector.push_back(&ca2);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();   
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());


    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");    
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val1", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("new_val2", C_STR_FIELD(a2, "value"));
    EXPECT_EQ(1360232700, a2.getIntField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* updateNEntNAttr -
*/
TEST(mongoUpdateContextRequest, updateNEntNAttr)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;   

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce1, ce2;
    ce1.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "new_val1");
    ContextAttribute ca2("A2", "TA2", "new_val2");
    ce2.entityId.fill("E2", "T2", "false");
    ContextAttribute ca3("A3", "TA3", "new_val3");
    ContextAttribute ca4("A4", "TA4", "new_val4");
    ce1.contextAttributeVector.push_back(&ca1);
    ce1.contextAttributeVector.push_back(&ca2);
    ce2.contextAttributeVector.push_back(&ca3);
    ce2.contextAttributeVector.push_back(&ca4);
    req.contextElementVector.push_back(&ce1);
    req.contextElementVector.push_back(&ce2);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T2", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern) << "wrong entity isPattern (context element response #2)";
    ASSERT_EQ(2, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A3", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->metadataVector.size());
    EXPECT_EQ("A4", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA4", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(1, 1)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(1, 1)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val1", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("new_val2", C_STR_FIELD(a2, "value"));
    EXPECT_EQ(1360232700, a2.getIntField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("new_val3", C_STR_FIELD(a3, "value"));
    EXPECT_EQ(1360232700, a3.getIntField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_STREQ("new_val4", C_STR_FIELD(a4, "value"));
    EXPECT_EQ(1360232700, a4.getIntField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* append1Ent1Attr -
*/
TEST(mongoUpdateContextRequest, append1Ent1Attr)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A8", "TA8", "val8");
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A8", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA8", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(3, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    BSONObj a8 = getAttr(attrs, "A8", "TA8");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A8", C_STR_FIELD(a8, "name"));
    EXPECT_STREQ("TA8", C_STR_FIELD(a8, "type"));
    EXPECT_STREQ("val8", C_STR_FIELD(a8, "value"));
    EXPECT_EQ(1360232700, a8.getIntField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* append1Ent1AttrNoType -
*/
TEST(mongoUpdateContextRequest, append1Ent1AttrNoType)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A8", "", "val8");
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A8", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->type.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(3, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    BSONObj a8 = getAttr(attrs, "A8", "");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A8", C_STR_FIELD(a8, "name"));
    EXPECT_STREQ("", C_STR_FIELD(a8, "type"));
    EXPECT_STREQ("val8", C_STR_FIELD(a8, "value"));
    EXPECT_EQ(1360232700, a8.getIntField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* append1EntNoType1Attr -
*/
TEST(mongoUpdateContextRequest, append1EntNoType1Attr)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "", "false");
    ContextAttribute ca("A8", "TA8", "val8");
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(3, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A8", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA8", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E1", RES_CER(1).entityId.id);
    EXPECT_EQ("T1bis", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A8", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA8", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Context Element response # 3 */
    EXPECT_EQ("E1", RES_CER(2).entityId.id);
    EXPECT_EQ(0, RES_CER(2).entityId.type.size());
    EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(2).contextAttributeVector.size());
    EXPECT_EQ("A8", RES_CER_ATTR(2, 0)->name);
    EXPECT_EQ("TA8", RES_CER_ATTR(2, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(2, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(2, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(3, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    BSONObj a8 = getAttr(attrs, "A8", "TA8");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A8", C_STR_FIELD(a8, "name"));
    EXPECT_STREQ("TA8", C_STR_FIELD(a8, "type"));
    EXPECT_STREQ("val8", C_STR_FIELD(a8, "value"));
    EXPECT_EQ(1360232700, a8.getIntField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a8 = getAttr(attrs, "A8", "TA8");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A8", C_STR_FIELD(a8, "name"));
    EXPECT_STREQ("TA8", C_STR_FIELD(a8, "type"));
    EXPECT_STREQ("val8", C_STR_FIELD(a8, "value"));
    EXPECT_EQ(1360232700, a8.getIntField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(3, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    a8 = getAttr(attrs, "A8", "TA8");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A8", C_STR_FIELD(a8, "name"));
    EXPECT_STREQ("TA8", C_STR_FIELD(a8, "type"));
    EXPECT_STREQ("val8", C_STR_FIELD(a8, "value"));
    EXPECT_EQ(1360232700, a8.getIntField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* append1EntNoType1AttrNoType -
*/
TEST(mongoUpdateContextRequest, append1EntNoType1AttrNoType)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "", "false");
    ContextAttribute ca("A8", "", "val8");
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();   
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(3, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A8", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->type.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E1", RES_CER(1).entityId.id);
    EXPECT_EQ("T1bis", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A8", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->type.size());
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Context Element response # 3 */
    EXPECT_EQ("E1", RES_CER(2).entityId.id);
    EXPECT_EQ(0, RES_CER(2).entityId.type.size());
    EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(2).contextAttributeVector.size());
    EXPECT_EQ("A8", RES_CER_ATTR(2, 0)->name);
    EXPECT_EQ(0, RES_CER_ATTR(2, 0)->type.size());
    EXPECT_EQ(0, RES_CER_ATTR(2, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(2, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(3, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    BSONObj a8 = getAttr(attrs, "A8", "");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    EXPECT_STREQ("A8", C_STR_FIELD(a8, "name"));
    EXPECT_STREQ("", C_STR_FIELD(a8, "type"));
    EXPECT_STREQ("val8", C_STR_FIELD(a8, "value"));
    EXPECT_EQ(1360232700, a8.getIntField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a8 = getAttr(attrs, "A8", "");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A8", C_STR_FIELD(a8, "name"));
    EXPECT_STREQ("", C_STR_FIELD(a8, "type"));
    EXPECT_STREQ("val8", C_STR_FIELD(a8, "value"));
    EXPECT_EQ(1360232700, a8.getIntField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(3, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");    
    a8 = getAttr(attrs, "A8", "");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A8", C_STR_FIELD(a8, "name"));
    EXPECT_STREQ("", C_STR_FIELD(a8, "type"));
    EXPECT_STREQ("val8", C_STR_FIELD(a8, "value"));
    EXPECT_EQ(1360232700, a8.getIntField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* appendNEnt1Attr -
*/
TEST(mongoUpdateContextRequest, appendNEnt1Attr)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce1, ce2;
    ce1.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A8", "TA8", "val8");
    ce2.entityId.fill("E2", "T2", "false");
    ContextAttribute ca2("A9", "TA9", "val9");
    ce1.contextAttributeVector.push_back(&ca1);
    ce2.contextAttributeVector.push_back(&ca2);
    req.contextElementVector.push_back(&ce1);
    req.contextElementVector.push_back(&ce2);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A8", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA8", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T2", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A9", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA9", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(3, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    BSONObj a8 = getAttr(attrs, "A8", "TA8");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A8", C_STR_FIELD(a8, "name"));
    EXPECT_STREQ("TA8", C_STR_FIELD(a8, "type"));
    EXPECT_STREQ("val8", C_STR_FIELD(a8, "value"));
    EXPECT_EQ(1360232700, a8.getIntField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(3, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    BSONObj a9 = getAttr(attrs, "A9", "TA9");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));
    EXPECT_STREQ("A9", C_STR_FIELD(a9, "name"));
    EXPECT_STREQ("TA9", C_STR_FIELD(a9, "type"));
    EXPECT_STREQ("val9", C_STR_FIELD(a9, "value"));
    EXPECT_EQ(1360232700, a9.getIntField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* append1EntNAttr -
*/
TEST(mongoUpdateContextRequest, append1EntNAttr)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A8", "TA8", "val8");
    ContextAttribute ca2("A9", "TA9", "val9");
    ce.contextAttributeVector.push_back(&ca1);
    ce.contextAttributeVector.push_back(&ca2);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();    
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A8", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA8", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("A9", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA9", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(4, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    BSONObj a8 = getAttr(attrs, "A8", "TA8");
    BSONObj a9 = getAttr(attrs, "A9", "TA9");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A8", C_STR_FIELD(a8, "name"));
    EXPECT_STREQ("TA8", C_STR_FIELD(a8, "type"));
    EXPECT_STREQ("val8", C_STR_FIELD(a8, "value"));
    EXPECT_EQ(1360232700, a8.getIntField("modDate"));
    EXPECT_STREQ("A9", C_STR_FIELD(a9, "name"));
    EXPECT_STREQ("TA9", C_STR_FIELD(a9, "type"));
    EXPECT_STREQ("val9", C_STR_FIELD(a9, "value"));
    EXPECT_EQ(1360232700, a9.getIntField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* appendNEntNAttr -
*/
TEST(mongoUpdateContextRequest, appendNEntNAttr)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce1, ce2;
    ce1.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A8", "TA8", "val8");
    ContextAttribute ca2("A9", "TA9", "val9");
    ce2.entityId.fill("E2", "T2", "false");
    ContextAttribute ca3("A10", "TA10", "val10");
    ContextAttribute ca4("A11", "TA11", "val11");
    ce1.contextAttributeVector.push_back(&ca1);
    ce1.contextAttributeVector.push_back(&ca2);
    ce2.contextAttributeVector.push_back(&ca3);
    ce2.contextAttributeVector.push_back(&ca4);
    req.contextElementVector.push_back(&ce1);
    req.contextElementVector.push_back(&ce2);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();    
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A8", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA8", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("A9", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA9", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T2", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A10", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA10", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->metadataVector.size());
    EXPECT_EQ("A11", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA11", RES_CER_ATTR(1, 1)->type);    
    EXPECT_EQ(0, RES_CER_ATTR(1, 1)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(1, 1)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());


    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(4, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    BSONObj a8 = getAttr(attrs, "A8", "TA8");
    BSONObj a9 = getAttr(attrs, "A9", "TA9");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A8", C_STR_FIELD(a8, "name"));
    EXPECT_STREQ("TA8", C_STR_FIELD(a8, "type"));
    EXPECT_STREQ("val8", C_STR_FIELD(a8, "value"));
    EXPECT_EQ(1360232700, a8.getIntField("modDate"));
    EXPECT_STREQ("A9", C_STR_FIELD(a9, "name"));
    EXPECT_STREQ("TA9", C_STR_FIELD(a9, "type"));
    EXPECT_STREQ("val9", C_STR_FIELD(a9, "value"));
    EXPECT_EQ(1360232700, a9.getIntField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(4, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    BSONObj a10 = getAttr(attrs, "A10", "TA10");
    BSONObj a11 = getAttr(attrs, "A11", "TA11");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));
    EXPECT_STREQ("A10", C_STR_FIELD(a10, "name"));
    EXPECT_STREQ("TA10", C_STR_FIELD(a10, "type"));
    EXPECT_STREQ("val10", C_STR_FIELD(a10, "value"));
    EXPECT_EQ(1360232700, a10.getIntField("modDate"));
    EXPECT_STREQ("A11", C_STR_FIELD(a11, "name"));
    EXPECT_STREQ("TA11", C_STR_FIELD(a11, "type"));
    EXPECT_STREQ("val11", C_STR_FIELD(a11, "value"));
    EXPECT_EQ(1360232700, a11.getIntField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* delete1Ent0Attr -
*/
TEST(mongoUpdateContextRequest, delete1Ent0Attr)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("DELETE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();   
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern) << "wrong entity isPattern (context element response #1)";
    ASSERT_EQ(0, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(4, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* delete1Ent1Attr -
*/
TEST(mongoUpdateContextRequest, delete1Ent1Attr)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A2", "TA2");
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("DELETE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();    
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* delete1Ent1AttrNoType -
*/
TEST(mongoUpdateContextRequest, delete1Ent1AttrNoType)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A2", "");
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("DELETE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();    
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->type.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* delete1EntNoType0Attr -
*/
TEST(mongoUpdateContextRequest, delete1EntNoType0Attr)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "", "false");
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("DELETE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(3, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(0, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E1", RES_CER(1).entityId.id);
    EXPECT_EQ("T1bis", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(0, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Context Element response # 3 */
    EXPECT_EQ("E1", RES_CER(2).entityId.id);
    EXPECT_EQ(0, RES_CER(2).entityId.type.size());
    EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
    ASSERT_EQ(0, RES_CER(2).contextAttributeVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* delete1EntNoType1Attr -
*/
TEST(mongoUpdateContextRequest, delete1EntNoType1Attr)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "", "false");
    ContextAttribute ca("A2", "TA2");
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("DELETE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(3, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E1", RES_CER(1).entityId.id);
    EXPECT_EQ("T1bis", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->metadataVector.size());
    EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(1).code);
    EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ("action: DELETE - entity: [E1, T1bis] - offending attribute: A2 - attribute not found", RES_CER_STATUS(1).details);

    /* Context Element response # 3 */
    EXPECT_EQ("E1", RES_CER(2).entityId.id);
    EXPECT_EQ(0, RES_CER(2).entityId.type.size());
    EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(2).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(2, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(2, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(2, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(2, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* delete1EntNoType1AttrNoType -
*/
TEST(mongoUpdateContextRequest, delete1EntNoType1AttrNoType)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "", "false");
    ContextAttribute ca("A2", "");
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("DELETE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(3, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->type.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E1", RES_CER(1).entityId.id);
    EXPECT_EQ("T1bis", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->type.size());
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->metadataVector.size());
    EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(1).code);
    EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ("action: DELETE - entity: [E1, T1bis] - offending attribute: A2 - attribute not found", RES_CER_STATUS(1).details);

    /* Context Element response # 3 */
    EXPECT_EQ("E1", RES_CER(2).entityId.id);
    EXPECT_EQ(0, RES_CER(2).entityId.type.size());
    EXPECT_EQ("false", RES_CER(2).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(2).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(2, 0)->name);
    EXPECT_EQ(0, RES_CER_ATTR(2, 0)->type.size());
    EXPECT_EQ(0, RES_CER_ATTR(2, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(2, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(2).code);
    EXPECT_EQ("OK", RES_CER_STATUS(2).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(2).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");    
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* deleteNEnt1Attr -
*/
TEST(mongoUpdateContextRequest, deleteNEnt1Attr)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce1, ce2;
    ce1.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A2", "TA2");
    ce2.entityId.fill("E2", "T2", "false");
    ContextAttribute ca2("A4", "TA4");
    ce1.contextAttributeVector.push_back(&ca1);
    ce2.contextAttributeVector.push_back(&ca2);
    req.contextElementVector.push_back(&ce1);
    req.contextElementVector.push_back(&ce2);
    req.updateActionType.set("DELETE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T2", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern) << "wrong entity isPattern (context element response #2)";
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A4", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA4", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();


}

/* ****************************************************************************
*
* delete1EntNAttr -
*/
TEST(mongoUpdateContextRequest, delete1EntNAttr)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1");
    ContextAttribute ca2("A2", "TA2");
    ce.contextAttributeVector.push_back(&ca1);
    ce.contextAttributeVector.push_back(&ca2);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("DELETE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(0, attrs.size());

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* deleteNEntNAttr -
*/
TEST(mongoUpdateContextRequest, deleteNEntNAttr)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce1, ce2;
    ce1.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1");
    ContextAttribute ca2("A2", "TA2");
    ce2.entityId.fill("E2", "T2", "false");
    ContextAttribute ca3("A3", "TA3");
    ContextAttribute ca4("A4", "TA4");
    ce1.contextAttributeVector.push_back(&ca1);
    ce1.contextAttributeVector.push_back(&ca2);
    ce2.contextAttributeVector.push_back(&ca3);
    ce2.contextAttributeVector.push_back(&ca4);
    req.contextElementVector.push_back(&ce1);
    req.contextElementVector.push_back(&ce2);
    req.updateActionType.set("DELETE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();   
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 2 */
    EXPECT_EQ("E2", RES_CER(1).entityId.id);
    EXPECT_EQ("T2", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A3", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->metadataVector.size());
    EXPECT_EQ("A4", RES_CER_ATTR(1, 1)->name);
    EXPECT_EQ("TA4", RES_CER_ATTR(1, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(1, 1)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(1, 1)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(0, attrs.size());

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));    
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    EXPECT_EQ(0, attrs.size());

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* updateEntityFails -
*/
TEST(mongoUpdateContextRequest, updateEntityFails)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E4", "T4", "false");
    ContextAttribute ca("A1", "TA1", "new_val");
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E4", RES_CER(0).entityId.id);
    EXPECT_EQ("T4", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(0, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ(SccContextElementNotFound, RES_CER_STATUS(0).code);
    EXPECT_EQ("No context element found", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("E4", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* createEntity -
*/
TEST(mongoUpdateContextRequest, createEntity)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E4", "T4", "false");
    ContextAttribute ca("A1", "TA1", "new_val");
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();    
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E4", RES_CER(0).entityId.id);
    EXPECT_EQ("T4", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);    
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E4" << "_id.type" << "T4"));
    EXPECT_STREQ("E4", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T4", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_TRUE(ent.hasField("creDate"));
    EXPECT_TRUE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_TRUE(a1.hasField("creDate"));
    EXPECT_TRUE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* createEntityWithId -
*/
TEST(mongoUpdateContextRequest, createEntityWithId)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E4", "T4", "false");
    ContextAttribute ca("A1", "TA1", "new_val");
    Metadata md("ID", "string", "ID1");
    ca.metadataVector.push_back(&md);
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E4", RES_CER(0).entityId.id);
    EXPECT_EQ("T4", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("ID1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E4" << "_id.type" << "T4"));
    EXPECT_STREQ("E4", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T4", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_TRUE(ent.hasField("creDate"));
    EXPECT_TRUE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_STREQ("ID1", C_STR_FIELD(a1, "id"));
    EXPECT_TRUE(a1.hasField("creDate"));
    EXPECT_TRUE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* createEntityMixIdNoIdFails -
*/
TEST(mongoUpdateContextRequest, createEntityMixIdNoIdFails)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E4", "T4", "false");
    ContextAttribute ca1("A1", "TA1", "new_val");
    Metadata md("ID", "string", "ID1");
    ca1.metadataVector.push_back(&md);
    ce.contextAttributeVector.push_back(&ca1);
    ContextAttribute ca2("A1", "TA1", "new_val2");
    ce.contextAttributeVector.push_back(&ca2);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();    
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E4", RES_CER(0).entityId.id);
    EXPECT_EQ("T4", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("ID1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ("A1", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->value.size());
    ASSERT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(0).code);
    EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("Attributes with same name with ID and not ID at the same time in the same entity are forbidden: entity: [E4, T4]", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();
}

/* ****************************************************************************
*
* updateEmptyValueFail -
*/
TEST(mongoUpdateContextRequest, updateEmptyValueFail)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A1", "TA1");
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(0).code);
    EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("action: UPDATE - entity: [E1, T1, false] - offending attribute: A1 - empty attribute not allowed in APPEND or UPDATE", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* appendEmptyValueFail -
*/
TEST(mongoUpdateContextRequest, appendEmptyValueFail)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A8", "TA8");
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */      
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A8", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA8", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(0).code);
    EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("action: APPEND - entity: [E1, T1, false] - offending attribute: A8 - empty attribute not allowed in APPEND or UPDATE", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));   

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* updateAttrNotFoundFail -
*/
TEST(mongoUpdateContextRequest, updateAttrNotFoundFail)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A8", "TA8", "new_val8");
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A8", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA8", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(0).code);
    EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("action: UPDATE - entity: [E1, T1] - offending attribute: A8", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* deleteAttrNotFoundFail -
*/
TEST(mongoUpdateContextRequest, deleteAttrNotFoundFail)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A8", "TA8");
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("DELETE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();   
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A8", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA8", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(0).code);
    EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("action: DELETE - entity: [E1, T1] - offending attribute: A8 - attribute not found", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* mixUpdateAndCreate -
*/
TEST(mongoUpdateContextRequest, mixUpdateAndCreate)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce1, ce2;
    ce1.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "new_val1");
    ce2.entityId.fill("E5", "T5", "false");
    ContextAttribute ca2("A3", "TA3", "new_val13");
    ce1.contextAttributeVector.push_back(&ca1);
    ce2.contextAttributeVector.push_back(&ca2);
    req.contextElementVector.push_back(&ce1);
    req.contextElementVector.push_back(&ce2);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response #1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response #2 (create) */
    EXPECT_EQ("E5", RES_CER(1).entityId.id);
    EXPECT_EQ("T5", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ("A3", RES_CER_ATTR(1, 0)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(1, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(1, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val1", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E5" << "_id.type" << "T5"));
    EXPECT_STREQ("E5", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T5", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_TRUE(ent.hasField("creDate"));
    EXPECT_TRUE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a3 = getAttr(attrs, "A3", "TA3");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3",C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("new_val13", C_STR_FIELD(a3, "value"));
    EXPECT_TRUE(a3.hasField("creDate"));
    EXPECT_TRUE(a3.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* appendExistingAttr -
*/
TEST(mongoUpdateContextRequest, appendExistingAttr)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A1", "TA1", "new_val");
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* updateAttrWithId -
*/
TEST(mongoUpdateContextRequest, updateAttrWithId)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithAttributeIds();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E10", "T10", "false");
    ContextAttribute ca("A1", "TA1", "new_val");
    Metadata md("ID", "string", "ID1");
    ca.metadataVector.push_back(&md);
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T10", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("ID1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_FALSE(a3.hasField("id"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));
    EXPECT_FALSE(a4.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_FALSE(a5.hasField("id"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));
    EXPECT_FALSE(a6.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E10" << "_id.type" << "T10"));
    EXPECT_STREQ("E10", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T10", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(3, attrs.size());
    BSONObj a1id1 = getAttr(attrs, "A1", "TA1", "ID1");
    BSONObj a1id2 = getAttr(attrs, "A1", "TA1", "ID2");    
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1id1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1id1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1id1, "value"));
    EXPECT_STREQ("ID1", C_STR_FIELD(a1id1, "id"));
    EXPECT_EQ(1360232700, a1id1.getIntField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1id2, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1id2, "type"));
    EXPECT_STREQ("val12", C_STR_FIELD(a1id2, "value"));
    EXPECT_STREQ("ID2", C_STR_FIELD(a1id2, "id"));
    EXPECT_FALSE(a1id2.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2",C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("val2", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* updateAttrWithAndWithoutId -
*/
TEST(mongoUpdateContextRequest, updateAttrWithAndWithoutId)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithAttributeIds();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E10", "T10", "false");
    ContextAttribute ca1("A1", "TA1", "new_val");
    Metadata md("ID", "string", "ID1");
    ca1.metadataVector.push_back(&md);
    ce.contextAttributeVector.push_back(&ca1);
    ContextAttribute ca2("A2", "TA2", "new_val2");
    ce.contextAttributeVector.push_back(&ca2);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T10", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("ID1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->value.size());
    ASSERT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_FALSE(a3.hasField("id"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));
    EXPECT_FALSE(a4.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_FALSE(a5.hasField("id"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));
    EXPECT_FALSE(a6.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E10" << "_id.type" << "T10"));
    EXPECT_STREQ("E10", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T10", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(3, attrs.size());
    BSONObj a1id1 = getAttr(attrs, "A1", "TA1", "ID1");
    BSONObj a1id2 = getAttr(attrs, "A1", "TA1", "ID2");    
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1id1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1id1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1id1, "value"));
    EXPECT_STREQ("ID1", C_STR_FIELD(a1id1, "id"));
    EXPECT_EQ(1360232700, a1id1.getIntField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1id2, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1id2, "type"));
    EXPECT_STREQ("val12", C_STR_FIELD(a1id2, "value"));
    EXPECT_STREQ("ID2", C_STR_FIELD(a1id2, "id"));
    EXPECT_FALSE(a1id2.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2",C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("new_val2", C_STR_FIELD(a2, "value"));
    EXPECT_EQ(1360232700, a2.getIntField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* appendAttrWithId -
*/
TEST(mongoUpdateContextRequest, appendAttrWithId)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithAttributeIds();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E10", "T10", "false");
    ContextAttribute ca("A1", "TA1", "new_val");
    Metadata md("ID", "string", "ID3");
    ca.metadataVector.push_back(&md);
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T10", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("ID3", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_FALSE(a3.hasField("id"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));
    EXPECT_FALSE(a4.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_FALSE(a5.hasField("id"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));
    EXPECT_FALSE(a6.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E10" << "_id.type" << "T10"));
    EXPECT_STREQ("E10", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T10", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(4, attrs.size());
    BSONObj a1id1 = getAttr(attrs, "A1", "TA1", "ID1");
    BSONObj a1id2 = getAttr(attrs, "A1", "TA1", "ID2");
    BSONObj a1id3 = getAttr(attrs, "A1", "TA1", "ID3");    
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1id1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1id1, "type"));
    EXPECT_STREQ("val11", C_STR_FIELD(a1id1, "value"));
    EXPECT_STREQ("ID1", C_STR_FIELD(a1id1, "id"));
    EXPECT_FALSE(a1id1.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1id2, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1id2, "type"));
    EXPECT_STREQ("val12", C_STR_FIELD(a1id2, "value"));
    EXPECT_STREQ("ID2", C_STR_FIELD(a1id2, "id"));
    EXPECT_FALSE(a1id2.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1id3, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1id3, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1id3, "value"));
    EXPECT_STREQ("ID3", C_STR_FIELD(a1id3, "id"));
    EXPECT_EQ(1360232700, a1id3.getIntField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2",C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("val2", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* appendAttrWithAndWithoutId -
*/
TEST(mongoUpdateContextRequest, appendAttrWithAndWithoutId)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithAttributeIds();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E10", "T10", "false");
    ContextAttribute ca1("A1", "TA1", "new_val");
    Metadata md("ID", "string", "ID3");
    ca1.metadataVector.push_back(&md);
    ce.contextAttributeVector.push_back(&ca1);
    ContextAttribute ca2("A3", "TA3", "new_val3");
    ce.contextAttributeVector.push_back(&ca2);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T10", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("ID3", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ("A3", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->value.size());
    ASSERT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_FALSE(a3.hasField("id"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));
    EXPECT_FALSE(a4.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_FALSE(a5.hasField("id"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));
    EXPECT_FALSE(a6.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E10" << "_id.type" << "T10"));
    EXPECT_STREQ("E10", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T10", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(5, attrs.size());
    BSONObj a1id1 = getAttr(attrs, "A1", "TA1", "ID1");
    BSONObj a1id2 = getAttr(attrs, "A1", "TA1", "ID2");
    BSONObj a1id3 = getAttr(attrs, "A1", "TA1", "ID3");
    a2 = getAttr(attrs, "A2", "TA2");
    a3 = getAttr(attrs, "A3", "TA3");
    EXPECT_STREQ("A1", C_STR_FIELD(a1id1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1id1, "type"));
    EXPECT_STREQ("val11", C_STR_FIELD(a1id1, "value"));
    EXPECT_STREQ("ID1", C_STR_FIELD(a1id1, "id"));
    EXPECT_FALSE(a1id1.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1id2, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1id2, "type"));
    EXPECT_STREQ("val12", C_STR_FIELD(a1id2, "value"));
    EXPECT_STREQ("ID2", C_STR_FIELD(a1id2, "id"));
    EXPECT_FALSE(a1id2.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1id3, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1id3, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1id3, "value"));
    EXPECT_STREQ("ID3", C_STR_FIELD(a1id3, "id"));
    EXPECT_EQ(1360232700, a1id3.getIntField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2",C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("val2", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3",C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("new_val3", C_STR_FIELD(a3, "value"));
    EXPECT_EQ(1360232700, a3.getIntField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* appendAttrWithIdFails -
*/
TEST(mongoUpdateContextRequest, appendAttrWithIdFails)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithAttributeIds();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E10", "T10", "false");
    ContextAttribute ca("A2", "TA2", "new_val");
    Metadata md("ID", "string", "IDX");
    ca.metadataVector.push_back(&md);
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T10", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("IDX", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(0).code);
    EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("action: APPEND - entity: [E10, T10] - offending attribute: A2 - attribute can not be appended", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_FALSE(a3.hasField("id"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));
    EXPECT_FALSE(a4.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_FALSE(a5.hasField("id"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));
    EXPECT_FALSE(a6.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E10" << "_id.type" << "T10"));
    EXPECT_STREQ("E10", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T10", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(3, attrs.size());
    BSONObj a1id1 = getAttr(attrs, "A1", "TA1", "ID1");
    BSONObj a1id2 = getAttr(attrs, "A1", "TA1", "ID2");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1id1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1id1, "type"));
    EXPECT_STREQ("val11", C_STR_FIELD(a1id1, "value"));
    EXPECT_STREQ("ID1", C_STR_FIELD(a1id1, "id"));
    EXPECT_FALSE(a1id1.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1id2, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1id2, "type"));
    EXPECT_STREQ("val12", C_STR_FIELD(a1id2, "value"));
    EXPECT_STREQ("ID2", C_STR_FIELD(a1id2, "id"));
    EXPECT_FALSE(a1id2.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2",C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("val2", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* appendAttrWithoutIdFails -
*/
TEST(mongoUpdateContextRequest, appendAttrWithoutIdFails)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithAttributeIds();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E10", "T10", "false");
    ContextAttribute ca("A1", "TA1", "new_val");
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();    
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T10", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    ASSERT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(0).code);
    EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("action: APPEND - entity: [E10, T10] - offending attribute: A1 - attribute can not be appended", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_FALSE(a3.hasField("id"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));
    EXPECT_FALSE(a4.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_FALSE(a5.hasField("id"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));
    EXPECT_FALSE(a6.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E10" << "_id.type" << "T10"));
    EXPECT_STREQ("E10", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T10", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(3, attrs.size());
    BSONObj a1id1 = getAttr(attrs, "A1", "TA1", "ID1");
    BSONObj a1id2 = getAttr(attrs, "A1", "TA1", "ID2");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1id1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1id1, "type"));
    EXPECT_STREQ("val11", C_STR_FIELD(a1id1, "value"));
    EXPECT_STREQ("ID1", C_STR_FIELD(a1id1, "id"));
    EXPECT_FALSE(a1id1.hasField("modDate"));
    EXPECT_STREQ("A1", C_STR_FIELD(a1id2, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1id2, "type"));
    EXPECT_STREQ("val12", C_STR_FIELD(a1id2, "value"));
    EXPECT_STREQ("ID2", C_STR_FIELD(a1id2, "id"));
    EXPECT_FALSE(a1id2.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2",C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("val2", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* deleteAttrWithId -
*/
TEST(mongoUpdateContextRequest, deleteAttrWithId)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithAttributeIds();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E10", "T10", "false");
    ContextAttribute ca("A1", "TA1");
    Metadata md("ID", "string", "ID1");
    ca.metadataVector.push_back(&md);
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("DELETE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T10", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("ID1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_FALSE(a3.hasField("id"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));
    EXPECT_FALSE(a4.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_FALSE(a5.hasField("id"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));
    EXPECT_FALSE(a6.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E10" << "_id.type" << "T10"));
    EXPECT_STREQ("E10", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T10", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1id2 = getAttr(attrs, "A1", "TA1", "ID2");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1id2, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1id2, "type"));
    EXPECT_STREQ("val12", C_STR_FIELD(a1id2, "value"));
    EXPECT_STREQ("ID2", C_STR_FIELD(a1id2, "id"));
    EXPECT_FALSE(a1id2.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2",C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("val2", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* deleteAttrWithAndWithoutId -
*/
TEST(mongoUpdateContextRequest, deleteAttrWithAndWithoutId)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithAttributeIds();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E10", "T10", "false");
    ContextAttribute ca1("A1", "TA1");
    Metadata md("ID", "string", "ID1");
    ca1.metadataVector.push_back(&md);
    ce.contextAttributeVector.push_back(&ca1);
    ContextAttribute ca2("A2", "TA2");
    ce.contextAttributeVector.push_back(&ca2);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("DELETE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E10", RES_CER(0).entityId.id);
    EXPECT_EQ("T10", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("ID", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("ID1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->value.size());
    ASSERT_EQ(0, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(6, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_FALSE(a3.hasField("id"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));
    EXPECT_FALSE(a4.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_FALSE(a5.hasField("id"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));
    EXPECT_FALSE(a6.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("id"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(a2.hasField("id"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E10" << "_id.type" << "T10"));
    EXPECT_STREQ("E10", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T10", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1id2 = getAttr(attrs, "A1", "TA1", "ID2");    
    EXPECT_STREQ("A1", C_STR_FIELD(a1id2, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1id2, "type"));
    EXPECT_STREQ("val12", C_STR_FIELD(a1id2, "value"));
    EXPECT_STREQ("ID2", C_STR_FIELD(a1id2, "id"));
    EXPECT_FALSE(a1id2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* appendCreateEntWithMd -
*/
TEST(mongoUpdateContextRequest, appendCreateEntWithMd)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare (a clean) database */
    setupDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A1", "TA1", "val1");
    Metadata md1("MD1", "TMD1", "MD1val");
    Metadata md2("MD2", "TMD2", "MD2val");
    ca.metadataVector.push_back(&md1);
    ca.metadataVector.push_back(&md2);
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    ASSERT_EQ(2, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("MD1val", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ("MD2", RES_CER_ATTR(0, 0)->metadataVector.get(1)->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(0, 0)->metadataVector.get(1)->type);
    EXPECT_EQ("MD2val", RES_CER_ATTR(0, 0)->metadataVector.get(1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_EQ(1360232700, a1.getIntField("creDate"));
    std::vector<BSONElement> mdV = a1.getField("md").Array();
    ASSERT_EQ(2, mdV.size());
    EXPECT_EQ("MD1", STR_FIELD(mdV[0].embeddedObject(), "name"));
    EXPECT_EQ("TMD1", STR_FIELD(mdV[0].embeddedObject(), "type"));
    EXPECT_EQ("MD1val", STR_FIELD(mdV[0].embeddedObject(), "value"));
    EXPECT_EQ("MD2", STR_FIELD(mdV[1].embeddedObject(), "name"));
    EXPECT_EQ("TMD2", STR_FIELD(mdV[1].embeddedObject(), "type"));
    EXPECT_EQ("MD2val", STR_FIELD(mdV[1].embeddedObject(), "value"));

    /* Release connection */
    mongoDisconnect();

    utExit();
}

/* ****************************************************************************
*
* appendMdAllExisting -
*/
TEST(mongoUpdateContextRequest, appendMdAllExisting)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseMd();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A1", "TA1", "val1");
    Metadata md("MD1", "TMD1", "new_val");
    ca.metadataVector.push_back(&md);
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("new_val", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_FALSE(a1.hasField("creDate"));
    std::vector<BSONElement> mdV = a1.getField("md").Array();
    ASSERT_EQ(2, mdV.size());
    EXPECT_EQ("MD1", STR_FIELD(mdV[0].embeddedObject(), "name"));
    EXPECT_EQ("TMD1", STR_FIELD(mdV[0].embeddedObject(), "type"));
    EXPECT_EQ("new_val", STR_FIELD(mdV[0].embeddedObject(), "value"));
    EXPECT_EQ("MD2", STR_FIELD(mdV[1].embeddedObject(), "name"));
    EXPECT_EQ("TMD2", STR_FIELD(mdV[1].embeddedObject(), "type"));
    EXPECT_EQ("MD2val", STR_FIELD(mdV[1].embeddedObject(), "value"));

    /* Release connection */
    mongoDisconnect();

    utExit();
}

/* ****************************************************************************
*
* updateMdAllExisting -
*/
TEST(mongoUpdateContextRequest, updateMdAllExisting)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseMd();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A1", "TA1", "val1");
    Metadata md("MD1", "TMD1", "new_val");
    ca.metadataVector.push_back(&md);
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();    
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("new_val", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_FALSE(a1.hasField("creDate"));
    std::vector<BSONElement> mdV = a1.getField("md").Array();
    ASSERT_EQ(2, mdV.size());
    EXPECT_EQ("MD1", STR_FIELD(mdV[0].embeddedObject(), "name"));
    EXPECT_EQ("TMD1", STR_FIELD(mdV[0].embeddedObject(), "type"));
    EXPECT_EQ("new_val", STR_FIELD(mdV[0].embeddedObject(), "value"));
    EXPECT_EQ("MD2", STR_FIELD(mdV[1].embeddedObject(), "name"));
    EXPECT_EQ("TMD2", STR_FIELD(mdV[1].embeddedObject(), "type"));
    EXPECT_EQ("MD2val", STR_FIELD(mdV[1].embeddedObject(), "value"));

    /* Release connection */
    mongoDisconnect();

    utExit();
}

/* ****************************************************************************
*
* appendMdAllNew -
*/
TEST(mongoUpdateContextRequest, appendMdAllNew)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseMd();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A1", "TA1", "val1");
    Metadata md("MD3", "TMD3", "new_val3");
    ca.metadataVector.push_back(&md);
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD3", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD3", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("new_val3", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_FALSE(a1.hasField("creDate"));
    std::vector<BSONElement> mdV = a1.getField("md").Array();
    ASSERT_EQ(3, mdV.size());
    EXPECT_EQ("MD3", STR_FIELD(mdV[0].embeddedObject(), "name"));
    EXPECT_EQ("TMD3", STR_FIELD(mdV[0].embeddedObject(), "type"));
    EXPECT_EQ("new_val3", STR_FIELD(mdV[0].embeddedObject(), "value"));
    EXPECT_EQ("MD1", STR_FIELD(mdV[1].embeddedObject(), "name"));
    EXPECT_EQ("TMD1", STR_FIELD(mdV[1].embeddedObject(), "type"));
    EXPECT_EQ("MD1val", STR_FIELD(mdV[1].embeddedObject(), "value"));
    EXPECT_EQ("MD2", STR_FIELD(mdV[2].embeddedObject(), "name"));
    EXPECT_EQ("TMD2", STR_FIELD(mdV[2].embeddedObject(), "type"));
    EXPECT_EQ("MD2val", STR_FIELD(mdV[2].embeddedObject(), "value"));
    /* Release connection */
    mongoDisconnect();

    utExit();
}

/* ****************************************************************************
*
*  updateMdAllNew -
*/
TEST(mongoUpdateContextRequest, updateMdAllNew)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseMd();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A1", "TA1", "val1");
    Metadata md("MD3", "TMD3", "new_val3");
    ca.metadataVector.push_back(&md);
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD3", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD3", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("new_val3", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_FALSE(a1.hasField("creDate"));
    std::vector<BSONElement> mdV = a1.getField("md").Array();
    ASSERT_EQ(3, mdV.size());
    EXPECT_EQ("MD3", STR_FIELD(mdV[0].embeddedObject(), "name"));
    EXPECT_EQ("TMD3", STR_FIELD(mdV[0].embeddedObject(), "type"));
    EXPECT_EQ("new_val3", STR_FIELD(mdV[0].embeddedObject(), "value"));
    EXPECT_EQ("MD1", STR_FIELD(mdV[1].embeddedObject(), "name"));
    EXPECT_EQ("TMD1", STR_FIELD(mdV[1].embeddedObject(), "type"));
    EXPECT_EQ("MD1val", STR_FIELD(mdV[1].embeddedObject(), "value"));
    EXPECT_EQ("MD2", STR_FIELD(mdV[2].embeddedObject(), "name"));
    EXPECT_EQ("TMD2", STR_FIELD(mdV[2].embeddedObject(), "type"));
    EXPECT_EQ("MD2val", STR_FIELD(mdV[2].embeddedObject(), "value"));
    /* Release connection */
    mongoDisconnect();

    utExit();
}

/* ****************************************************************************
*
* appendMdSomeNew -
*/
TEST(mongoUpdateContextRequest, appendMdSomeNew)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseMd();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A1", "TA1", "val1");
    Metadata md1("MD2", "TMD2", "new_val2");
    Metadata md2("MD3", "TMD3", "new_val3");
    ca.metadataVector.push_back(&md1);
    ca.metadataVector.push_back(&md2);
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    ASSERT_EQ(2, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD2", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("new_val2", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ("MD3", RES_CER_ATTR(0, 0)->metadataVector.get(1)->name);
    EXPECT_EQ("TMD3", RES_CER_ATTR(0, 0)->metadataVector.get(1)->type);
    EXPECT_EQ("new_val3", RES_CER_ATTR(0, 0)->metadataVector.get(1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_FALSE(a1.hasField("creDate"));
    std::vector<BSONElement> mdV = a1.getField("md").Array();
    ASSERT_EQ(3, mdV.size());
    EXPECT_EQ("MD2", STR_FIELD(mdV[0].embeddedObject(), "name"));
    EXPECT_EQ("TMD2", STR_FIELD(mdV[0].embeddedObject(), "type"));
    EXPECT_EQ("new_val2", STR_FIELD(mdV[0].embeddedObject(), "value"));
    EXPECT_EQ("MD3", STR_FIELD(mdV[1].embeddedObject(), "name"));
    EXPECT_EQ("TMD3", STR_FIELD(mdV[1].embeddedObject(), "type"));
    EXPECT_EQ("new_val3", STR_FIELD(mdV[1].embeddedObject(), "value"));
    EXPECT_EQ("MD1", STR_FIELD(mdV[2].embeddedObject(), "name"));
    EXPECT_EQ("TMD1", STR_FIELD(mdV[2].embeddedObject(), "type"));
    EXPECT_EQ("MD1val", STR_FIELD(mdV[2].embeddedObject(), "value"));
    /* Release connection */
    mongoDisconnect();

    utExit();
}

/* ****************************************************************************
*
* updateMdSomeNew -
*/
TEST(mongoUpdateContextRequest, updateMdSomeNew)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseMd();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A1", "TA1", "val1");
    Metadata md1("MD2", "TMD2", "new_val2");
    Metadata md2("MD3", "TMD3", "new_val3");
    ca.metadataVector.push_back(&md1);
    ca.metadataVector.push_back(&md2);
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();    
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    ASSERT_EQ(2, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD2", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD2", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("new_val2", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ("MD3", RES_CER_ATTR(0, 0)->metadataVector.get(1)->name);
    EXPECT_EQ("TMD3", RES_CER_ATTR(0, 0)->metadataVector.get(1)->type);
    EXPECT_EQ("new_val3", RES_CER_ATTR(0, 0)->metadataVector.get(1)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_FALSE(a1.hasField("creDate"));
    std::vector<BSONElement> mdV = a1.getField("md").Array();
    ASSERT_EQ(3, mdV.size());
    EXPECT_EQ("MD2", STR_FIELD(mdV[0].embeddedObject(), "name"));
    EXPECT_EQ("TMD2", STR_FIELD(mdV[0].embeddedObject(), "type"));
    EXPECT_EQ("new_val2", STR_FIELD(mdV[0].embeddedObject(), "value"));
    EXPECT_EQ("MD3", STR_FIELD(mdV[1].embeddedObject(), "name"));
    EXPECT_EQ("TMD3", STR_FIELD(mdV[1].embeddedObject(), "type"));
    EXPECT_EQ("new_val3", STR_FIELD(mdV[1].embeddedObject(), "value"));
    EXPECT_EQ("MD1", STR_FIELD(mdV[2].embeddedObject(), "name"));
    EXPECT_EQ("TMD1", STR_FIELD(mdV[2].embeddedObject(), "type"));
    EXPECT_EQ("MD1val", STR_FIELD(mdV[2].embeddedObject(), "value"));
    /* Release connection */
    mongoDisconnect();

    utExit();
}

/* ****************************************************************************
*
* appendValueAndMd -
*/
TEST(mongoUpdateContextRequest, appendValueAndMd)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseMd();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A1", "TA1", "attr_new_val");
    Metadata md("MD1", "TMD1", "new_val");
    ca.metadataVector.push_back(&md);
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();    
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("new_val", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("attr_new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_FALSE(a1.hasField("creDate"));
    std::vector<BSONElement> mdV = a1.getField("md").Array();
    ASSERT_EQ(2, mdV.size());
    EXPECT_EQ("MD1", STR_FIELD(mdV[0].embeddedObject(), "name"));
    EXPECT_EQ("TMD1", STR_FIELD(mdV[0].embeddedObject(), "type"));
    EXPECT_EQ("new_val", STR_FIELD(mdV[0].embeddedObject(), "value"));
    EXPECT_EQ("MD2", STR_FIELD(mdV[1].embeddedObject(), "name"));
    EXPECT_EQ("TMD2", STR_FIELD(mdV[1].embeddedObject(), "type"));
    EXPECT_EQ("MD2val", STR_FIELD(mdV[1].embeddedObject(), "value"));

    /* Release connection */
    mongoDisconnect();

    utExit();
}

/* ****************************************************************************
*
* updateValueAndMd -
*/
TEST(mongoUpdateContextRequest, updateValueAndMd)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseMd();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A1", "TA1", "attr_new_val");
    Metadata md("MD1", "TMD1", "new_val");
    ca.metadataVector.push_back(&md);
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();    
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("new_val", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("attr_new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_FALSE(a1.hasField("creDate"));
    std::vector<BSONElement> mdV = a1.getField("md").Array();
    ASSERT_EQ(2, mdV.size());
    EXPECT_EQ("MD1", STR_FIELD(mdV[0].embeddedObject(), "name"));
    EXPECT_EQ("TMD1", STR_FIELD(mdV[0].embeddedObject(), "type"));
    EXPECT_EQ("new_val", STR_FIELD(mdV[0].embeddedObject(), "value"));
    EXPECT_EQ("MD2", STR_FIELD(mdV[1].embeddedObject(), "name"));
    EXPECT_EQ("TMD2", STR_FIELD(mdV[1].embeddedObject(), "type"));
    EXPECT_EQ("MD2val", STR_FIELD(mdV[1].embeddedObject(), "value"));

    /* Release connection */
    mongoDisconnect();

    utExit();
}


/* ****************************************************************************
*
* appendMdNoActualChanges -
*/
TEST(mongoUpdateContextRequest, appendMdNoActualChanges)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseMd();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A1", "TA1", "val1");
    Metadata md("MD1", "TMD1", "MD1val");
    ca.metadataVector.push_back(&md);
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();    
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("MD1val", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("creDate"));
    std::vector<BSONElement> mdV = a1.getField("md").Array();
    ASSERT_EQ(2, mdV.size());
    EXPECT_EQ("MD1", STR_FIELD(mdV[0].embeddedObject(), "name"));
    EXPECT_EQ("TMD1", STR_FIELD(mdV[0].embeddedObject(), "type"));
    EXPECT_EQ("MD1val", STR_FIELD(mdV[0].embeddedObject(), "value"));
    EXPECT_EQ("MD2", STR_FIELD(mdV[1].embeddedObject(), "name"));
    EXPECT_EQ("TMD2", STR_FIELD(mdV[1].embeddedObject(), "type"));
    EXPECT_EQ("MD2val", STR_FIELD(mdV[1].embeddedObject(), "value"));

    /* Release connection */
    mongoDisconnect();

    utExit();
}

/* ****************************************************************************
*
* updateMdNoActualChanges -
*/
TEST(mongoUpdateContextRequest, updateMdNoActualChanges)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseMd();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A1", "TA1", "val1");
    Metadata md("MD1", "TMD1", "MD1val");
    ca.metadataVector.push_back(&md);
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();   
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("MD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->name);
    EXPECT_EQ("TMD1", RES_CER_ATTR(0, 0)->metadataVector.get(0)->type);
    EXPECT_EQ("MD1val", RES_CER_ATTR(0, 0)->metadataVector.get(0)->value);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_FALSE(a1.hasField("creDate"));
    std::vector<BSONElement> mdV = a1.getField("md").Array();
    ASSERT_EQ(2, mdV.size());
    EXPECT_EQ("MD1", STR_FIELD(mdV[0].embeddedObject(), "name"));
    EXPECT_EQ("TMD1", STR_FIELD(mdV[0].embeddedObject(), "type"));
    EXPECT_EQ("MD1val", STR_FIELD(mdV[0].embeddedObject(), "value"));
    EXPECT_EQ("MD2", STR_FIELD(mdV[1].embeddedObject(), "name"));
    EXPECT_EQ("TMD2", STR_FIELD(mdV[1].embeddedObject(), "type"));
    EXPECT_EQ("MD2val", STR_FIELD(mdV[1].embeddedObject(), "value"));

    /* Release connection */
    mongoDisconnect();

    utExit();
}

/* ****************************************************************************
*
* patternUnsupported -
*/
TEST(mongoUpdateContextRequest, patternUnsupported)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce1, ce2;
    ce1.entityId.fill("E1", "T1", "false");
    ContextAttribute ca1("A1", "TA1", "new_val1");
    ce2.entityId.fill("E[2-3]", "T", "true");
    ContextAttribute ca2("AX", "TAX", "X");

    ce1.contextAttributeVector.push_back(&ca1);
    ce2.contextAttributeVector.push_back(&ca2);
    req.contextElementVector.push_back(&ce1);
    req.contextElementVector.push_back(&ce2);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();    
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E[2-3]", RES_CER(1).entityId.id);
    EXPECT_EQ("T", RES_CER(1).entityId.type);
    EXPECT_EQ("true", RES_CER(1).entityId.isPattern);
    EXPECT_EQ(0, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ(SccNotImplemented, RES_CER_STATUS(1).code);
    EXPECT_EQ("Not Implemented", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val1", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();

}

/* ****************************************************************************
*
* notExistFilter -
*/
TEST(mongoUpdateContextRequest, notExistFilter)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "", "false");
    ContextAttribute ca("A1", "TA1", "new_val");

    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("UPDATE");

    /* Set filter */
    uriParams[URI_PARAM_NOT_EXIST] = "entity::type";

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    BSONObj a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a3 = getAttr(attrs, "A3", "TA3");
    BSONObj a4 = getAttr(attrs, "A4", "TA4");
    EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
    EXPECT_FALSE(a3.hasField("modDate"));
    EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
    EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
    EXPECT_FALSE(a4.hasField("value"));
    EXPECT_FALSE(a4.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    BSONObj a5 = getAttr(attrs, "A5", "TA5");
    BSONObj a6 = getAttr(attrs, "A6", "TA6");
    EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
    EXPECT_FALSE(a5.hasField("modDate"));
    EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
    EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
    EXPECT_FALSE(a6.hasField("value"));
    EXPECT_FALSE(a6.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(2, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    a2 = getAttr(attrs, "A2", "TA2");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("new_val", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_FALSE(a2.hasField("value"));
    EXPECT_FALSE(a2.hasField("modDate"));

    /* Release connection */
    mongoDisconnect();

    utExit();
}

/* ****************************************************************************
*
* firstTimeTrue -
*
* This function is used in some mocks that need to emulate more() function in the 
* following way: first call to the function is true, second and further calls are false
*/
bool firstTimeTrue(void)
{
    static bool firstTime = true;
    if (firstTime) {
        firstTime = false;
        return true;
    }
    else {
        return false;
    }
}

/* ****************************************************************************
*
* mongoDbUpdateFail -
*/
TEST(mongoUpdateContextRequest, mongoDbUpdateFail)
{

    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;    

    utInit();

    /* Set database */
    setupDatabase();

    /* Prepare mock */
    /* FIXME: cursorMockCsub is probably unnecesary if we solve the problem of Invoke() the real
     * functionality in the DBClientConnectionMockUpdateContext declaration */
    const DBException e = DBException("boom!!", 33);
    BSONObj fakeEn = BSON("_id" << BSON("id" << "E1" << "type" << "T1") <<
                       "attrs" << BSON_ARRAY(
                          BSON("name" << "A1" << "type" << "TA1" << "value" << "val1") <<
                          BSON("name" << "A2" << "type" << "TA2")
                          )
                     );
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();    
    DBClientCursorMock* cursorMockEnt = new DBClientCursorMock(connectionMock, "", 0, 0, 0);
    DBClientCursorMock* cursorMockCsub = new DBClientCursorMock(connectionMock, "", 0, 0, 0);
    ON_CALL(*cursorMockEnt, more())
            .WillByDefault(Invoke(firstTimeTrue));
    ON_CALL(*cursorMockEnt, next())
            .WillByDefault(Return(fakeEn));
    ON_CALL(*cursorMockCsub, more())
            .WillByDefault(Return(false));
    ON_CALL(*connectionMock, _query("unittest.entities",_,_,_,_,_,_))
            .WillByDefault(Return(cursorMockEnt));
    ON_CALL(*connectionMock, _query("unittest.csubs",_,_,_,_,_,_))
            .WillByDefault(Return(cursorMockCsub));
    ON_CALL(*connectionMock, update(_,_,_,_,_))
            .WillByDefault(Throw(e));

    /* Set MongoDB connection */
    mongoConnect(connectionMock);

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A1", "TA1", "new_val");
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();    
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccReceiverInternalError, RES_CER_STATUS(0).code);
    EXPECT_EQ("Internal Server Error", RES_CER_STATUS(0).reasonPhrase);

    EXPECT_EQ("collection: unittest.entities "
              "- update() query: { _id.id: \"E1\", _id.type: \"T1\", _id.servicePath: { $exists: false } } "
              "- update() doc: { $set: { attrs: [ { name: \"A1\", value: \"new_val\", type: \"TA1\", modDate: 1360232700 }, { name: \"A2\", type: \"TA2\" } ], modDate: 1360232700 }, $unset: { location: 1 } } "
              "- exception: boom!!", RES_CER_STATUS(0).details);

    /* Release mocks */
    //delete cursorMockEnt;
    //delete cursorMockCsub;
    delete connectionMock;

    utExit();

}

/* ****************************************************************************
*
* mongoDbQueryFail -
*/
TEST(mongoUpdateContextRequest, mongoDbQueryFail)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;    

    utInit();

    /* Prepare mock */
    const DBException e = DBException("boom!!", 33);
    DBClientConnectionMock* connectionMock = new DBClientConnectionMock();
    ON_CALL(*connectionMock, _query(_,_,_,_,_,_,_))
            .WillByDefault(Throw(e));

    /* Set MongoDB connection */
    mongoConnect(connectionMock);    

    /* Forge the request (from "inside" to "outside") */
    ContextElement ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A1", "TA1", "new_val");
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("UPDATE");

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();    
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    EXPECT_EQ(0, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ(SccReceiverInternalError, RES_CER_STATUS(0).code);
    EXPECT_EQ("Internal Server Error", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("collection: unittest.entities "
              "- query(): { _id.id: \"E1\", _id.type: \"T1\", _id.servicePath: { $exists: false } } "
              "- exception: boom!!", RES_CER_STATUS(0).details);

    /* Release mock */
    delete connectionMock;

    utExit();
}


/* ****************************************************************************
*
* servicePathEntityUpdates_3levels -
*
*/
TEST(mongoUpdateContextRequest, servicePathEntityUpdate_3levels)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePaths();

  /* Forge the request (from "inside" to "outside") */
  ContextElement         ce;
  ce.entityId.fill("E1", "T1", "false");
  ContextAttribute ca("A1", "TA1", "kz01-modified");
  ce.contextAttributeVector.push_back(&ca);
  req.contextElementVector.push_back(&ce);
  req.updateActionType.set("UPDATE");
  servicePathVector.push_back("/home/kz/01");

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(0, res.errorCode.code);
  EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.errorCode.details.size());

  ASSERT_EQ(1, res.contextElementResponseVector.size());
  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T1", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* entities collection */
  BSONObj ent;
  std::vector<BSONElement> attrs;
  ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1" << "_id.servicePath" << "/home/kz/01"));
  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_EQ(1360232700, ent.getIntField("modDate"));
  attrs = ent.getField("attrs").Array();
  ASSERT_EQ(1, attrs.size());
  BSONObj a1 = getAttr(attrs, "A1", "TA1");
  EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
  EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("kz01-modified", C_STR_FIELD(a1, "value"));
  EXPECT_EQ(1360232700, a1.getIntField("modDate"));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1" << "_id.servicePath" << "/home/kz/02"));
  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").Array();
  ASSERT_EQ(1, attrs.size());
  a1 = getAttr(attrs, "A1", "TA1");
  EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
  EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("kz02", C_STR_FIELD(a1, "value"));
  EXPECT_FALSE(a1.hasField("modDate"));

  utExit();
}

/* ****************************************************************************
*
* servicePathEntityUpdates_2levels -
*
*/
TEST(mongoUpdateContextRequest, servicePathEntityUpdate_2levels)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePaths();

  /* Forge the request (from "inside" to "outside") */
  ContextElement         ce;
  ce.entityId.fill("E1", "T1", "false");
  ContextAttribute ca("A1", "TA1", "kz-modified");
  ce.contextAttributeVector.push_back(&ca);
  req.contextElementVector.push_back(&ce);
  req.updateActionType.set("UPDATE");
  servicePathVector.push_back("/home/kz");

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(0, res.errorCode.code);
  EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.errorCode.details.size());

  ASSERT_EQ(2, res.contextElementResponseVector.size());
  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T1", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

  /* Context Element response # 2 */
  EXPECT_EQ("E1", RES_CER(1).entityId.id);
  EXPECT_EQ("T1", RES_CER(1).entityId.type);
  EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
  EXPECT_EQ("A1", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA1", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(1, 0)->value.size());
  EXPECT_EQ(0, RES_CER_ATTR(1, 0)->metadataVector.size());
  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* entities collection */
  BSONObj ent;
  std::vector<BSONElement> attrs;
  ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1" << "_id.servicePath" << "/home/kz/01"));
  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_EQ(1360232700, ent.getIntField("modDate"));
  attrs = ent.getField("attrs").Array();
  ASSERT_EQ(1, attrs.size());
  BSONObj a1 = getAttr(attrs, "A1", "TA1");
  EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
  EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("kz-modified", C_STR_FIELD(a1, "value"));
  EXPECT_EQ(1360232700, a1.getIntField("modDate"));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1" << "_id.servicePath" << "/home/kz/02"));
  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_EQ(1360232700, ent.getIntField("modDate"));
  attrs = ent.getField("attrs").Array();
  ASSERT_EQ(1, attrs.size());
  a1 = getAttr(attrs, "A1", "TA1");
  EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
  EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("kz-modified", C_STR_FIELD(a1, "value"));
  EXPECT_EQ(1360232700, a1.getIntField("modDate"));

  utExit();
}

/* ****************************************************************************
*
* servicePathEntityAppend_3levels -
*
*/
TEST(mongoUpdateContextRequest, servicePathEntityAppend_3levels)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePaths();

  /* Forge the request (from "inside" to "outside") */
  ContextElement         ce;
  ce.entityId.fill("E1", "T1", "false");
  ContextAttribute ca("A2", "TA2", "new");
  ce.contextAttributeVector.push_back(&ca);
  req.contextElementVector.push_back(&ce);
  req.updateActionType.set("APPEND");
  servicePathVector.push_back("/home/kz/01");

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(0, res.errorCode.code);
  EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.errorCode.details.size());

  ASSERT_EQ(1, res.contextElementResponseVector.size());
  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T1", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A2", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA2", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* entities collection */
  BSONObj ent;
  std::vector<BSONElement> attrs;
  ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1" << "_id.servicePath" << "/home/kz/01"));
  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_EQ(1360232700, ent.getIntField("modDate"));
  attrs = ent.getField("attrs").Array();
  ASSERT_EQ(2, attrs.size());
  BSONObj a1 = getAttr(attrs, "A1", "TA1");
  BSONObj a2 = getAttr(attrs, "A2", "TA2");
  EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
  EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("kz01", C_STR_FIELD(a1, "value"));
  EXPECT_FALSE(a1.hasField("modDate"));
  EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
  EXPECT_STREQ("TA2",C_STR_FIELD(a2, "type"));
  EXPECT_STREQ("new", C_STR_FIELD(a2, "value"));
  EXPECT_EQ(1360232700, a2.getIntField("modDate"));
  EXPECT_EQ(1360232700, a2.getIntField("creDate"));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1" << "_id.servicePath" << "/home/kz/02"));
  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").Array();
  ASSERT_EQ(1, attrs.size());
  a1 = getAttr(attrs, "A1", "TA1");
  EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
  EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("kz02", C_STR_FIELD(a1, "value"));
  EXPECT_FALSE(a1.hasField("modDate"));

  utExit();
}

/* ****************************************************************************
*
* servicePathEntityAppend_2levels -
*
*/
TEST(mongoUpdateContextRequest, servicePathEntityAppend_2levels)
{
  HttpStatusCode         ms;
  UpdateContextRequest   req;
  UpdateContextResponse  res;

  utInit();

  /* Prepare database */
  prepareDatabaseWithServicePaths();

  /* Forge the request (from "inside" to "outside") */
  ContextElement         ce;
  ce.entityId.fill("E1", "T1", "false");
  ContextAttribute ca("A2", "TA2", "new");
  ce.contextAttributeVector.push_back(&ca);
  req.contextElementVector.push_back(&ce);
  req.updateActionType.set("APPEND");
  servicePathVector.push_back("/home/kz");

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);

  EXPECT_EQ(0, res.errorCode.code);
  EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
  EXPECT_EQ(0, res.errorCode.details.size());

  ASSERT_EQ(2, res.contextElementResponseVector.size());
  /* Context Element response # 1 */
  EXPECT_EQ("E1", RES_CER(0).entityId.id);
  EXPECT_EQ("T1", RES_CER(0).entityId.type);
  EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
  EXPECT_EQ("A2", RES_CER_ATTR(0, 0)->name);
  EXPECT_EQ("TA2", RES_CER_ATTR(0, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
  EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
  EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
  EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

  /* Context Element response # 2 */
  EXPECT_EQ("E1", RES_CER(1).entityId.id);
  EXPECT_EQ("T1", RES_CER(1).entityId.type);
  EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
  ASSERT_EQ(1, RES_CER(1).contextAttributeVector.size());
  EXPECT_EQ("A2", RES_CER_ATTR(1, 0)->name);
  EXPECT_EQ("TA2", RES_CER_ATTR(1, 0)->type);
  EXPECT_EQ(0, RES_CER_ATTR(1, 0)->value.size());
  EXPECT_EQ(0, RES_CER_ATTR(1, 0)->metadataVector.size());
  EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
  EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
  EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* entities collection */
  BSONObj ent;
  std::vector<BSONElement> attrs;
  ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1" << "_id.servicePath" << "/home/kz/01"));
  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_EQ(1360232700, ent.getIntField("modDate"));
  attrs = ent.getField("attrs").Array();
  ASSERT_EQ(2, attrs.size());
  BSONObj a1 = getAttr(attrs, "A1", "TA1");
  BSONObj a2 = getAttr(attrs, "A2", "TA2");
  EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
  EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("kz01", C_STR_FIELD(a1, "value"));
  EXPECT_FALSE(a1.hasField("modDate"));
  EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
  EXPECT_STREQ("TA2",C_STR_FIELD(a2, "type"));
  EXPECT_STREQ("new", C_STR_FIELD(a2, "value"));
  EXPECT_EQ(1360232700, a2.getIntField("modDate"));
  EXPECT_EQ(1360232700, a2.getIntField("creDate"));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1" << "_id.servicePath" << "/home/kz/02"));
  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_EQ(1360232700, ent.getIntField("modDate"));
  attrs = ent.getField("attrs").Array();
  ASSERT_EQ(2, attrs.size());
  a1 = getAttr(attrs, "A1", "TA1");
  a2 = getAttr(attrs, "A2", "TA2");
  EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
  EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("kz02", C_STR_FIELD(a1, "value"));
  EXPECT_FALSE(a1.hasField("modDate"));
  EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
  EXPECT_STREQ("TA2",C_STR_FIELD(a2, "type"));
  EXPECT_STREQ("new", C_STR_FIELD(a2, "value"));
  EXPECT_EQ(1360232700, a2.getIntField("modDate"));
  EXPECT_EQ(1360232700, a2.getIntField("creDate"));

  utExit();
}

/* ****************************************************************************
*
* servicePathEntityCreation_2levels -
*
*/
TEST(mongoUpdateContextRequest, servicePathEntityCreation_2levels)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithServicePaths();

    /* Forge the request (from "inside" to "outside") */
    ContextElement         ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A1", "TA1", "fg");
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");
    servicePathVector.push_back("/home/fg");

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(3, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1" << "_id.servicePath" << "/home/kz/01"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("kz01", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1" << "_id.servicePath" << "/home/kz/02"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("kz02", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1" << "_id.servicePath" << "/home/fg"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("fg", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_EQ(1360232700, a1.getIntField("creDate"));

    utExit();

}

/* ****************************************************************************
*
* servicePathEntityCreation_3levels -
*
*/
TEST(mongoUpdateContextRequest, servicePathEntityCreation_3levels)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithServicePaths();

    /* Forge the request (from "inside" to "outside") */
    ContextElement         ce;
    ce.entityId.fill("E1", "T1", "false");
    ContextAttribute ca("A1", "TA1", "fg");
    ce.contextAttributeVector.push_back(&ca);
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("APPEND");
    servicePathVector.push_back("/home/fg/01");

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->value.size());
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(3, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1" << "_id.servicePath" << "/home/kz/01"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("kz01", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1" << "_id.servicePath" << "/home/kz/02"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("kz02", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1" << "_id.servicePath" << "/home/fg/01"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("fg", C_STR_FIELD(a1, "value"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    EXPECT_EQ(1360232700, a1.getIntField("creDate"));

    utExit();

}

/* ****************************************************************************
*
* servicePathEntityDeletion_2levels -
*
*/
TEST(mongoUpdateContextRequest, servicePathEntityDeletion_2levels)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithServicePaths();

    /* Forge the request (from "inside" to "outside") */
    ContextElement         ce;
    ce.entityId.fill("E1", "T1", "false");
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("DELETE");
    servicePathVector.push_back("/home/kz");

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(2, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(0, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Context Element response # 2 */
    EXPECT_EQ("E1", RES_CER(1).entityId.id);
    EXPECT_EQ("T1", RES_CER(1).entityId.type);
    EXPECT_EQ("false", RES_CER(1).entityId.isPattern);
    ASSERT_EQ(0, RES_CER(1).contextAttributeVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(1).code);
    EXPECT_EQ("OK", RES_CER_STATUS(1).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(1).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(0, connection->count(ENTITIES_COLL, BSONObj()));

    utExit();
}


/* ****************************************************************************
*
* servicePathEntityDeletion_3levels -
*
*/
TEST(mongoUpdateContextRequest, servicePathEntityDeletion_3levels)
{
    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    utInit();

    /* Prepare database */
    prepareDatabaseWithServicePaths();

    /* Forge the request (from "inside" to "outside") */
    ContextElement         ce;
    ce.entityId.fill("E1", "T1", "false");
    req.contextElementVector.push_back(&ce);
    req.updateActionType.set("DELETE");
    servicePathVector.push_back("/home/kz/01");

    /* Invoke the function in mongoBackend library */
    ms = mongoUpdateContext(&req, &res, "", servicePathVector, uriParams);

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(0, res.errorCode.code);
    EXPECT_EQ(0, res.errorCode.reasonPhrase.size());
    EXPECT_EQ(0, res.errorCode.details.size());

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(0, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ(0, RES_CER_STATUS(0).details.size());

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent;
    std::vector<BSONElement> attrs;
    ASSERT_EQ(1, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1" << "_id.servicePath" << "/home/kz/02"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").Array();
    ASSERT_EQ(1, attrs.size());
    BSONObj a1 = getAttr(attrs, "A1", "TA1");
    EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
    EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("kz02", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("modDate"));

    utExit();
}

/* ****************************************************************************
*
* servicePathEntityVectorNotAllowed -
*
*/
TEST(mongoUpdateContextRequest, servicePathEntityVectorNotAllowed)
{
  HttpStatusCode         ms;
  UpdateContextRequest   ucReq;
  UpdateContextResponse  ucRes;

  utInit();

  /* Prepare database */
  prepareDatabase();

  /* Forge the request (from "inside" to "outside") */
  ContextElement ce;
  ce.entityId.fill("E1", "T1", "false");
  ContextAttribute  ca("A1", "TA1", "kz01");
  ce.contextAttributeVector.push_back(&ca);
  ucReq.contextElementVector.push_back(&ce);
  ucReq.updateActionType.set("APPEND");
  servicePathVector.clear();
  servicePathVector.push_back("/home/kz");
  servicePathVector.push_back("/home/fg");

  /* Invoke the function in mongoBackend library */
  ms = mongoUpdateContext(&ucReq, &ucRes, "", servicePathVector, uriParams);


  /* Check response is as expected */
  EXPECT_EQ(SccOk, ms);
  ASSERT_EQ(0, ucRes.contextElementResponseVector.size());
  EXPECT_EQ(SccBadRequest, ucRes.errorCode.code);
  EXPECT_EQ("Bad Request", ucRes.errorCode.reasonPhrase);
  EXPECT_EQ("service path length greater than one in update", ucRes.errorCode.details);

  /* Check that every involved collection at MongoDB is as expected */
  /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
   * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

  DBClientBase* connection = getMongoConnection();

  /* entities collection */
  BSONObj ent;
  std::vector<BSONElement> attrs;
  ASSERT_EQ(5, connection->count(ENTITIES_COLL, BSONObj()));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").Array();
  ASSERT_EQ(2, attrs.size());
  BSONObj a1 = getAttr(attrs, "A1", "TA1");
  BSONObj a2 = getAttr(attrs, "A2", "TA2");
  EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
  EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("val1", C_STR_FIELD(a1, "value"));
  EXPECT_FALSE(a1.hasField("modDate"));
  EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
  EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
  EXPECT_FALSE(a2.hasField("value"));
  EXPECT_FALSE(a2.hasField("modDate"));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
  EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").Array();
  ASSERT_EQ(2, attrs.size());
  BSONObj a3 = getAttr(attrs, "A3", "TA3");
  BSONObj a4 = getAttr(attrs, "A4", "TA4");
  EXPECT_STREQ("A3", C_STR_FIELD(a3, "name"));
  EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
  EXPECT_STREQ("val3", C_STR_FIELD(a3, "value"));
  EXPECT_FALSE(a3.hasField("modDate"));
  EXPECT_STREQ("A4", C_STR_FIELD(a4, "name"));
  EXPECT_STREQ("TA4", C_STR_FIELD(a4, "type"));
  EXPECT_FALSE(a4.hasField("value"));
  EXPECT_FALSE(a4.hasField("modDate"));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
  EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").Array();
  ASSERT_EQ(2, attrs.size());
  BSONObj a5 = getAttr(attrs, "A5", "TA5");
  BSONObj a6 = getAttr(attrs, "A6", "TA6");
  EXPECT_STREQ("A5", C_STR_FIELD(a5, "name"));
  EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
  EXPECT_STREQ("val5", C_STR_FIELD(a5, "value"));
  EXPECT_FALSE(a5.hasField("modDate"));
  EXPECT_STREQ("A6", C_STR_FIELD(a6, "name"));
  EXPECT_STREQ("TA6", C_STR_FIELD(a6, "type"));
  EXPECT_FALSE(a6.hasField("value"));
  EXPECT_FALSE(a6.hasField("modDate"));

  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1bis"));
  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_STREQ("T1bis", C_STR_FIELD(ent.getObjectField("_id"), "type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").Array();
  ASSERT_EQ(1, attrs.size());
  a1 = getAttr(attrs, "A1", "TA1");
  EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
  EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("val1bis2", C_STR_FIELD(a1, "value"));
  EXPECT_FALSE(a1.hasField("modDate"));

  /* Note "_id.type: {$exists: false}" is a way for querying for entities without type */
  ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << BSON("$exists" << false)));
  EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
  EXPECT_FALSE(ent.getObjectField("_id").hasField("type"));
  EXPECT_FALSE(ent.hasField("modDate"));
  attrs = ent.getField("attrs").Array();
  ASSERT_EQ(2, attrs.size());
  a1 = getAttr(attrs, "A1", "TA1");
  a2 = getAttr(attrs, "A2", "TA2");
  EXPECT_STREQ("A1", C_STR_FIELD(a1, "name"));
  EXPECT_STREQ("TA1",C_STR_FIELD(a1, "type"));
  EXPECT_STREQ("val1-nt", C_STR_FIELD(a1, "value"));
  EXPECT_FALSE(a1.hasField("modDate"));
  EXPECT_STREQ("A2", C_STR_FIELD(a2, "name"));
  EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
  EXPECT_FALSE(a2.hasField("value"));
  EXPECT_FALSE(a2.hasField("modDate"));

  /* Release connection */
  mongoDisconnect();

  utExit();
}
