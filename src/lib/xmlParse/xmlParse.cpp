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
* compoundValueRootV - 
*
* The vector 'compoundValueRootV' contains a list of all the paths where we allow
* a compound value
*/
static const char* compoundValueRootV[] =
{
  "/updateContextRequest/contextElementList/contextElement/contextAttributeList/contextAttribute/contextValue"
};



/* ****************************************************************************
*
* isCompoundValuePath - 
*
* This function examines a path to see whether we are inside a compouind value or not.
* Also, it returned the root of the compound (found in 'compoundValueRootV') and also
* the 'rest' of the path, i.e. its relative path inside the compound.
*
* If the path doesn't belong to any compond, FALSE is returned.
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
* xmlTypeFromString - translate ASCII string to orion::CompoundValueNode::Type
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
*
* This function is called once (actually it is not that simple) for each node in
* the tree that the XML parser has built for us.
* 
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
  // I have NEVER seen this happen, but I leave it here, just in case ...
  // [ If it happens and we don't return here, the broker dies with a SIGSEGV. We don't want that ]
  if ((node == NULL) || (node->name() == NULL))
    return;

  //
  // The tree of the XML library we have chosen is a little bit strange.
  // This here is probably the end of a container,
  // The end of a container is interesting only when inside a Compound
  //
  if ((node->name()[0] == 0) && (ciP->compoundValueP == NULL))
    return;

  //
  // Here we have 6 possibilities:
  //   1. Compound Start
  //      - isCompound           == true
  //      - ciP->inCompoundValue == false
  //   We enter here when an XML tag has been found e.g. under a 'contextValue'
  //
  //   2. In the middle of a Compound
  //      - isCompound           == true
  //      - ciP->inCompoundValue == true
  //      Here we will enter as until we get out of e.g. 'contextValue' 
  //
  //     2.1 Start of a container, or a leaf
  //        - node name            != ""
  //
  //     2.2. Compound container ends
  //        - path.len == fatherPath.len + 1
  //
  //   3. Compound ends
  //      - ciP->inCompoundValue == true
  //      - isCompound           == false
  //    We enter here if the function isCompoundValuePath says the path is NOT part of a compound
  //    but, the program logic says we're in one. If so, time to end it.
  //
  //   4. Normal treated path
  //      - isCompound           == false
  //      - ciP->inCompoundValue == false
  //      - treated              == true
  //   This is the normal case and is taken care of by the 'treat' function
  //
  //   5. Untreated path that should give a parse error
  //      - isCompound           == false
  //      - ciP->inCompoundValue == false
  //      - treated              == false
  //   The path is not a compound, nor a path to be treated.
  //   Only one choice left, the path is unknown and should give a parse error
  //

  std::string  root;
  std::string  rest;
  std::string  path          = fatherPath + "/" + node->name();
  std::string  name          = node->name();
  std::string  value         = node->value();
  bool         isCompound    = isCompoundValuePath(fatherPath.c_str(), root, rest);
  bool         treated       = treat(node, path, parseVector, parseDataP);
  std::string  value2        = node->value();
  char*        trimmedValue  = wsStrip((char*) value2.c_str());

  LM_T(LmtCompoundValue, ("--------------------------------------"));
  LM_T(LmtCompoundValue, ("fatherPath: '%s'", fatherPath.c_str()));
  LM_T(LmtCompoundValue, ("path:       '%s'", path.c_str()));
  LM_T(LmtCompoundValue, ("root:       '%s'", root.c_str()));
  LM_T(LmtCompoundValue, ("rest:       '%s'", rest.c_str()));

  LM_T(LmtCompoundValueRaw, ("%s: value: '%s'", path.c_str(), trimmedValue));

  if ((isCompound == true) && (ciP->inCompoundValue == false))
  {
    // We are in a compound, but we weren't before.
    // Thus, a new compound is started,
    // I have selected to detect the entrance of a compound when its first XML node appears.
    // I mean, /1/2/3/contextValue could contain a compound value but it could also NOT.
    // We don't enter here (to create the tree) until we encounter a node with a path 'inside'
    // the compound, e.g.:
    //
    //   /1/2/3/contextValue/firstNodeInCompound
    //
    // So, the function 'compoundValueStart' must both create the root node of the tree AND
    // take care of its first habitant (firstNodeInCompound), which is does by preparing a 
    // call to compoundValueMiddle (See lib/parse/compoundValue.cpp)
    //
    // Both here and in the next 'else if' (the one 'inside' a compound), the node must be checked
    // for the XML attribute 'vector'
    //
    // This is duplicating code, but I don't want to put it inside 'compoundValueStart' because I
    // intend the JSON parser to use 'compoundValueStart' also.
    // It would be more code-duplication if I put the XML Attribute stuff inside 'compoundValueStart'
    //
    std::string                     xmlAttribute = xmlTypeAttributeGet(node);
    orion::CompoundValueNode::Type  type         = xmlTypeFromString(xmlAttribute, name, value);

    // The XML attribute 'type' must be either 'vector' or nothing at all
    if ((xmlAttribute != "vector") && (xmlAttribute != ""))
    {
      ciP->httpStatusCode = SccBadRequest;
      ciP->answer = std::string("Bad value for XML attribute 'type' for '") + node->name() + "': '" + xmlAttribute + "'";
      LM_W(("ERROR: '%s', PATH: '%s'   ", ciP->answer.c_str(), fatherPath.c_str()));
      return;
    }

    orion::compoundValueStart(ciP, path, name, value, root, rest, type, xmlAttribute == "vector");
  }
  else if ((isCompound == true) && (ciP->inCompoundValue == true))
  {
    //
    // The path is a compound and we were already in a compound
    //
    // First, check whether the node has the XML attribute 'vector', or any other attribute
    // that wouldresult in an error.
    //
    // Then, check what we really got. Did we get the and of a container, or did we get a new node?
    // The decision details are commented below.
    // If it was a new node, 'compoundValueMiddle' is called to take care of it.
    //
    std::string                     xmlAttribute  = xmlTypeAttributeGet(node);
    orion::CompoundValueNode::Type  type          = xmlTypeFromString(xmlAttribute, name, value);

    LM_T(LmtCompoundValueStep, ("Step up?  New path:     '%s'", rest.c_str()));
    LM_T(LmtCompoundValueStep, ("Step up?  Old path:     '%s'", ciP->compoundValueP->path.c_str()));
    LM_T(LmtCompoundValueStep, ("Step up?  trimmedValue: '%s'", trimmedValue));
    LM_T(LmtCompoundValueStep, ("Step in?  Name:         '%s'", name.c_str()));
    
    // The XML attribute 'type' must be either 'vector' or nothing at all
    if ((xmlAttribute != "vector") && (xmlAttribute != ""))
    {
      ciP->httpStatusCode = SccBadRequest;
      ciP->answer = std::string("Bad value for XML attribute 'type' for '") + node->name() + "': '" + xmlAttribute + "'";
      LM_W(("ERROR: '%s', PATH: '%s'   ", ciP->answer.c_str(), fatherPath.c_str()));
      return;
    }
    else if ((rest.size() < ciP->compoundValueP->path.size()) && (trimmedValue[0] == 0) && (fatherPath.size() + 1 == path.size()))
    {
      //
      // A container ended - we have to go up one level to the container of the container that ended.
      // Only way I can detect this is to compare the length of the path of this node with the path of current container.
      // But it is more complicated than that.
      // This function is called every time a node ends.
      // Only way I have found to detect a unique end of a container is by 'fatherPath.size() + 1 == path.size()' + 'trimmedValue[0] == 0'
      // apart from the 'logical' rest.size() < ciP->compoundValueP->path.size().
      // This is pretty complex, but, it doesn't work any other way ...
     //
      ciP->compoundValueP = ciP->compoundValueP->container;
      LM_T(LmtCompoundValueContainer, ("Set current container to '%s' (%s)  [after stepping up]", ciP->compoundValueP->path.c_str(), ciP->compoundValueP->name.c_str()));
    }
    else if (name != "")
    {
      // If the name is non-empty, and it is NOT the end of a container, then a new Leaf or container has started
      orion::compoundValueMiddle(ciP, rest, name, value, type);
    }
  }
  else if ((isCompound == false) && (ciP->inCompoundValue == true))
  {
    //
    // We were is a compound (ciP->inCompoundValue == true) but we no longer are (isCompound == false)
    // Conclusion? The compound value has ended.
    // In 'compoundValueEnd', the compound value is checked for errors and in case all is OK,
    // The XML node that is the owner of the compound (e.g. a 'contextValue') is given the root pointer of the tree.
    // The current ContextAttribute, whose 'contextValue' is the owner of this compound value tree is set in the
    // parsing routines.
    // 
    // In case of error ("duplicated item in struct", "not same item name in vector"), ciP->httpStatusCode and ciP->answer
    // are set to reflect the error, that will be taken care of by the caller of this function
    //
    orion::compoundValueEnd(ciP, path, parseDataP);
  }
  else if ((isCompound == false) && (ciP->inCompoundValue == false) && (treated == false))
  {
    //
    // PARSE ERROR
    //
    // This incoming node has a path that the broker is unaware of.
    // It cannot be treated.
    // Only choice is to return a parse error and a good description of the error
    //
    ciP->httpStatusCode = SccBadRequest;
    if (ciP->answer == "")
      ciP->answer = std::string("Unknown XML field: '") + node->name() + "'";
    LM_W(("ERROR: '%s', PATH: '%s'   ", ciP->answer.c_str(), fatherPath.c_str()));
    return;
  }
  else if ((isCompound == false) && (ciP->inCompoundValue == false) && (treated == true))
  {
    // This is just a trace message.
    // The actual treatment of the niode was made before, in the call to the 'treat' function.
    //
    LM_T(LmtCompoundValue, ("OK:              '%s'", path.c_str()));
  }


  //
  // Recursive calls for all children of this node
  //
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
  LM_T(LmtNullNode, ("Not treating node '%s'", node->name()));
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
      LM_RE("unsupported attribute for EntityId", ("Warning: unsupported attribute '%s' for EntityId", attr->name()));
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
* Vectors don't exist in XML, all containers are considered 'structs', but as
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
* - type="struct"
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
       if (type != "") // set already? 
         return "more than one 'type' attribute";

       type = attr->value(); // save the value of the attribute 'type'
    }
    else  // XML attribute whose name != 'type': ERROR
      return std::string("unknown attribute '") + attr->name() + "'";
  }

  // Returning the value of the XML attribute 'type' (or the emnpty string, if not found)
  return type;
}
