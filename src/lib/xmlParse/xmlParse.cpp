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



static bool isComplexValuePath(const char* path, char* root, char* rest)
{
   unsigned int len;

   root[0] = 0;
   rest[0] = 0;

   for (unsigned int ix = 0; ix < sizeof(complexValueRootV) / sizeof(complexValueRootV[0]); ++ix)
   {
      len = strlen(complexValueRootV[ix]);

      if (strlen(path) < len)
          continue;

      if (strncmp(complexValueRootV[ix], path, len) == 0)
      {
         if (root != NULL)
            strcpy(root, complexValueRootV[ix]);
         if (rest != NULL)
            strcpy(rest, &path[len]);

         return true;
      }
   }

   return false;
}



/* ****************************************************************************
*
* xmlParse - 
*/
void xmlParse(ConnectionInfo* ciP, xml_node<>* father, xml_node<>* node, std::string indentation, std::string fatherPath, XmlNode* parseVector, ParseData* reqDataP)
{
  static orion::ComplexValueNode* container = NULL;
  static int                      level     = 0;

  if ((node == NULL) || (node->name() == NULL))
  {
     LM_M(("NULL node/name for father '%s'", fatherPath.c_str()));
     return;
  }

  if (node->name()[0] == 0)
  {
    if (ciP->inComplexValue)
    {
      char root[1024];
      char rest[1024];

      isComplexValuePath(fatherPath.c_str(), root, rest);
      if (rest[0] == 0)
      {
        LM_T(LmtComplexValue, ("******************** Complex value ends here (%s)", fatherPath.c_str()));
        ciP->inComplexValue = false;
        ciP->complexValueNode[ciP->complexValueNode.size() - 1]->show("");
      }
      else
      {
        container = container->container;
        --level;
        LM_T(LmtComplexValue, ("Complex part '%s' ends - new container is '%s', level %d", rest, container->path.c_str(), level));
      }
    }
    return;
  }

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
    char root[256];
    char rest[256];

    if (isComplexValuePath(path.c_str(), root, rest))
    {
      if (ciP->inComplexValue == false)
      {
        LM_T(LmtComplexValue, ("-------------- Complex value starts here (%s)", root));

        container = new orion::ComplexValueNode(root);
        ciP->complexValueNode.push_back(container);
        LM_T(LmtComplexValue, ("Created complexValueNode number %d", ciP->complexValueNode.size()));
      }

      ciP->inComplexValue = true;

      if (node->value()[0] == 0)
        LM_T(LmtComplexValue, ("Complex part: '%s'", rest));
      else
      {
        orion::ComplexValueNode* cvNode;

        if (strcmp(node->value(), " ") == 0)
        {

          LM_T(LmtComplexValue, ("Create Struct: '%s', father is '%s'", rest, container->path.c_str()));
          cvNode = new orion::ComplexValueNode(container, rest, node->name(), node->value(), container->childV.size(), orion::ComplexValueNode::Struct, level);
          container->add(cvNode);

          container = cvNode;
          ++level;
        }
        else
        {
           cvNode = new orion::ComplexValueNode(container, rest, node->name(), node->value(), container->childV.size(), orion::ComplexValueNode::Leaf, level);
           container->add(cvNode);
           LM_T(LmtComplexValue, ("Complex part: '%s', value '%s'", rest, node->value()));
        }
      }
    }
    else
      LM_E(("Parse Error: unknown node '%s'", path.c_str(), node->name()));
  }

  xml_node<>* child = node->first_node();

  while (child != NULL)
  {
    xmlParse(ciP, node, child, indentation + "  ", path, parseVector, reqDataP);
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
    else
      LM_RE("unsupported attribute for EntityId", ("Warning: unsupported attribute '%s' for EntityId", attr->name()));
  }

  return "OK";
}
