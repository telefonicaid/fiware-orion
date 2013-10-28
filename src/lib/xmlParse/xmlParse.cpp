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
* Author: Ken Zangelin
*/
#include <stdio.h>

#include "xmlParse/XmlNode.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "ngsi/ParseData.h"
#include "ngsi/EntityId.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlParse.h"



/* ****************************************************************************
*
* xmlParse - 
*/
void xmlParse(xml_node<>* father, xml_node<>* node, std::string indentation, std::string fatherPath, XmlNode* parseVector, ParseData* reqDataP)
{
  if (node == NULL)
    return;

  if (node->name() == NULL)
    return;

  if (node->name()[0] == 0)
    return;

  std::string path = fatherPath + "/" + node->name();


  //
  // Lookup node in the node vector
  //
  bool treated = false;
  for (unsigned int ix = 0; parseVector[ix].path != "LAST"; ++ix)
  {
    if (path == parseVector[ix].path)
    {
      int r;

      if ((r = parseVector[ix].treat(node, reqDataP)) != 0)
        LM_E(("parse vector treat function error: %d", r));

      treated = true;
      break;
    }

  }

  if (treated == false)
    LM_W(("Warning: node '%s' not treated (%s)", path.c_str(), node->name()));

  xml_node<>* child = node->first_node();

  while (child != NULL)
  {
    xmlParse(node, child, indentation + "  ", path, parseVector, reqDataP);
    child = child->next_sibling();
  }
}



/* ****************************************************************************
*
* xmlNullTreat - 
*/
int nullTreat(xml_node<>* node, ParseData* reqDataP)
{
  LM_T(LmtNullNode, ("Not treating node '%s'", node->name()));
  return 0;
}



/* ****************************************************************************
*
* entityIdParse - 
*/
std::string entityIdParse(RequestType requestType, xml_node<>* node, EntityId* entityIdP)
{
  for (xml_attribute<> *attr = node->first_attribute(); attr; attr = attr->next_attribute())
  {
    if (attr->name() == std::string("type"))
    {
      entityIdP->type = attr->value();
      LM_T(LmtEntityId, ("Got a type for an entity: '%s'", entityIdP->type.c_str()));
    }
    else if (attr->name() == std::string("isPattern"))
    {
      entityIdP->isPattern = attr->value();
      LM_T(LmtEntityId, ("Got an isPattern for an entity: '%s'", entityIdP->isPattern.c_str()));
    }
    else if (attr->name() == std::string("id"))
    {
      entityIdP->id = attr->value();
      LM_T(LmtEntityId, ("Got an id for an entity: '%s'", entityIdP->id.c_str()));
    }
    else
       LM_RE("unsupported attribute for EntityId", ("Warning: unsupported attribute '%s' for EntityId", attr->name()));
  }

  return "OK";
}
