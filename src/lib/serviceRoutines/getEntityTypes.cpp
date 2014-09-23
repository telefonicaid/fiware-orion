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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <string>
#include <vector>

#include "rest/ConnectionInfo.h"
#include "ngsi/ParseData.h"
#include "utility/EntityTypesResponse.h"
#include "serviceRoutines/getEntityTypes.h"



/* ****************************************************************************
*
* getEntityTypes - 
*
* Response in XML:
* <entityTypesResponse>
*   <entityTypesFound>4</entityTypesFound>
*   <entityTypes>
*     <entityType>Name of Type 1</entityType>
*     <entityType>Name of Type 2</entityType>
*     <entityType>Name of Type 3</entityType>
*     <entityType>Name of Type 4</entityType>
*   <entityTypes>
* </entityTypesResponse>
*
* Response in JSON:
* {
*   "typesFound" : 4,
*   "types" : [
*     "Name of Type 1",
*     "Name of Type 2",
*     "Name of Type 3",
*     "Name of Type 4"
*   ]
* }
*/
std::string getEntityTypes(ConnectionInfo* ciP, int components, std::vector<std::string>& compV, ParseData* parseDataP)
{
  EntityTypesResponse response;

#if 0
  mongoEntityTypes(&response, ciP->tenant, ciP->servicePathV);
#else
  //
  // mongoEntityTypes is not implemented
  // Instead, I create a response by hand, to test the render method
  //
  
  TypeEntity* te1 = new TypeEntity("Type 1");
//  TypeEntity* te2 = new TypeEntity("Type 2");
//  TypeEntity* te3 = new TypeEntity("Type 3");
  
  te1->contextAttributeVector.push_back(new ContextAttribute("A1", "at1", ""));

//  te2->contextAttributeVector.push_back(new ContextAttribute("A1", "at1", ""));
//  te2->contextAttributeVector.push_back(new ContextAttribute("A2", "at2", ""));

//  te3->contextAttributeVector.push_back(new ContextAttribute("A1", "at1", ""));
//  te3->contextAttributeVector.push_back(new ContextAttribute("A2", "at2", ""));
//  te3->contextAttributeVector.push_back(new ContextAttribute("A3", "at3", ""));

  response.typeEntityVector.push_back(te1);
//  response.typeEntityVector.push_back(te2);
//  response.typeEntityVector.push_back(te3);
#endif

  response.statusCode.fill(SccOk);
  std::string rendered = response.render(ciP, "");
  response.release();

  return rendered;
}
