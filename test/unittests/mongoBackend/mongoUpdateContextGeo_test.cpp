/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
* iot_support at tid dot es
*
* Author: Fermin Galan
*/
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "orionld/common/tenantList.h"     // tenant0

#include "common/globals.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoUpdateContext.h"
#include "ngsi/EntityId.h"
#include "ngsi10/UpdateContextRequest.h"
#include "ngsi10/UpdateContextResponse.h"

#include "unittests/unittest.h"



/* ****************************************************************************
*
* USING
*/
using mongo::DBClientBase;
using mongo::BSONObj;
using mongo::BSONArray;
using mongo::BSONElement;
using mongo::OID;
using mongo::DBException;
using mongo::BSONObjBuilder;
using mongo::BSONNULL;



extern void setMongoConnectionForUnitTest(DBClientBase* _connection);



/* ****************************************************************************
*
* Tests
*
* - newEntityLocAttribute
* - appendLocAttribute
* - updateLocAttribute
* - deleteLocAttribute
* - newEntityTwoLocAttributesFail
* - newEntityWrongCoordinatesFormatFail
* - newEntityNotSupportedLocationFail
* - appendAdditionalLocAttributeFail
* - appendWrongCoordinatesFormatFail
* - appendNotSupportedLocationFail
* - updateWrongCoordinatesFormatFail
* - updateLocationMetadataFail
* - deleteLocationMetadataFail
*
*/

/* ****************************************************************************
*
* prepareDatabase -
*
* This function is called before every test, to populate some information in the
* entities collection.
*/
static void prepareDatabase(void)
{
  /* Set database */
  setupDatabase();

  DBClientBase* connection = getMongoConnection();


  BSONObj en1 = BSON("_id" << BSON("id" << "E1" << "type" << "T1") <<
                     "attrNames" << BSON_ARRAY("A1" << "A2") <<
                     "attrs" << BSON(
                        "A1" << BSON("type" << "TA1" << "value" << "-5, 2") <<
                        "A2" << BSON("type" << "TA2" << "value" << "noloc")) <<
                     "location" << BSON("attrName" << "A1" <<
                                        "coords" << BSON("type" << "Point" <<
                                                         "coordinates" << BSON_ARRAY(2.0 << -5.0))));

  BSONObj en2 = BSON("_id" << BSON("id" << "E2" << "type" << "T2") <<
                     "attrNames" << BSON_ARRAY("A2") <<
                     "attrs" << BSON(
                       "A2" << BSON("type" << "TA2" << "value" << "Y")));

  connection->insert(ENTITIES_COLL, en1);
  connection->insert(ENTITIES_COLL, en2);
}


#define coordX(e) e.getObjectField("location").getObjectField("coords").getField("coordinates").Array()[0].Double()
#define coordY(e) e.getObjectField("location").getObjectField("coords").getField("coordinates").Array()[1].Double()

/* ****************************************************************************
*
* findAttr -
*
* FIXME P4: this functions is repeated in several places (e.g. mongoUpdateContext_test.cpp). Factorice in a common place.
*
*/
static bool findAttr(std::vector<BSONElement> attrs, std::string name)
{
  for (unsigned int ix = 0; ix < attrs.size(); ++ix)
  {
    if (attrs[ix].str() == name)
    {
      return true;
    }
  }
  return false;
}



/* ****************************************************************************
*
* newEntityLocAttribute -
*/
TEST(mongoUpdateContextGeoRequest, newEntityLocAttribute)
{
    utInit();

    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement*   ceP = new ContextElement();
    ContextAttribute* caP = new ContextAttribute("A3", "TA3", "4, -5");
    Metadata*         mdP = new Metadata("location", "string", "WGS84");

    ceP->entityId.fill("E3", "T3", "false");
    caP->metadataVector.push_back(mdP);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, &tenant0, servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E3", RES_CER(0).entityId.id);
    EXPECT_EQ("T3", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A3", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA3", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(3, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("-5, 2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("creDate"));
    EXPECT_FALSE(a1.hasField("modDate"));
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("noloc", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A1", ent.getObjectField("location").getStringField("attrName"));
    ASSERT_TRUE(ent.hasField("location"));
    EXPECT_EQ(2, coordX(ent));
    EXPECT_EQ(-5, coordY(ent));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("Y", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("location"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E3" << "_id.type" << "T3"));
    EXPECT_STREQ("E3", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T3", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("creDate"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a3 = attrs.getField("A3").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A3"));
    EXPECT_STREQ("TA3", C_STR_FIELD(a3, "type"));
    EXPECT_STREQ("4, -5", C_STR_FIELD(a3, "value"));
    EXPECT_EQ(1360232700, a3.getIntField("creDate"));
    EXPECT_EQ(1360232700, a3.getIntField("modDate"));
    EXPECT_STREQ("A3", ent.getObjectField("location").getStringField("attrName"));
    ASSERT_TRUE(ent.hasField("location"));
    EXPECT_EQ(-5, coordX(ent));
    EXPECT_EQ(4, coordY(ent));

    utExit();
}

/* ****************************************************************************
*
*  - appendLocAttribute
*/
TEST(mongoUpdateContextGeoRequest, appendLocAttribute)
{
    utInit();

    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E2", "T2", "false");
    ContextAttribute* caP = new ContextAttribute("A5", "TA5", "8, -9");
    Metadata* mdP = new Metadata("location", "string", "WGS84");
    caP->metadataVector.push_back(mdP);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, &tenant0, servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E2", RES_CER(0).entityId.id);
    EXPECT_EQ("T2", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A5", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA5", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("creDate"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("-5, 2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("creDate"));
    EXPECT_FALSE(a1.hasField("modDate"));
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("noloc", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A1", ent.getObjectField("location").getStringField("attrName"));
    ASSERT_TRUE(ent.hasField("location"));
    EXPECT_EQ(2, coordX(ent));
    EXPECT_EQ(-5, coordY(ent));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    a2 = attrs.getField("A2").embeddedObject();
    BSONObj a5 = attrs.getField("A5").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_TRUE(findAttr(attrNames, "A5"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("Y", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("TA5", C_STR_FIELD(a5, "type"));
    EXPECT_STREQ("8, -9", C_STR_FIELD(a5, "value"));
    EXPECT_EQ(1360232700, a5.getIntField("creDate"));
    EXPECT_EQ(1360232700, a5.getIntField("modDate"));
    EXPECT_STREQ("A5", ent.getObjectField("location").getStringField("attrName"));
    ASSERT_TRUE(ent.hasField("location"));
    EXPECT_EQ(-9, coordX(ent));
    EXPECT_EQ(8, coordY(ent));

    utExit();
}

/* ****************************************************************************
*
*  - updateLocAttribute
*/
TEST(mongoUpdateContextGeoRequest, updateLocAttribute)
{
    utInit();

    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "2, -4");
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeUpdate;

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, &tenant0, servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("creDate"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("2, -4", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("creDate"));
    EXPECT_EQ(1360232700, a1.getIntField("modDate"));
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("noloc", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A1", ent.getObjectField("location").getStringField("attrName"));
    ASSERT_TRUE(ent.hasField("location"));
    EXPECT_EQ(-4, coordX(ent));
    EXPECT_EQ(2, coordY(ent));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("Y", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("location"));

    utExit();
}

/* ****************************************************************************
*
*  - deleteLocAttribute
*/
TEST(mongoUpdateContextGeoRequest, deleteLocAttribute)
{
    utInit();

    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "");
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeDelete;

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, &tenant0, servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccOk, RES_CER_STATUS(0).code);
    EXPECT_EQ("OK", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("creDate"));
    EXPECT_EQ(1360232700, ent.getIntField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("noloc", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("location"));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("Y", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("location"));

    utExit();
}

/* ****************************************************************************
*
*  - newEntityTwoLocAttributesFail
*/
TEST(mongoUpdateContextGeoRequest, newEntityTwoLocAttributesFail)
{
    utInit();

    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ContextAttribute* ca1P = new ContextAttribute("A1", "TA1", "2, -4");
    ContextAttribute* ca2P = new ContextAttribute("A2", "TA2", "5, -6");
    Metadata*         md1P = new Metadata("location", "string", "WGS84");
    Metadata*         md2P = new Metadata("location", "string", "WGS84");

    ceP->entityId.fill("E3", "T3", "false");

    ca1P->metadataVector.push_back(md1P);
    ceP->contextAttributeVector.push_back(ca1P);

    ca2P->metadataVector.push_back(md2P);
    ceP->contextAttributeVector.push_back(ca2P);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, &tenant0, servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E3", RES_CER(0).entityId.id);
    EXPECT_EQ("T3", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(2, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ("A2", RES_CER_ATTR(0, 1)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 1)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 1)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 1)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(0, 1)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 1)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(0, 1)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(0).code);
    EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("You cannot use more than one geo location attribute when creating an entity "
              "[see Orion user manual]",
              RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("creDate"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("-5, 2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("creDate"));
    EXPECT_FALSE(a1.hasField("modDate"));
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("noloc", C_STR_FIELD(a2, "value"));
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A1", ent.getObjectField("location").getStringField("attrName"));
    ASSERT_TRUE(ent.hasField("location"));
    EXPECT_EQ(2, coordX(ent));
    EXPECT_EQ(-5, coordY(ent));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("Y", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("location"));

    utExit();
}

/* ****************************************************************************
*
*  - newEntityWrongCoordinatesFormatFail
*/
TEST(mongoUpdateContextGeoRequest, newEntityWrongCoordinatesFormatFail)
{
    utInit();

    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E3", "T3", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "invalid");
    Metadata* mdP = new Metadata("location", "string", "WGS84");
    caP->metadataVector.push_back(mdP);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, &tenant0, servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E3", RES_CER(0).entityId.id);
    EXPECT_EQ("T3", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(0).code);
    EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("geo coordinates format error [see Orion user manual]: invalid", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("creDate"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("-5, 2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("creDate"));
    EXPECT_FALSE(a1.hasField("modDate"));
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("noloc", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A1", ent.getObjectField("location").getStringField("attrName"));
    ASSERT_TRUE(ent.hasField("location"));
    EXPECT_EQ(2, coordX(ent));
    EXPECT_EQ(-5, coordY(ent));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("Y", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("location"));

    utExit();
}

/* ****************************************************************************
*
*  - newEntityNotSupportedLocationFail
*/
TEST(mongoUpdateContextGeoRequest, newEntityNotSupportedLocationFail)
{
    utInit();

    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E3", "T3", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "2, 4");
    Metadata* mdP = new Metadata("location", "string", "gurugu");
    caP->metadataVector.push_back(mdP);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, &tenant0, servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E3", RES_CER(0).entityId.id);
    EXPECT_EQ("T3", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("gurugu", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(0).code);
    EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("only WGS84 are supported, found: gurugu", RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("creDate"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("-5, 2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("creDate"));
    EXPECT_FALSE(a1.hasField("modDate"));
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("noloc", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A1", ent.getObjectField("location").getStringField("attrName"));
    ASSERT_TRUE(ent.hasField("location"));
    EXPECT_EQ(2, coordX(ent));
    EXPECT_EQ(-5, coordY(ent));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("Y", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("location"));

    utExit();
}


/* ****************************************************************************
*
*  - appendAdditionalLocAttributeFail
*/
TEST(mongoUpdateContextGeoRequest, appendAdditionalLocAttributeFail)
{
    utInit();

    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A5", "TA5", "2, 4");
    Metadata* mdP = new Metadata("location", "string", "WGS84");
    caP->metadataVector.push_back(mdP);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, &tenant0, servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A5", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA5", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(0).code);
    EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("action: APPEND - entity: [E1, T1] - offending attribute: A5 - "
              "attempt to define a geo location attribute [A5] "
              "when another one has been previously defined [A1]",
              RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("creDate"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("-5, 2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("creDate"));
    EXPECT_FALSE(a1.hasField("modDate"));
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("noloc", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A1", ent.getObjectField("location").getStringField("attrName"));
    ASSERT_TRUE(ent.hasField("location"));
    EXPECT_EQ(2, coordX(ent));
    EXPECT_EQ(-5, coordY(ent));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("Y", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("location"));

    utExit();
}

/* ****************************************************************************
*
*  - appendWrongCoordinatesFormatFail
*/
TEST(mongoUpdateContextGeoRequest, appendWrongCoordinatesFormatFail)
{
    utInit();

    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E2", "T2", "false");
    ContextAttribute* caP = new ContextAttribute("A5", "TA5", "erroneous");
    Metadata* mdP = new Metadata("location", "string", "WGS84");
    caP->metadataVector.push_back(mdP);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, &tenant0, servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E2", RES_CER(0).entityId.id);
    EXPECT_EQ("T2", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A5", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA5", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(0).code);
    EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("action: APPEND - entity: [E2, T2] - offending attribute: A5 "
              "- error parsing location attribute for new attribute: "
              "geo coordinates format error [see Orion user manual]: erroneous",
              RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("creDate"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("-5, 2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("creDate"));
    EXPECT_FALSE(a1.hasField("modDate"));
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("noloc", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A1", ent.getObjectField("location").getStringField("attrName"));
    ASSERT_TRUE(ent.hasField("location"));
    EXPECT_EQ(2, coordX(ent));
    EXPECT_EQ(-5, coordY(ent));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("Y", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("location"));

    utExit();
}

/* ****************************************************************************
*
*  - appendNotSupportedLocationFail
*/
TEST(mongoUpdateContextGeoRequest, appendNotSupportedLocationFail)
{
    utInit();

    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E2", "T2", "false");
    ContextAttribute* caP = new ContextAttribute("A5", "TA5", "8, -9");
    Metadata* mdP = new Metadata("location", "string", "gurugu");
    caP->metadataVector.push_back(mdP);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeAppend;

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, &tenant0, servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E2", RES_CER(0).entityId.id);
    EXPECT_EQ("T2", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A5", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA5", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("gurugu", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(0).code);
    EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("action: APPEND - entity: [E2, T2] - offending attribute: A5 - "
              "only WGS84 is supported for location, found: [gurugu]",
              RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("creDate"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("-5, 2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("creDate"));
    EXPECT_FALSE(a1.hasField("modDate"));
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("noloc", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A1", ent.getObjectField("location").getStringField("attrName"));
    ASSERT_TRUE(ent.hasField("location"));
    EXPECT_EQ(2, coordX(ent));
    EXPECT_EQ(-5, coordY(ent));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("Y", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("location"));

    utExit();
}

/* ****************************************************************************
*
*  - updateWrongCoordinatesFormatFail
*/
TEST(mongoUpdateContextGeoRequest, updateWrongCoordinatesFormatFail)
{
    utInit();

    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "invalid");
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeUpdate;

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, &tenant0, servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(0, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(0).code);
    EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("action: UPDATE - entity: [E1, T1] - offending attribute: A1 - "
              "error parsing location attribute: "
              "geo coordinates format error [see Orion user manual]: invalid",
              RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("creDate"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("-5, 2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("creDate"));
    EXPECT_FALSE(a1.hasField("modDate"));
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("noloc", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A1", ent.getObjectField("location").getStringField("attrName"));
    ASSERT_TRUE(ent.hasField("location"));
    EXPECT_EQ(2, coordX(ent));
    EXPECT_EQ(-5, coordY(ent));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("Y", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("location"));

    utExit();
}

/* ****************************************************************************
*
*  - updateLocationMetadataFail
*/
TEST(mongoUpdateContextGeoRequest, updateLocationMetadataFail)
{
    utInit();

    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A2", "TA2", "2, -4");
    Metadata* mdP = new Metadata("location", "string", "WGS84");
    caP->metadataVector.push_back(mdP);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeUpdate;

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, &tenant0, servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A2", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA2", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(0).code);
    EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("action: UPDATE - entity: [E1, T1] - offending attribute: A2 - "
              "attempt to define a geo location attribute [A2] when another one has been previously defined [A1]",
              RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("creDate"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("-5, 2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("creDate"));
    EXPECT_FALSE(a1.hasField("modDate"));
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("noloc", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A1", ent.getObjectField("location").getStringField("attrName"));
    ASSERT_TRUE(ent.hasField("location"));
    EXPECT_EQ(2, coordX(ent));
    EXPECT_EQ(-5, coordY(ent));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("Y", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("location"));

    utExit();
}

/* ****************************************************************************
*
*  - deleteLocationMetadataFail
*/
TEST(mongoUpdateContextGeoRequest, deleteLocationMetadataFail)
{
    utInit();

    HttpStatusCode         ms;
    UpdateContextRequest   req;
    UpdateContextResponse  res;

    /* Prepare database */
    prepareDatabase();

    /* Forge the request (from "inside" to "outside") */
    ContextElement* ceP = new ContextElement();
    ceP->entityId.fill("E1", "T1", "false");
    ContextAttribute* caP = new ContextAttribute("A1", "TA1", "");
    Metadata* mdP = new Metadata("location", "string", "WGS84");
    caP->metadataVector.push_back(mdP);
    ceP->contextAttributeVector.push_back(caP);
    req.contextElementVector.push_back(ceP);
    req.updateActionType = ActionTypeDelete;

    /* Invoke the function in mongoBackend library */
    servicePathVector.clear();
    ms = mongoUpdateContext(&req, &res, &tenant0, servicePathVector, uriParams, "", "", "");

    /* Check response is as expected */
    EXPECT_EQ(SccOk, ms);

    EXPECT_EQ(SccOk, res.errorCode.code);
    EXPECT_EQ("OK", res.errorCode.reasonPhrase);
    EXPECT_EQ("", res.errorCode.details);

    ASSERT_EQ(1, res.contextElementResponseVector.size());
    /* Context Element response # 1 */
    EXPECT_EQ("E1", RES_CER(0).entityId.id);
    EXPECT_EQ("T1", RES_CER(0).entityId.type);
    EXPECT_EQ("false", RES_CER(0).entityId.isPattern);
    ASSERT_EQ(1, RES_CER(0).contextAttributeVector.size());
    EXPECT_EQ("A1", RES_CER_ATTR(0, 0)->name);
    EXPECT_EQ("TA1", RES_CER_ATTR(0, 0)->type);
    EXPECT_EQ(0, RES_CER_ATTR(0, 0)->stringValue.size());
    ASSERT_EQ(1, RES_CER_ATTR(0, 0)->metadataVector.size());
    EXPECT_EQ("location", RES_CER_ATTR(0, 0)->metadataVector[0]->name);
    EXPECT_EQ("string", RES_CER_ATTR(0, 0)->metadataVector[0]->type);
    EXPECT_EQ("WGS84", RES_CER_ATTR(0, 0)->metadataVector[0]->stringValue);
    EXPECT_EQ(SccInvalidParameter, RES_CER_STATUS(0).code);
    EXPECT_EQ("request parameter is invalid/not allowed", RES_CER_STATUS(0).reasonPhrase);
    EXPECT_EQ("action: DELETE - entity: [E1, T1] - offending attribute: A1 - "
              "location attribute has to be defined at creation time, with APPEND",
              RES_CER_STATUS(0).details);

    /* Check that every involved collection at MongoDB is as expected */
    /* Note we are using EXPECT_STREQ() for some cases, as Mongo Driver returns const char*, not string
     * objects (see http://code.google.com/p/googletest/wiki/Primer#String_Comparison) */

    DBClientBase* connection = getMongoConnection();

    /* entities collection */
    BSONObj ent, attrs;
    std::vector<BSONElement> attrNames;
    ASSERT_EQ(2, connection->count(ENTITIES_COLL, BSONObj()));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E1" << "_id.type" << "T1"));
    EXPECT_STREQ("E1", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T1", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("creDate"));
    EXPECT_FALSE(ent.hasField("modDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(2, attrs.nFields());
    ASSERT_EQ(2, attrNames.size());
    BSONObj a1 = attrs.getField("A1").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A1"));
    EXPECT_STREQ("TA1", C_STR_FIELD(a1, "type"));
    EXPECT_STREQ("-5, 2", C_STR_FIELD(a1, "value"));
    EXPECT_FALSE(a1.hasField("creDate"));
    EXPECT_FALSE(a1.hasField("modDate"));
    BSONObj a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("noloc", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_STREQ("A1", ent.getObjectField("location").getStringField("attrName"));
    ASSERT_TRUE(ent.hasField("location"));
    EXPECT_EQ(2, coordX(ent));
    EXPECT_EQ(-5, coordY(ent));

    ent = connection->findOne(ENTITIES_COLL, BSON("_id.id" << "E2" << "_id.type" << "T2"));
    EXPECT_STREQ("E2", C_STR_FIELD(ent.getObjectField("_id"), "id"));
    EXPECT_STREQ("T2", C_STR_FIELD(ent.getObjectField("_id"), "type"));
    EXPECT_FALSE(ent.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("creDate"));
    attrs = ent.getField("attrs").embeddedObject();
    attrNames = ent.getField("attrNames").Array();
    ASSERT_EQ(1, attrs.nFields());
    ASSERT_EQ(1, attrNames.size());
    a2 = attrs.getField("A2").embeddedObject();
    EXPECT_TRUE(findAttr(attrNames, "A2"));
    EXPECT_STREQ("TA2", C_STR_FIELD(a2, "type"));
    EXPECT_STREQ("Y", C_STR_FIELD(a2, "value"));
    EXPECT_FALSE(a2.hasField("creDate"));
    EXPECT_FALSE(a2.hasField("modDate"));
    EXPECT_FALSE(ent.hasField("location"));

    utExit();
}
