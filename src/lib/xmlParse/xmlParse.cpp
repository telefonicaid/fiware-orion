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
#include "common/wsStrip.h"
#include "common/string.h"
#include "ngsi/ParseData.h"
#include "ngsi/EntityId.h"
#include "parse/CompoundValueNode.h"
#include "parse/compoundValue.h"
#include "rest/ConnectionInfo.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlParse.h"


/* ****************************************************************************
*
* compoundValueRoots - 
*/
const char* compoundValueRootV[] =
{
  "/updateContextRequest/contextElementList/contextElement/contextAttributeList/contextAttribute/contextValue"
};



/* ****************************************************************************
*
* isCompoundValuePath - 
*/
static bool isCompoundValuePath(const char* path, std::string& root, std::string& rest)
{
   unsigned int len;

   root = "";
   rest = "";

   for (unsigned int ix = 0; ix < sizeof(compoundValueRootV) / sizeof(compoundValueRootV[0]); ++ix)
   {
      len = strlen(compoundValueRootV[ix]);

      if (strlen(path) < len)
         continue;

      if (strncmp(compoundValueRootV[ix], path, len) == 0)
      {
         root = compoundValueRootV[ix];
         rest = &path[len];

         return true;
      }
   }

   return false;
}



/* ****************************************************************************
*
* treat - 
*/
static bool treat(xml_node<>* node, std::string path, XmlNode* parseVector, ParseData* parseDataP)
{
  for (unsigned int ix = 0; parseVector[ix].path != "LAST"; ++ix)
  {
    if (path == parseVector[ix].path)
    {
      int r;

      if ((r = parseVector[ix].treat(node, parseDataP)) != 0)
        LM_E(("parse vector treat function error: %d", r));

      return true; // Node has been treated
    }
  }

  return false; // Node was not found in the parse vector
}



/* ****************************************************************************
*
* xmlTypeFromString - 
*/
orion::CompoundValueNode::Type xmlTypeFromString(std::string xmlAttribute, std::string name, std::string value)
{
  if (xmlAttribute == "vector")
    return orion::CompoundValueNode::Vector;
  else if (onlyWs(value.c_str()) && (name != ""))
    return orion::CompoundValueNode::Struct;

  return orion::CompoundValueNode::Leaf;
}



/* ****************************************************************************
*
* xmlParse - 
*/
void xmlParse
(
   ConnectionInfo*  ciP,
   xml_node<>*      father,
   xml_node<>*      node,
   std::string      indentation,
   std::string      fatherPath,
   XmlNode*         parseVector,
   ParseData*       parseDataP
)
{
  if ((node == NULL) || (node->name() == NULL))
    return;

  if ((node->name()[0] == 0) && (ciP->compoundValueP == NULL))
    return;

  std::string  root;
  std::string  rest;
  std::string  path       = fatherPath + "/" + node->name();
  std::string  name       = node->name();
  std::string  value      = node->value();
  bool         isCompound = isCompoundValuePath(fatherPath.c_str(), root, rest);
  bool         treated    = treat(node, path, parseVector, parseDataP);

  LM_T(LmtCompoundValue, ("--------------------------------------"));
  LM_T(LmtCompoundValue, ("fatherPath: '%s'", fatherPath.c_str()));
  LM_T(LmtCompoundValue, ("path:       '%s'", path.c_str()));
  LM_T(LmtCompoundValue, ("root:       '%s'", root.c_str()));
  LM_T(LmtCompoundValue, ("rest:       '%s'", rest.c_str()));
  //
  // Here we have 6 possibilities:
  //   1. Compound Start
  //      - ciP->inCompound == false
  //      - isCompound      == true
  //
  //   2. In the middle of a Compound
  //      - ciP->inCompound == true
  //      - isCompound      == true
  //
  //   3. Compound container ends
  //      - ciP->inCompound == true
  //      - isCompound      == true
  //      - strlen(path) < strlen(oldpath)
  //
  //   4. Compound ends
  //      - ciP->inCompound == true
  //      - isCompound      == false
  //
  //   5. Normal treated path
  //      - isCompound      == false
  //      - ciP->inCompound == false
  //      - treated         == true
  //
  //   6. Untreated path that should give a parse error
  //      - isCompound      == false
  //      - ciP->inCompound == false
  //      - treated         == false
  //
  std::string  value2        = node->value();
  char*        trimmedValue  = wsStrip((char*) value2.c_str());

  LM_T(LmtCompoundValueRaw, ("%s: value: '%s'", path.c_str(), trimmedValue));

  if ((isCompound == true) && (ciP->inCompoundValue == false))
  {
    std::string                     xmlAttribute = xmlTypeAttributeGet(node);
    orion::CompoundValueNode::Type  type         = xmlTypeFromString(xmlAttribute, name, value);

    orion::compoundValueStart(ciP, path, name, value, root, rest, type);
  }
  else if ((isCompound == true) && (ciP->inCompoundValue == true))
  {
    std::string                     xmlAttribute  = xmlTypeAttributeGet(node);
    orion::CompoundValueNode::Type  type          = xmlTypeFromString(xmlAttribute, name, value);

    LM_T(LmtCompoundValueStep, ("Step up?  New path:     '%s'", rest.c_str()));
    LM_T(LmtCompoundValueStep, ("Step up?  Old path:     '%s'", ciP->compoundValueP->path.c_str()));
    LM_T(LmtCompoundValueStep, ("Step up?  trimmedValue: '%s'", trimmedValue));
    LM_T(LmtCompoundValueStep, ("Step in?  Name:         '%s'", name.c_str()));
    
    if ((xmlAttribute != "vector") && (xmlAttribute !=""))
    {
      ciP->httpStatusCode = SccBadRequest;
      ciP->answer = std::string("Bad value for XML attribute 'type' for '") + node->name() + "': '" + xmlAttribute + "'";
      LM_W(("ERROR: '%s', PATH: '%s'   ", ciP->answer.c_str(), fatherPath.c_str()));
      return;
    }
    else if ((rest.size() < ciP->compoundValueP->path.size()) && (trimmedValue[0] == 0) && (fatherPath.size() + 1 == path.size()))
    {
      ciP->compoundValueP = ciP->compoundValueP->container;
      LM_T(LmtCompoundValueContainer, ("Set current container to '%s' (%s)  [after stepping up]", ciP->compoundValueP->path.c_str(), ciP->compoundValueP->name.c_str()));
    }
    else if (name != "")
      orion::compoundValueMiddle(ciP, rest, name, value, type);
  }
  else if ((isCompound == false) && (ciP->inCompoundValue == true))
  {
    orion::compoundValueEnd(ciP, path, name, value, fatherPath, parseDataP);
  }
  else if ((isCompound == false) && (ciP->inCompoundValue == false) && (treated == false))
  {
    ciP->httpStatusCode = SccBadRequest;
    if (ciP->answer == "")
      ciP->answer = std::string("Unknown XML field: '") + node->name() + "'";
    LM_W(("ERROR: '%s', PATH: '%s'   ", ciP->answer.c_str(), fatherPath.c_str()));
    return;
  }
  else if ((isCompound == false) && (ciP->inCompoundValue == false) && (treated == true))
    LM_T(LmtCompoundValue, ("OK:              '%s'", path.c_str()));

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



/* ****************************************************************************
*
* xmlTypeAttributeGet - 
*/
std::string xmlTypeAttributeGet(xml_node<>* node)
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
