/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Ken Zangelin
*/
#include "gtest/gtest.h"
#include "testInit.h"

#include "ngsi/Restriction.h"
#include "ngsi/Reference.h"
#include "ngsi/NotifyConditionVector.h"
#include "ngsi/EntityId.h"
#include "ngsi/ContextAttribute.h"
#include "cache/SubscriptionCache.h"



/* ****************************************************************************
*
* - 
*/
TEST(cache, SubscriptionCache)
{
  std::vector<EntityInfo*>  entityIdInfos;
  std::vector<std::string>  attributeV;
  Restriction               restriction;
  NotifyConditionVector     nv;
  Reference                 reference;
  SubscriptionCache*        cache = new SubscriptionCache();
  Subscription*             subP;
  EntityInfo*               ei1 = new EntityInfo();
  EntityInfo*               ei2 = new EntityInfo();

  ASSERT_TRUE(cache != NULL);

  attributeV.push_back("attr1");
  attributeV.push_back("attr2");
  attributeV.push_back("attr3");

  regcomp(&ei1->entityIdPattern, "E1.*", 0);

  ei1->entityType = "at1";
  ei2->entityType = "at2";

  subP = new Subscription("012345678901234567890123", entityIdInfos, attributeV, 5, -1, restriction, nv, "REFERENCE");

  cache->insert(subP);

  subP = cache->lookupById("012345678901234567890123");
  EXPECT_TRUE(subP != NULL);
  EXPECT_EQ("012345678901234567890123", subP->subscriptionId);

  subP = cache->lookup("E10", "", "at1");
  EXPECT_TRUE(subP != NULL);
  EXPECT_EQ("012345678901234567890123", subP->subscriptionId);

  subP = cache->lookup("E20", "", "at1");
  EXPECT_TRUE(subP == NULL);
}
