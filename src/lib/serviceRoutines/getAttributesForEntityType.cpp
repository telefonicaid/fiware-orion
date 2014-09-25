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
#include "utility/EntityTypesAttributesResponse.h"
#include "rest/uriParamNames.h"
#include "serviceRoutines/getAttributesForEntityType.h"

#include "mongoBackend/mongoQueryTypes.h"


/* ****************************************************************************
*
* getAttributesForEntityType -
*
* Response in XML
* <entityTypeAttributesResponse>
*   <attributesfound>4</attributesfound>
*   <attributes>
*     <attribute>Name of Attr 1</attribute>
*     <attribute>Name of Attr 2</attribute>
*     <attribute>Name of Attr 3</attribute>
*     <attribute>Name of Attr 4</attribute>
*   </attributes>
* </entityTypeAttributesResponse>
*
* AND, when returning also the attribute-type:
* <entityTypeAttributesResponse>
*   <attributesfound>4</attributesfound>
*   <attributes>
*     <attribute>
*       <name>Name of Attr 1</name>
*       <type>Type of Attr 1</type>
*     </attribute>
*   </attributes>
* </entityTypeAttributesResponse>
*
* Response in JSON?
* {
*   "attributesFound" : 4,
*   "attributes" : [
*     "Name of Attribute 1",
*     "Name of Attribute 2",
*     "Name of Attribute 3",
*     "Name of Attribute 4"
*   ]
* }
*
*/
std::string getAttributesForEntityType
(
  ConnectionInfo*            ciP,
  int                        components,
  std::vector<std::string>&  compV,
  ParseData*                 parseDataP
)
{
  EntityTypesAttributesResponse  response;
  std::string                    entityTypeName = (ciP->tenantFromUrl != "")? compV[3] : compV[2];

#if 1
  mongoAttributesForEntityType(entityTypeName, &response, ciP->tenant, ciP->servicePathV, ciP->uriParam);
#else
  //
  // mongoAttributesForEntityType is not implemented
  // Instead, I create a response by hand, to test the render method
  //
  response.entityType.type = entityTypeName;
  response.entityType.contextAttributeVector.push_back(new ContextAttribute("temperature", "celsius", ""));
  response.entityType.contextAttributeVector.push_back(new ContextAttribute("pressure",    "mmHg",    ""));
#endif

  response.statusCode.fill(SccOk);
  std::string rendered = response.render(ciP, "");
  response.release();

  return rendered;
}
