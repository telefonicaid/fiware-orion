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
#include "parse/ComplexValueNode.h"
#include "rest/ConnectionInfo.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlParse.h"


/* ****************************************************************************
*
* complexValueRoots - 
*/
const char* complexValueRootV[] =
{
  "/updateContextRequest/contextElementList/contextElement/contextAttributeList/contextAttribute/contextValue" 
};



static bool isComplexValuePath(const char* path, std::string& root, std::string& rest)
{
   unsigned int len;

   root = "";
   rest = "";

   for (unsigned int ix = 0; ix < sizeof(complexValueRootV) / sizeof(complexValueRootV[0]); ++ix)
   {
      len = strlen(complexValueRootV[ix]);

      if (strlen(path) < len)
          continue;

      if (strncmp(complexValueRootV[ix], path, len) == 0)
      {
         root = complexValueRootV[ix];
         rest = &path[len];

         return true;
      }
   }

   return false;
}


/* ****************************************************************************
*
* xmlTypeAttributeGet - 
*/
static std::string xmlTypeAttributeGet(xml_node<>* node)
{
  std::string type = "";

  for (xml_attribute<> *attr = node->first_attribute(); attr; attr = attr->next_attribute())
  {
    if (attr->name() == std::string("type"))
    {
       if (type != "")
         return "more than one 'type' attribute";
       type = attr->value();
    }
    else
      return std::string("unknown attribute '") + attr->name() + "'";
  }

  return type;
}



/* ****************************************************************************
*
* xmlParse - 
*/
void xmlParse(ConnectionInfo* ciP, xml_node<>* father, xml_node<>* node, std::string indentation, std::string fatherPath, XmlNode* parseVector, ParseData* parseDataP)
{
  if ((node == NULL) || (node->name() == NULL))
    return;

  if ((node->name()[0] == 0) && (ciP->complexValueContainer == NULL))
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

      if ((r = parseVector[ix].treat(node, parseDataP)) != 0)
      {
        fprintf(stderr, "parse vector treat function error: %d\n", r);
        LM_E(("parse vector treat function error: %d", r));
      }

      treated = true;
      break;
    }
  }

  if (treated == false)
  {
    std::string root;
    std::string rest;

    if (isComplexValuePath(fatherPath.c_str(), root, rest))
    {
      std::string  name   = node->name();
      std::string  value  = node->value();
      std::string  type   = xmlTypeAttributeGet(node);

      if ((type != "") && (type != "vector"))
      {
         ciP->httpStatusCode = SccBadRequest;
         ciP->answer = std::string("Bad value for XML attribute 'type' for '") + node->name() + "': '" + type + "'";
         LM_W(("ERROR: '%s', PATH: '%s'   ", ciP->answer.c_str(), fatherPath.c_str()));
         return;
      }

      if (rest == "")  // Toplevel
      {
        if (ciP->complexValueContainer == NULL) // Toplevel start
        {
          LM_T(LmtComplexValue, ("Complex value start for '%s'", fatherPath.c_str()));
          ciP->complexValueContainer = new orion::ComplexValueNode(root);
          ciP->complexValueNode.push_back(ciP->complexValueContainer);
        }
        else // Toplevel END
        {
          LM_T(LmtComplexValue, ("Complex value end for '%s'", fatherPath.c_str()));
          std::string status = ciP->complexValueContainer->finish();
          parseDataP->lastContextAttribute->complexValueP = ciP->complexValueContainer;
          ciP->complexValueContainer = NULL;

          if (status != "")
          {
            ciP->httpStatusCode = SccBadRequest;
            ciP->answer = std::string("complex value error: ") + status;
            LM_W(("ERROR: '%s', PATH: '%s'   ", ciP->answer.c_str(), fatherPath.c_str()));
            return;
          }
        }
      }

      if ((value == " ") && (name != ""))
      {
         if (type == "vector")
           ciP->complexValueContainer->add(orion::ComplexValueNode::Vector, name, rest);
         else
           ciP->complexValueContainer->add(orion::ComplexValueNode::Struct, name, rest);
      }
      else if (name != "")
        ciP->complexValueContainer->add(orion::ComplexValueNode::Leaf, name, rest, value);
    }
    else
    {
      ciP->httpStatusCode = SccBadRequest;
      if (ciP->answer == "")
        ciP->answer = std::string("Unknown XML field: '") + node->name() + "'";
      LM_W(("ERROR: '%s', PATH: '%s'   ", ciP->answer.c_str(), fatherPath.c_str()));
      return;
    }
  }

  xml_node<>* child = node->first_node();

  while (child != NULL)
  {
    xmlParse(ciP, node, child, indentation + "  ", path, parseVector, parseDataP);
    child = child->next_sibling();
  }
}



/* ****************************************************************************
*
* xmlNullTreat - 
*/
int nullTreat(xml_node<>* node, ParseData* parseDataP)
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
    else
      LM_RE("unsupported attribute for EntityId", ("Warning: unsupported attribute '%s' for EntityId", attr->name()));
  }

  return "OK";
}
