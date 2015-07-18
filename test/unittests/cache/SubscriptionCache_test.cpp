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
  regcomp(&ei2->entityIdPattern, "E2.*", 0);

  ei1->entityType = "at1";
  ei2->entityType = "at2";
  entityIdInfos.push_back(ei1);
  entityIdInfos.push_back(ei2);

  subP = new Subscription("012345678901234567890123", entityIdInfos, attributeV, 5, -1, restriction, nv, "REFERENCE");

  cache->insert(subP);

  subP = cache->lookupById("012345678901234567890123");
  EXPECT_TRUE(subP != NULL);
  EXPECT_EQ("012345678901234567890123", subP->subscriptionId);

  std::vector<Subscription*> subV;
  cache->lookup("E10", "", "attr1", &subV);
  ASSERT_EQ(1, subV.size());
  EXPECT_EQ("012345678901234567890123", subV[0]->subscriptionId);
  subV.clear();

  cache->lookup("E20", "", "attr1", &subV);
  ASSERT_EQ(1, subV.size());
  EXPECT_EQ("012345678901234567890123", subV[0]->subscriptionId);
  subV.clear();

  cache->lookup("E30", "", "attr1", &subV);
  EXPECT_EQ(0, subV.size());
  subV.clear();

  cache->lookup("E30", "", "attr4", &subV);
  EXPECT_EQ(0, subV.size());
  subV.clear();

  cache->lookup("E10", "at1", "attr3", &subV);
  ASSERT_EQ(1, subV.size());
  EXPECT_EQ("012345678901234567890123", subV[0]->subscriptionId);
  subV.clear();

  cache->lookup("E10", "at3", "attr4", &subV);
  ASSERT_EQ(0, subV.size());
  subV.clear();

  subP = new Subscription("012345678901234567890124", entityIdInfos, attributeV, 5, -1, restriction, nv, "REFERENCE");
  cache->insert(subP);

  cache->lookup("E10", "", "attr3", &subV);
  ASSERT_EQ(2, subV.size());
  EXPECT_EQ("012345678901234567890123", subV[0]->subscriptionId);
  EXPECT_EQ("012345678901234567890124", subV[1]->subscriptionId);
  subV.clear();
}
