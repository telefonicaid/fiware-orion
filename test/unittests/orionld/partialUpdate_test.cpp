/*
*
* Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
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
extern "C"
{
#include "kjson/kjBuilder.h"                                   // kjObject, kjString, kjChildAdd
#include "kalloc/kaBufferInit.h"                               // kaBufferInit
#include "kalloc/kaBufferReset.h"                              // kaBufferReset
}

#include "logMsg/logMsg.h"
#include "rest/ConnectionInfo.h"
#include "orionld/serviceRoutines/orionldPostEntity.h"
#include "orionld/common/orionldState.h"

#include "unittest.h"



/* ****************************************************************************
*
* partialUpdateCreation -
*/
TEST(orionld, partialUpdateCreation)
{
  kaBufferInit(&orionldState.kalloc, orionldState.kallocBuffer, sizeof(orionldState.kallocBuffer), 2 * 1024, NULL, "Thread KAlloc buffer");

  orionldStateInit();
  ConnectionInfo    ci("/ngsi-ld/v1/entities/urn:entity:E1/attrs", "POST", "1.1");

  LM_TMP(("In partialUpdateCreation"));

  utInit();

  orionldStateErrorAttributeAdd("A1");
  orionldStateErrorAttributeAdd("A2");
  orionldStateErrorAttributeAdd("A3");

  EXPECT_STREQ("|A1|A2|A3|", orionldState.errorAttributeArrayP);
  
  //
  // Now force a malloc (by making orionldState.errorAttributeArrayP > 512 bytes)
  //
  orionldStateErrorAttributeAdd("A0_0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");
  orionldStateErrorAttributeAdd("A1_0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");
  orionldStateErrorAttributeAdd("A2_0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");
  orionldStateErrorAttributeAdd("A3_0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");
  orionldStateErrorAttributeAdd("A4_0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");
  orionldStateErrorAttributeAdd("A5_0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");

  EXPECT_STREQ("|A1|A2|A3|A0_0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789|A1_0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789|A2_0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789|A3_0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789|A4_0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789|A5_0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789|", orionldState.errorAttributeArrayP);

  orionldStateRelease();
  kaBufferReset(&orionldState.kalloc, false);
  utExit();
}



/* ****************************************************************************
*
* partialUpdateResponse -
*/
TEST(orionld, partialUpdateResponse)
{
  kaBufferInit(&orionldState.kalloc, orionldState.kallocBuffer, sizeof(orionldState.kallocBuffer), 2 * 1024, NULL, "Thread KAlloc buffer");

  orionldStateInit();
  ConnectionInfo       ci("/ngsi-ld/v1/entities/urn:entity:E1/attrs", "POST", "1.1");
  KjNode*              attrNodeP;
  KjNode*              kNodeP;
  const char*          attrNamesV[3] = { "A1", "A2", "A3" };
  utInit();

  LM_TMP(("In partialUpdateResponse"));
  orionldStateInit();

  //
  // Create a request tree with attributes A1, A2, and A3
  //
  ci.requestTree = kjObject(orionldState.kjsonP, NULL);

  for (unsigned int ix = 0; ix < sizeof(attrNamesV) / sizeof(attrNamesV[0]); ix++)
  {
    attrNodeP = kjObject(orionldState.kjsonP, attrNamesV[ix]);

    kNodeP = kjString(orionldState.kjsonP, "type", "Property");
    kjChildAdd(attrNodeP, kNodeP);
    
    kNodeP = kjString(orionldState.kjsonP, "value", attrNamesV[ix]);
    kjChildAdd(attrNodeP, kNodeP);

    kjChildAdd(ci.requestTree, attrNodeP);
  }

  //
  // Add some Error Attributes
  //
  orionldStateErrorAttributeAdd("A1");
  orionldStateErrorAttributeAdd("A3");

  EXPECT_STREQ("|A1|A3|", orionldState.errorAttributeArrayP);

  //
  // Create the Response Tree (with only A2) 
  //
  LM_TMP(("Calling orionldPartialUpdateResponseCreate"));
  orionldPartialUpdateResponseCreate(&ci);
  LM_TMP(("After orionldPartialUpdateResponseCreate"));
  EXPECT_EQ(NULL, ci.requestTree);

  //
  // Now, ci.responseTree should contain only A2
  //
  if (ci.responseTree->value.firstChildP == NULL)
  {
    EXPECT_STREQ("ci.responseTree->value.firstChildP == NULL", "It should be != NULL");
  }

  for (kNodeP = ci.responseTree->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
    LM_TMP(("  Node: '%s'", kNodeP->name));
  
  EXPECT_EQ(NULL, ci.responseTree->value.firstChildP->next);

  char* attrName = ci.responseTree->value.firstChildP->name;

  EXPECT_STREQ("A2", attrName);

  orionldStateRelease();
  kaBufferReset(&orionldState.kalloc, false);
  utExit();
}
