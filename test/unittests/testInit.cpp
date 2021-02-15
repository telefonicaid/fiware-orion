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
* iot_support at tid dot es
*
* Author: Fermin Galan
*/
#include "unittests/testInit.h"

#include "logMsg/logMsg.h"

#include "mongoBackend/MongoGlobal.h"
// #include "mongoBackend/mongoConnectionPool.h"



#if 0
// FIXME OLD-DR: disabled until driver migration can be done
/* ****************************************************************************
*
* External declarations
*/
extern DBClientBase mongoInitialConnectionGetForUnitTest();
extern void         setMongoConnectionForUnitTest(DBClientBase* _connection);



/* ****************************************************************************
*
* setupDatabase -
*
* This function (which is called before every test) cleans the database
*/
void setupDatabase(void)
{
  DBClientBase*  connection   = NULL;
  static bool    mongoStarted = false;

  /* mongoStart is needed one time to create the connection pool */
  if (mongoStarted == false)
  {
    /* In fact, the mongoConnectionPoolInit() parameters related with the pool, e.g. pool size, are irrelevant,
     * given that the connection creation is mocked under UNIT_TEST in the mongoBackend library
     */
    mongoConnectionPoolInit("localhost", "", "", "", "", "", "", false, false, 0, 10);
    mongoStarted = true;
  }

  connection = getMongoConnection();

  connection->dropCollection(REGISTRATIONS_COLL);
  connection->dropCollection(ENTITIES_COLL);
  connection->dropCollection(SUBSCRIBECONTEXT_COLL);

  setDbPrefix(DBPREFIX);
  setRegistrationsCollectionName("registrations");
  setEntitiesCollectionName("entities");
  setSubscribeContextCollectionName("csubs");
}
#endif



/* ****************************************************************************
*
* equalEntity -
*/
static bool equalEntity(EntityId enExpected, EntityId enArg)
{
  LM_M(("enArg '%s', '%s', '%s'",
        enArg.id.c_str(),
        enArg.type.c_str(),
        enArg.isPattern.c_str()));

  LM_M(("enExpected '%s', '%s', '%s'",
        enExpected.id.c_str(),
        enExpected.type.c_str(),
        enExpected.isPattern.c_str()));

  if (enExpected.id == enArg.id && enExpected.type == enArg.type && enExpected.isPattern == enArg.isPattern)
  {
    return true;
  }
  else
  {
    return false;
  }
}



/* ****************************************************************************
*
* equalContextAttribute -
*/
static bool equalContextAttribute(ContextAttribute* caExpected, ContextAttribute* caArg)
{
  if (caArg->name != caExpected->name)
  {
    return false;
  }

  if (caArg->type != caExpected->type)
  {
    return false;
  }

  if (caArg->valueType != caExpected->valueType)
  {
    return false;
  }

  if ((caArg->valueType == orion::ValueTypeString) && (caArg->stringValue != caExpected->stringValue))
  {
    return false;
  }

  if ((caArg->valueType == orion::ValueTypeNumber) && (caArg->numberValue != caExpected->numberValue))
  {
    return false;
  }

  if ((caArg->valueType == orion::ValueTypeBoolean) && (caArg->boolValue != caExpected->boolValue))
  {
    return false;
  }

  LM_M(("caArg '%s', '%s', '%s'", caArg->name.c_str(), caArg->type.c_str(), caArg->getValue().c_str()));
  LM_M(("caExpected '%s', '%s', '%s'",
        caExpected->name.c_str(),
        caExpected->type.c_str(),
        caExpected->getValue().c_str()));

  return true;
}



/* ****************************************************************************
*
* equalContextAttributeVector -
*/
static bool equalContextAttributeVector(ContextAttributeVector caExpectedV, ContextAttributeVector caArgV)
{
  /* Check vector size */
  if (caExpectedV.size() != caArgV.size())
  {
    LM_M(("different sizes: expected %d, actual %d", caExpectedV.size(), caArgV.size()));
    return false;
  }

  /* Check that every attribute in 'caExpectedV' is in 'caArgV'. Order doesn't matter */
  for (unsigned int ix = 0; ix < caArgV.size(); ++ix)
  {
    bool attributeMatch = false;

    for (unsigned int jx = 0; jx < caExpectedV.size(); ++jx)
    {
      ContextAttribute* caArg = caArgV[ix];
      ContextAttribute* caExpected = caExpectedV[jx];

      LM_M(("%d == %d?", ix, jx));
      if (equalContextAttribute(caExpected, caArg))
      {
        LM_M(("attribute matches in ContextAttributeVector comparison, check next one..."));
        attributeMatch = true;
        break; /* loop in jx */
      }
    }

    if (!attributeMatch)
    {
      LM_M(("after looking everyone, attribute doesn't match in ContextAttributeVector comparison"));
      return false;
    }
  }

  LM_M(("ContextAttributeVector comparison ok"));
  return true;
}



/* ****************************************************************************
*
* equalContextElementResponseVector -
*/
static bool equalContextElementResponseVector
(
  ContextElementResponseVector cerExpectedV,
  ContextElementResponseVector cerArgV
)
{
  /* Check vector size */
  if (cerExpectedV.size() != cerArgV.size())
  {
    LM_M(("different sizes: expected %d, actual %d", cerExpectedV.size(), cerArgV.size()));
    return false;
  }

  /* Check that every entity in 'cerArgV' is in 'cerExpectedV'. Order doesn't matter */
  for (unsigned int ix = 0; ix < cerArgV.size(); ++ix)
  {
    bool entityMatch = false;

    for (unsigned int jx = 0; jx < cerExpectedV.size(); ++jx)
    {
      ContextElementResponse* cerArg      = cerArgV[ix];
      ContextElementResponse* cerExpected = cerExpectedV[jx];

      EntityId enExpected(cerExpected->entity.id, cerExpected->entity.type);
      EntityId enArg(cerArg->entity.id, cerArg->entity.type);

      if (!equalEntity(enExpected, enArg))
      {
        LM_M(("entity doesn't match in ContextElementResponseVector comparison, continue ..."));
        continue; /* loop in jx */
      }

      /* If there aren't attributes to check, then early exits the loop */
      if (cerExpected->entity.attributeVector.size() == 0)
      {
        LM_M(("entity (without attributes) matches in ContextElementResponseVector comparison, check next one..."));
        entityMatch = true;
        break; /* loop in jx */
      }

      if (equalContextAttributeVector(cerExpected->entity.attributeVector, cerArg->entity.attributeVector))
      {
        LM_M(("entity (with attributes) matches in ContextElementResponseVector comparison, check next one..."));
        entityMatch = true;
        break; /* loop in jx */
      }
    }

    if (!entityMatch)
    {
      LM_M(("after checking everything, entity doesn't match in ContextElementResponseVector comparison"));
      return false;
    }
  }

  LM_M(("ContextElementResponseVector comparison ok"));
  return true;
}



/* ****************************************************************************
*
* matchNotifyContextRequest -
*
* We need a matcher to compare NotifyContextRequest in EXPECT_CALL() for sendNotifyContextRequest, due
* to NotifyContextRequest is not yet a full fledged object and we can not use the '==' method and
* Eq() matcher. FIXME
*/
bool matchNotifyContextRequest(NotifyContextRequest* expected, NotifyContextRequest* arg)
{
  LM_M(("inside MatchNcr"));

  /* The isEmpty() check is needed because, in some occasion, we don't have any expectations on
   * subscriptionId and, in these cases, we don't check its value */
  if (!expected->subscriptionId.isEmpty() && (expected->subscriptionId.get() != arg->subscriptionId.get()))
  {
    return false;
  }

  if (expected->originator.get() != arg->originator.get())
  {
    return false;
  }

  if (!equalContextElementResponseVector(expected->contextElementResponseVector, arg->contextElementResponseVector))
  {
    return false;
  }

  return true;
}
