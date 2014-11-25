#ifndef SRC_LIB_XMLPARSE_XMLPARSE_H_
#define SRC_LIB_XMLPARSE_XMLPARSE_H_

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
* Author: Ken Zangelin
*/
#include <string>

#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "xmlParse/XmlNode.h"



/* ****************************************************************************
*
* xmlParse - 
*/
extern void xmlParse
(
  ConnectionInfo*     ciP,
  xml_node<>*         father,
  xml_node<>*         node,
  const std::string&  indentation,
  const std::string&  fatherPath,
  XmlNode*            treatV,
  ParseData*          reqData,
  std::string*        errorMsgP = NULL
);



/* ****************************************************************************
*
* xmlNullTreat - 
*/
extern int nullTreat(xml_node<>* node, ParseData* reqData);



/* ****************************************************************************
*
* entityIdParse - 
*/
#include "ngsi/Request.h"
struct EntityId;
extern std::string entityIdParse(RequestType request, xml_node<>* node, EntityId* entityIdP);



/* ****************************************************************************
*
* xmlTypeAttributeGet - 
*/
extern std::string xmlTypeAttributeGet(xml_node<>* node);

#endif  // SRC_LIB_XMLPARSE_XMLPARSE_H_
