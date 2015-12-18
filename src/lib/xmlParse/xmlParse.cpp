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
#include <stdio.h>
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/wsStrip.h"
#include "common/string.h"
#include "common/limits.h"
#include "alarmMgr/alarmMgr.h"

#include "ngsi/ParseData.h"
#include "ngsi/EntityId.h"
#include "parse/CompoundValueNode.h"
#include "parse/compoundValue.h"
#include "parse/forbiddenChars.h"
#include "rest/ConnectionInfo.h"
#include "xmlParse/XmlNode.h"
#include "xmlParse/xmlParse.h"

using namespace orion;



/* ****************************************************************************
*
* compoundValueRootV -
*
* The vector 'compoundValueRootV' contains a list of all the paths where we allow
* a compound value
*/
static const char* compoundValueRootV[] =
{
  "/updateContextRequest/contextElementList/contextElement/contextAttributeList/contextAttribute/contextValue",
  "/appendContextElementRequest/contextAttributeList/contextAttribute/contextValue",
  "/updateContextElementRequest/contextAttributeList/contextAttribute/contextValue",
  "/updateContextAttributeRequest/contextValue",
  "/notifyContextRequest/contextResponseList/contextElementResponse/contextElement/contextAttributeList/contextAttribute/contextValue"
};



/* ****************************************************************************
*
* isCompoundValuePath -
*
* This function examines a path to see whether we are inside a compound value or not.
*
* If the path doesn't belong to any compond, FALSE is returned, else TRUE.
*/
static bool isCompoundValuePath(const char* path)
{

  for (unsigned int ix = 0; ix < sizeof(compoundValueRootV) / sizeof(compoundValueRootV[0]); ++ix)
  {
    size_t len = strlen(compoundValueRootV[ix]);

    if (strlen(path) < len)
    {
      continue;
    }

    if (strncmp(compoundValueRootV[ix], path, len) == 0)
    {
      return true;
    }
  }

  return false;
}



/* ****************************************************************************
*
* treat -
*
* This is the function that actually treats a node, bu calling its treat function
* provided by src/lib/xmlRequest - the entry point of XML parsing.
*
* It simple compares the current path with the paths in the incoming vector 'parseVector'
* and if a hit is found calls the 'treat' function of that hit (the instance of the vector).
*
* If no hit is found it means that the path of the current XML node is unknown.
* This will result in either a 'PARSE ERROR' or thatthe node is part of a Compound.
*/
static bool treat(ConnectionInfo* ciP, xml_node<>* node, const std::string& path, XmlNode* parseVector, ParseData* parseDataP)
{
  for (unsigned int ix = 0; parseVector[ix].path != "LAST"; ++ix)
  {
    if (path == parseVector[ix].path)
    {
      int r;

      //
      // Before treating a node, a check is made that the value of the node has no forbidden characters.
      // However, if the the node has attributes, then the values of the attributes are checked instead.
      //
      if (node->first_attribute() == NULL)
      {
        //
        // If we're in a Scope, no forbiddenChars check made.
        // We still don't know the type of the Scope, so the forbiddenChars check
        // will have to be postponed until we know both the type and the value.
        // For example, the 'check' method can take care of this
        //
        if (strcasecmp(node->name(), "scopeValue") != 0)
        {
          if (forbiddenChars(node->value()) == true)
          {
            std::string details = std::string("found a forbidden value in '") + node->value() + "'";
            alarmMgr.badInput(clientIp, details);

            ciP->httpStatusCode = SccBadRequest;
            ciP->answer = std::string("Illegal value for XML attribute");
            return true;
          }
        }
      }
      else
      {
        for (xml_attribute<> *attr = node->first_attribute(); attr; attr = attr->next_attribute())
        {
          if (forbiddenChars(attr->value()) == true)
          {
            std::string details = std::string("found a forbidden value in attribute: '") + node->value() + "'";
            alarmMgr.badInput(clientIp, details);

            ciP->httpStatusCode = SccBadRequest;
            ciP->answer = std::string("Illegal value for XML attribute");
            return true;
          }
        }
      }

      if ((r = parseVector[ix].treat(node, parseDataP)) != 0)
      {
        char rV[STRING_SIZE_FOR_INT];
        snprintf(rV, sizeof(rV), "%d", r);
        std::string details = std::string("xml parse error ") + rV;
        alarmMgr.badInput(clientIp, details);
      }

      return true;  // Node has been treated
    }
  }

  return false;  // Node was not found in the parse vector
}



/* ****************************************************************************
*
* eatCompound - consume the compound tree
*
* Three types of tree nodes here (4 actually);
*   - toplevel node
*   - string
*   - object node
*   - vector node
*/
void eatCompound(ConnectionInfo* ciP, orion::CompoundValueNode* containerP, xml_node<>* node, const std::string& indent)
{
  if (containerP == NULL)  // toplevel
  {
    std::string xmlAttribute = xmlTypeAttributeGet(node);

    if (xmlAttribute == "vector")
    {
      containerP = new CompoundValueNode(orion::ValueTypeVector);
    }
    else if (xmlAttribute =="")
    {
      containerP = new CompoundValueNode(orion::ValueTypeObject);
    }
    else
    {
      ciP->httpStatusCode = SccBadRequest;

      ciP->answer = std::string("Bad value for XML attribute /type/ for /") + node->name() + "/: " + xmlAttribute;

      alarmMgr.badInput(clientIp, ciP->answer);

      return;
    }

    ciP->compoundValueRoot = containerP;
  }
  else
  {
    std::string value = wsStrip(node->value());
    std::string name  = wsStrip(node->name());

    if (value == "")   // Object OR Vector
    {
      std::string xmlAttribute = xmlTypeAttributeGet(node);

      if (xmlAttribute == "vector")
      {
        containerP = containerP->add(orion::ValueTypeVector, name, "");
      }
      else if (xmlAttribute == "")
      {
        containerP = containerP->add(orion::ValueTypeObject, name, "");
      }
      else
      {
        ciP->httpStatusCode = SccBadRequest;
        ciP->answer = std::string("Bad value for XML attribute /type/ for /") + name + "/: " + xmlAttribute;
        alarmMgr.badInput(clientIp, ciP->answer);

        return;
      }
    }
    else  // String
    {
      if (forbiddenChars(value.c_str()) == true)
      {
        std::string details = std::string("found a forbidden value in '") + value + "'";
        alarmMgr.badInput(clientIp, details);

        ciP->httpStatusCode = SccBadRequest;
        ciP->answer = std::string("Illegal value for XML attribute");
        return;
      }

      containerP->add(orion::ValueTypeString, name, value);
    }
  }

  xml_node<>* child = node->first_node();
  while (child != NULL)
  {
    if (child->name()[0] != 0)
    {
      eatCompound(ciP, containerP, child, indent + "  ");
    }

    child = child->next_sibling();
  }
}



/* ****************************************************************************
*
* xmlParse -
*
* This function is called once (actually it is not that simple) for each node in
* the tree that the XML parser has built for us.
*
*
* In the node '<contextValue>', isCompoundValuePath returns TRUE.
* Under "<contextValue>", we have either a simple string or a Compound Value Tree.
* In the case of a "simple string" the value of the current node is NON EMPTY,
* and as the node is treated already in the previous call to 'treat()', no further action is needed.
*
* If a node is NOT TREATED and it is NOT a compound, a parse error is issued
*/
void xmlParse
(
  ConnectionInfo*     ciP,
  xml_node<>*         father,
  xml_node<>*         node,
  const std::string&  indentation,
  const std::string&  fatherPath,
  XmlNode*            parseVector,
  ParseData*          parseDataP,
  std::string*        errorMsgP
)
{
  std::string  value            = wsStrip(node->value());
  std::string  name             = wsStrip(node->name());
  std::string  path             = fatherPath + "/" + name;
  bool         treated          = treat(ciP, node, path, parseVector, parseDataP);

  if (isCompoundValuePath(path.c_str()) && (value == "") && (node->first_node() != NULL))
  {
    //
    // Count children (to avoid false compounds because of just en empty sttribute value)
    //
    xml_node<>* child    = node->first_node();
    int         children = 0;
    while (child != NULL)
    {
      if (child->name()[0] != 0)
      {
        ++children;
      }

      child = child->next_sibling();
    }

    if (children == 0)  // NOT a compound value
    {
      return;
    }

    eatCompound(ciP, NULL, node, "");
    compoundValueEnd(ciP, parseDataP);

    return;
  }
  else  if (treated == false)
  {
    ciP->httpStatusCode = SccBadRequest;
    if (ciP->answer == "")
    {
      ciP->answer = std::string("Unknown XML field: ") + name.c_str();
    }

    alarmMgr.badInput(clientIp, ciP->answer);

    if (errorMsgP)
    {
      *errorMsgP = std::string("Bad Input: ") + ciP->answer;
    }

    return;
  }

  // Recursive calls for all children of this node
  xml_node<>* child = node->first_node();
  while (child != NULL)
  {
    if ((child != NULL) && (child->name() != NULL) && (onlyWs(child->name()) == false))
    {
      xmlParse(ciP, node, child, indentation + "  ", path, parseVector, parseDataP, errorMsgP);
    }

    child = child->next_sibling();
  }
}



/* ****************************************************************************
*
* xmlNullTreat -
*
* Some of the nodes in the XML tree are known but no action needs to ne taken.
* Especially the start of a container.
*
* We need this to distinguish between XML nodes that are 'ok but no treatment needed' and
* XML nodes that are just 'not recognized' and should give a parse error (or they belong
* to a Compound).
*/
int nullTreat(xml_node<>* node, ParseData* parseDataP)
{
  LM_T(LmtNullNode, ("Node '%s' is recognized but no action is needed", node->name()));

  return 0;
}



/* ****************************************************************************
*
* entityIdParse - help routine for the XML node entityId
*
* Created mainly to not copy the XML attribute lookup stuff
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
    {
      std::string details = std::string("unsupported attribute '") + attr->name() + "' for EntityId";
      alarmMgr.badInput(clientIp, details);

      return "unsupported attribute for EntityId";
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* xmlTypeAttributeGet -
*
* This function looks for the presence of the XML attribute 'type'.
* This attribute is used in Compound Values to indicate that a container is
* a vector and not a structure.
* Vectors don't exist in XML, all containers are considered 'objects', but as
* vectors exist in JSON and we support both formats we need a way to have vectors
* in XML.
* The way we have chosen is to use an XML attribute with the name 'type' and its
* only allowed values is "vector".
*
* This function returns:
* - "vector"       if 'type="vector"' found as an XML attribute
* - ""             if no XML attribute at all is found
* - error string   in all other cases.
*
* The following XML attribute is considered an error:
* - type="object"
* - vector="yes"
* - etc, etc.
*/
std::string xmlTypeAttributeGet(xml_node<>* node)
{
  std::string type = "";   // Value of the XML attribute 'vector' - initialized to the empty string

  // Loop over all XML attrributes of the node
  for (xml_attribute<> *attr = node->first_attribute(); attr; attr = attr->next_attribute())
  {
    // We only accept 'type' as valid XML attribute - any other name of attribute is considered an error
    if (attr->name() == std::string("type"))
    {
       if (type != "")  // set already?
       {
         return "more than one /type/ attribute";
       }

       type = attr->value();  // save the value of the attribute 'type'
    }
    else  // XML attribute whose name != 'type': ERROR
    {
      return std::string("unknown attribute /") + attr->name() + "/";
    }
  }

  // Returning the value of the XML attribute 'type' (or the emnpty string, if not found)
  return type;
}
