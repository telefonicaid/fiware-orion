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
#include <set>
#include <string>
#include <stdint.h>
#include <exception>

//
// http://www.boost.org/doc/libs/1_31_0/libs/spirit/doc/grammar.html:
//
//  if a grammar is intended to be used in multithreaded code, we should then define
//  BOOST_SPIRIT_THREADSAFE before including any spirit header files.
//  In this case it will also be required to link against Boost.Threads
//
#define BOOST_SPIRIT_THREADSAFE

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi/Request.h"
#include "ngsi/ParseData.h"

#include "parse/compoundValue.h"
#include "parse/CompoundValueNode.h"

#include "jsonParse/JsonNode.h"
#include "jsonParse/jsonParse.h"

using boost::property_tree::ptree;



/* ****************************************************************************
*
* compoundValueRootV - 
*
* The vector 'compoundValueRootV' contains a list of all the paths where we allow
* a compound value
*/
static const char* compoundValueRootV[] =
{
  "/contextElements/contextElement/attributes/attribute/value/"
};



/* ****************************************************************************
*
* isCompoundValuePath - 
*
* This function examines a path to see whether we are inside a compound value or not.
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
*/
static bool treat(ConnectionInfo* ciP, std::string path, std::string value, JsonNode* parseVector, ParseData* parseDataP)
{
  LM_T(LmtTreat, ("Treating path '%s', value '%s'", path.c_str(), value.c_str()));

  for (unsigned int ix = 0; parseVector[ix].path != "LAST"; ++ix)
  {
    if (path == parseVector[ix].path)
    {
      LM_T(LmtTreat, ("calling treat function for '%s': '%s'", path.c_str(), value.c_str()));
      std::string res = parseVector[ix].treat(path, value, parseDataP);
      LM_T(LmtTreat, ("called treat function for '%s'. result: '%s'", path.c_str(), res.c_str()));
      return true;
    }
  }

  return false;
}



/* ****************************************************************************
*
* getArrayElementName -
*/
static std::string getArrayElementName(std::string arrayName)
{
  // Get the name of the array
  int pos = arrayName.find_last_of("/");
  std::string elementName = arrayName.substr(pos + 1);

  // Take out the last letter (if it is an array it will delete the 's' character
  // that is present in every case
  elementName = elementName.substr(0, elementName.length() - 1);

  // Is the singular is formed changing 'ie' for 'y' ??
  if(elementName.length() > 2 && elementName.substr(elementName.length() - 2).compare("ie") == 0)
    elementName.replace(elementName.length() - 2, 2, "y");

  return elementName;
}



/* ****************************************************************************
*
* nodeType - 
*/
std::string nodeType(std::string nodeName, std::string value, orion::CompoundValueNode::Type* typeP)
{
  bool        isStruct         = (nodeName == "") && (value == "");
  bool        isLeaf           = (nodeName != "") && (value != "");
  bool        isVector         = (nodeName != "") && (value == "");

  if (isStruct)
  {
    *typeP = orion::CompoundValueNode::Struct;
    return "Struct";
  }

  if (isLeaf)
  {
    *typeP = orion::CompoundValueNode::Leaf;
    return "Leaf";
  }
  
  if (isVector)
  {
     *typeP = orion::CompoundValueNode::Vector;
     return "Vector";
  }

  // Simple Leaf inside vector
  return "Leaf";
}



/* ****************************************************************************
*
* dirDepth - 
*/
static int dirDepth(const char* s)
{
  int slashes = 0;

  while (*s != 0)
  {
    if (*s == '/')
      ++slashes;
    ++s;
  }

  return slashes;
}



/* ****************************************************************************
*
* jsonParse - 
*/
static std::string jsonParse
(
   ConnectionInfo*                           ciP,
   boost::property_tree::ptree::value_type&  v,
   std::string                               path,
   JsonNode*                                 parseVector,
   ParseData*                                parseDataP
)
{
  std::string                     nodeName         = v.first.data();
  std::string                     nodeValue        = v.second.data();
  std::string                     arrayElementName = getArrayElementName(path);
  bool                            treated;

  // If the node name is empty, boost will yield an empty name. This will happen only in the case of a vector.
  // See: http://www.boost.org/doc/libs/1_41_0/doc/html/boost_propertytree/parsers.html#boost_propertytree.parsers.json_parser
  if (nodeName != "")
  {
    // This detects whether we are trying to use an object within an object instead of a one-item array.
    // We don't allow the first case, hence the exception thrown.
    // However, this restriction is not valid inside Compound Values.
     if ((nodeName != arrayElementName) || (ciP->inCompoundValue == true))
      path = path + "/" + nodeName;
    else
      throw std::logic_error("The object '" + path + "' may not have a child named '" + nodeName + "'");
  }
  else
    path = path + "/" + arrayElementName;

  treated = treat(ciP, path, nodeValue, parseVector, parseDataP);


  //
  // Here (just like in xmlParse) we have six possibilities:
  // 1. Compound Start
  // 2. In the middle of a Compound
  //    2.1 Start of a container, or a leaf
  //    2.2. Compound container ends
  // 4. Compound ends
  // 5. Normal treated path
  // 6. Untreated path that should give a parse error
  //
  std::string  root;
  std::string  rest;  
  bool         isCompound = isCompoundValuePath(path.c_str(), root, rest);
  
  if (treated == false)
  {
    orion::CompoundValueNode::Type  type;
    bool                            fatherIsVector = false;
    bool                            stepUp         = false;
    std::string                     comment;
    
    LM_T(LmtCompoundValue, ("nodeName: '%s', nodeValue: '%s'", nodeName.c_str(), nodeValue.c_str()));

    if ((nodeName != "") && (nodeValue != ""))
    {
      comment = "I am a LEAF";
      type = orion::CompoundValueNode::Leaf;
    }
    else if ((nodeName != "") && (nodeValue == ""))
    {
      comment = "I am a STRUCT or VECTOR";
      type = orion::CompoundValueNode::Struct;
    }
    else if ((nodeName == "") && (nodeValue == ""))
    {
      comment = "current container is a VECTOR, not a struct";
      fatherIsVector = true;
      type = orion::CompoundValueNode::Struct;
    }
    else if ((nodeName == "") && (nodeValue != ""))
    {
      //
      // We enter here for simple elements in a vector. E.g.:
      //
      // "value": [
      //   "1",
      //   "2",
      //   "3"
      // ]
      //
      // The element "1" is the first time we enter 'compound', so ciP->compoundValueP
      // doesn't exist yet.
      // Adding "1" to the vector "value" must be done later (after creating the 
      // root container - in compoundValueStart).
      //
      // In the case of "2" and "3", these simple leaves are just added and we return.
      //
      if (ciP->compoundValueP != NULL)
      {
        ciP->compoundValueP->add(orion::CompoundValueNode::Leaf, "item", nodeValue);
        ciP->compoundValueP->type = orion::CompoundValueNode::Vector;
        return "OK";
      }
      else
      {
        nodeName       = "item";
        type           = orion::CompoundValueNode::Leaf;
        fatherIsVector = true;
      }
    }

    if (ciP->compoundValueP != NULL)
    {
      int currentDepth = dirDepth(ciP->compoundValueP->path.c_str());
      int thisDepth    = dirDepth(rest.c_str()) + 1;  // rest has the initial '/' stripped off

      if (thisDepth <= currentDepth)
      {
        comment += ", STEPPING UP";
        stepUp = true;
        LM_T(LmtCompoundValueStep, ("STEPPING UP from '%s' to '%s' (from '%s' to '%s')", ciP->compoundValueP->cname(), ciP->compoundValueP->container->cname(), ciP->compoundValueP->cpath(), ciP->compoundValueP->container->cpath()));
      }
    }

    LM_T(LmtCompoundValue, ("%-70s (name: '%s', value: '%s') - %s", path.c_str(), nodeName.c_str(), nodeValue.c_str(), comment.c_str()));

    if  ((isCompound == true) && (ciP->inCompoundValue == false))
    {
      orion::compoundValueStart(ciP, path, nodeName, nodeValue, root, rest, type, fatherIsVector);
      if ((nodeName == "") && (nodeValue == ""))
      {
        ciP->compoundValueP->name = "item";
        ciP->compoundValueP->container = ciP->compoundValueP->rootP;
      }
    }
    else if ((isCompound == true) && (ciP->inCompoundValue == true))
    {
      //
      // This is either a NEW Object/Vector/Leaf, or the current container has ended.
      // If the path of the current object is shorter than the path of the current container,
      // then we know that we've stepped out of the current container and we have to point
      // to its father as being the new 'current container'
      //
      if (stepUp)
      {
        LM_T(LmtCompoundValueStep, ("Old container: '%s' (path: '%s')", ciP->compoundValueP->cname(), ciP->compoundValueP->cpath()));

        ciP->compoundValueP = ciP->compoundValueP->container;

        if (ciP->compoundValueP->container->isVector())
        {
          LM_T(LmtCompoundValueStep, ("Father of the new container is a vector - I have to step up one step more"));
          ciP->compoundValueP = ciP->compoundValueP->container;
        }
        LM_T(LmtCompoundValueStep, ("New container: '%s' (path: '%s')", ciP->compoundValueP->cname(), ciP->compoundValueP->cpath()));
      }
      else
      {
        if (fatherIsVector)
          ciP->compoundValueP->type = orion::CompoundValueNode::Vector;
      }

      if (nodeName != "")
        orion::compoundValueMiddle(ciP, rest, nodeName, nodeValue, type);
      else
      {
        // This is a vector item, its name must be 'item'
        orion::compoundValueMiddle(ciP, rest, "item", "", orion::CompoundValueNode::Struct);
      }
    }
    else if ((isCompound == false) && (ciP->inCompoundValue == false))
    {
      ciP->httpStatusCode = SccBadRequest;
      if (ciP->answer == "")
        ciP->answer = std::string("JSON Parse Error (unknown field: '") + path.c_str() + "')";
      LM_E(("ERROR: '%s'", ciP->answer.c_str()));
      return ciP->answer;
    }
  }
  else if ((isCompound == false) && (ciP->inCompoundValue == true))
  {
    orion::compoundValueEnd(ciP, path, parseDataP);
    if (ciP->httpStatusCode != SccOk)
      return ciP->answer;
  }

  boost::property_tree::ptree subtree = (boost::property_tree::ptree) v.second;
  BOOST_FOREACH(boost::property_tree::ptree::value_type &v2, subtree)
  {
    std::string out = jsonParse(ciP, v2, path, parseVector, parseDataP);
    if (out != "OK")
      LM_RE(out, ("JSON Parse Error: '%s'", out.c_str()));
  }

  return "OK";
}

/* ****************************************************************************
*
* jsonParse - 
*/
std::string jsonParse(ConnectionInfo* ciP, const char* content, std::string requestType, JsonNode* parseVector, ParseData* parseDataP)
{
  std::stringstream  ss;
  ptree              tree;
  ptree              subtree;
  std::string        path;

  ss << content;
  read_json(ss, subtree);

  tree.put_child(requestType, subtree);

  // LM_T(LmtParse, ("parsing '%s'", content));
  BOOST_FOREACH(boost::property_tree::ptree::value_type &v, tree.get_child(requestType))
  {
    std::string res = jsonParse(ciP, v, path, parseVector, parseDataP);
    if (res != "OK")
    {
      LM_W(("Parse error: '%s'", res.c_str()));
      return res;
    }
  }

  return "OK";
}
