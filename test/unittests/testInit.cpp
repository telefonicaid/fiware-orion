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
#include <string>

#include "mongo/client/dbclient.h"
#include "unittests/testInit.h"

#include "logMsg/logMsg.h"

#include "mongoBackend/MongoGlobal.h"



/* ****************************************************************************
*
* USING
*/
using mongo::DBClientBase;



/* ****************************************************************************
*
* External declarations
*/
extern DBClientBase* mongoInitialConnectionGetForUnitTest();
extern void          setMongoConnectionForUnitTest(DBClientBase* _connection);



/* ****************************************************************************
*
* C_STR_FIELD -
*/
const char* C_STR_FIELD(mongo::BSONObj bob, std::string f)
{
  return getStringField(&bob, f.c_str());
}


char dbPrefix[16];
char registrationsCollection[128];
char entitiesCollection[128];
char subscriptionsCollection[128];
char avSubscriptionsCollection[128];

void setDbPrefix(const char* prefix)
{
  strncpy(dbPrefix, prefix, sizeof(dbPrefix) - 1);
}

void setRegistrationsCollectionName(const char* colName)
{
  snprintf(registrationsCollection, sizeof(registrationsCollection), "%s.%s", dbPrefix, colName);
}

void setEntitiesCollectionName(const char* colName)
{
  snprintf(entitiesCollection, sizeof(entitiesCollection), "%s.%s", dbPrefix, colName);
}

void setSubscribeContextCollectionName(const char* colName)
{
  snprintf(subscriptionsCollection, sizeof(subscriptionsCollection), "%s.%s", dbPrefix, colName);
}

void setSubscribeContextAvailabilityCollectionName(const char* colName)
{
  snprintf(avSubscriptionsCollection, sizeof(avSubscriptionsCollection), "%s.%s", dbPrefix, colName);
}



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
    /* In fact, the mongoStart() parameters related with the pool, e.g. pool size, are irrelevant,
     * given that the connection creation is mocked under UNIT_TEST in the mongoBackend library
     */
    mongoStart("localhost", "", "", "", "", false, 0, 10);
    mongoStarted = true;
  }

  connection = getMongoConnection();

  connection->dropCollection(REGISTRATIONS_COLL);
  connection->dropCollection(ENTITIES_COLL);
  connection->dropCollection(SUBSCRIBECONTEXT_COLL);
  connection->dropCollection(SUBSCRIBECONTEXTAVAIL_COLL);

  setDbPrefix(DBPREFIX);
  setRegistrationsCollectionName("registrations");
  setEntitiesCollectionName("entities");
  setSubscribeContextCollectionName("csubs");
  setSubscribeContextAvailabilityCollectionName("casubs");
}



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

      if (!equalEntity(cerExpected->contextElement.entityId, cerArg->contextElement.entityId))
      {
        LM_M(("entity doesn't match in ContextElementResponseVector comparison, continue ..."));
        continue; /* loop in jx */
      }

      /* If there aren't attributes to check, then early exits the loop */
      if (cerExpected->contextElement.contextAttributeVector.size() == 0)
      {
        LM_M(("entity (without attributes) matches in ContextElementResponseVector comparison, check next one..."));
        entityMatch = true;
        break; /* loop in jx */
      }

      if (equalContextAttributeVector(cerExpected->contextElement.contextAttributeVector,
                                      cerArg->contextElement.contextAttributeVector))
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



/* ****************************************************************************
*
* equalContextRegistrationAttribute -
*/
static bool equalContextRegistrationAttribute
(
  ContextRegistrationAttribute* craExpected,
  ContextRegistrationAttribute* craArg
)
{
  LM_M(("craArg '%s', '%s'",
        craArg->name.c_str(),
        craArg->type.c_str()));

  LM_M(("craExpected '%s', '%s'",
        craExpected->name.c_str(),
        craExpected->type.c_str()));

  if (craArg->name == craExpected->name && craArg->type == craExpected->type)
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
* equalContextRegistrationAttributeVector -
*/
static bool equalContextRegistrationAttributeVector
(
  ContextRegistrationAttributeVector  craExpectedV,
  ContextRegistrationAttributeVector  craArgV
)
{
  /* Check vector size */
  if (craExpectedV.size() != craArgV.size())
  {
    LM_M(("different sizes: expected %d, actual %d", craExpectedV.size(), craArgV.size()));
    return false;
  }

  /* Check that every context registration attribute in 'craArgV' is in 'craExpectedV'. Order doesn't matter */
  for (unsigned int ix = 0; ix < craArgV.size(); ++ix)
  {
    bool contextRegistrationAttributeMatches = false;

    for (unsigned int jx = 0; jx < craExpectedV.size(); ++jx)
    {
      ContextRegistrationAttribute* craArg = craArgV[ix];
      ContextRegistrationAttribute* craExpected = craExpectedV[jx];

      LM_M(("%d == %d?", ix, jx));
      if (equalContextRegistrationAttribute(craExpected, craArg))
      {
        LM_M(("context registration attribute matches in ContextRegistrationAttributeVector comparison, check next"));
        contextRegistrationAttributeMatches = true;
        break; /* loop in jx */
      }
    }

    if (!contextRegistrationAttributeMatches)
    {
      LM_M(("post-lookup: context registration attribute doesn't match ContextRegistrationAttributeVector comparison"));
      return false;
    }
  }

  LM_M(("ContextRegistrationAttributeVector comparison ok"));
  return true;
}



/* ****************************************************************************
*
* equalEntityIdVector -
*/
static bool equalEntityIdVector(EntityIdVector enExpectedV, EntityIdVector enArgV)
{
  /* Check vector size */
  if (enExpectedV.size() != enArgV.size())
  {
    LM_M(("different sizes: expected %d, actual %d", enExpectedV.size(), enArgV.size()));
    return false;
  }

  /* Check that every entity in 'enArgV' is in 'enExpectedV'. Order doesn't matter */
  for (unsigned int ix = 0; ix < enArgV.size(); ++ix)
  {
    bool entityMatch = false;

    for (unsigned int jx = 0; jx < enExpectedV.size(); ++jx)
    {
      EntityId enArg      = *enArgV[ix];
      EntityId enExpected = *enExpectedV[jx];

      LM_M(("%d == %d?", ix, jx));
      if (equalEntity(enExpected, enArg))
      {
        LM_M(("entity matches in EntityIdVector comparison, check next one..."));
        entityMatch = true;
        break; /* loop in jx */
      }
    }

    if (!entityMatch)
    {
      LM_M(("after looking everyone, entity doesn't match in EntityIdVector"));
      return false;
    }
  }

  LM_M(("EntityIdVector comparison ok"));
  return true;
}



/* ****************************************************************************
*
* equalContextRegistrationResponseVector -
*/
static bool equalContextRegistrationResponseVector
(
  ContextRegistrationResponseVector crrExpectedV,
  ContextRegistrationResponseVector crrArgV
)
{
  /* Check vector size */
  if (crrExpectedV.size() != crrArgV.size())
  {
    LM_M(("different sizes: expected %d, actual %d", crrExpectedV.size(), crrArgV.size()));
    return false;
  }

  /* Check that every context registration in 'crrArgV' is in 'crrExpectedV'. Order doesn't matter */
  for (unsigned int ix = 0; ix < crrArgV.size(); ++ix)
  {
    bool contextRegistrationMatch = false;

    for (unsigned int jx = 0; jx < crrExpectedV.size(); ++jx)
    {
      ContextRegistration crArg       = crrArgV[ix]->contextRegistration;
      ContextRegistration crExpected  = crrExpectedV[jx]->contextRegistration;

      LM_M(("%d == %d?", ix, jx));
      if (!equalEntityIdVector(crExpected.entityIdVector, crArg.entityIdVector))
      {
        LM_M(("entity vector doesn't match in ContextRegistrationResponseVector comparison, continue..."));
        continue; /* loop in jx */
      }

      if (!equalContextRegistrationAttributeVector(crExpected.contextRegistrationAttributeVector,
                                                   crArg.contextRegistrationAttributeVector))
      {
        LM_M(("context registration attribute vector doesn't match in ContextRegistrationResponseVector comparison"));
        continue; /* loop in jx */
      }

      if (crExpected.providingApplication.get() == crArg.providingApplication.get())
      {
        contextRegistrationMatch = true;
        LM_M(("context registration match in ContextRegistrationResponseVector comparison, check next one..."));
        break; /* loop in jx */
      }
    }

    if (!contextRegistrationMatch)
    {
      LM_M(("after looking everyone, context registration doesn't matches in ContextElementResponseVector comparison"));
      return false;
    }
  }

  LM_M(("ContextElementResponseVector comparison ok"));
  return true;
}



/* ****************************************************************************
*
* matchNotifyContextAvailabilityRequest -
*
* We need a matcher to compare NotifyContextAvailabilityRequest in EXPECT_CALL() for
* sendNotifyContextAvailabilityRequest, due to NotifyContextAvailabilityRequest is
* not yet a full fledged object and we can not use the '==' method and Eq() matcher.
* FIXME
*/
bool matchNotifyContextAvailabilityRequest
(
  NotifyContextAvailabilityRequest* expected,
  NotifyContextAvailabilityRequest* arg
)
{
  LM_M(("inside MatchNcar"));

  /* The isEmpty() check is needed because, in some occasion, we don't have any expectations on
   * subscriptionId and, in these cases, we don't check its value */
  if (!expected->subscriptionId.isEmpty() && (expected->subscriptionId.get() != arg->subscriptionId.get()))
  {
    return false;
  }

  if (!equalContextRegistrationResponseVector(expected->contextRegistrationResponseVector,
                                              arg->contextRegistrationResponseVector))
  {
    return false;
  }

  return true;
}
