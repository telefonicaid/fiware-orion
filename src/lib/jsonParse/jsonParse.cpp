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
#include <stdint.h>

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

#include <set>
#include <string>
#include <exception>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi/Request.h"
#include "ngsi/ParseData.h"

#include "parse/compoundValue.h"
#include "parse/CompoundValueNode.h"
#include "parse/forbiddenChars.h"

#include "jsonParse/JsonNode.h"
#include "jsonParse/jsonParse.h"

using boost::property_tree::ptree;



/* ****************************************************************************
*
* compoundRootV -
*
*/
static const char* compoundRootV[] =
{
  "/contextElements/contextElement/attributes/attribute/value",
  "/attributes/attribute/value",
  "/contextResponses/contextResponse/contextElement/attributes/attribute/value",
  "/value"
};



/* ****************************************************************************
*
* isCompoundPath -
*
* This function examines a path to see whether we are inside a compound value or not.
* Also, it returns the root of the compound (found in 'compoundValueRootV') and also
* the 'rest' of the path, i.e. its relative path inside the compound.
*
* If the path doesn't belong to any compond, FALSE is returned.
*/
static bool isCompoundPath(const char* path)
{
  unsigned int len;

  for (unsigned int ix = 0; ix < sizeof(compoundRootV) / sizeof(compoundRootV[0]); ++ix)
  {
    len = strlen(compoundRootV[ix]);

    if (strlen(path) < len)
    {
      continue;
    }

    if (strncmp(compoundRootV[ix], path, len) == 0)
    {
      return true;
    }
  }

  return false;
}



/* ****************************************************************************
*
* treat -
*/
static bool treat
(
  ConnectionInfo*     ciP,
  const std::string&  path,
  const std::string&  value,
  JsonNode*           parseVector,
  ParseData*          parseDataP
)
{
  LM_T(LmtTreat, ("Treating path '%s', value '%s'", path.c_str(), value.c_str()));

  for (unsigned int ix = 0; parseVector[ix].path != "LAST"; ++ix)
  {
    //
    // Before treating a node, a check is made that the value of the node has no forbidden
    // characters.
    // However, if the the node has attributes, then the values of the attributes are checked instead
    //
    if (forbiddenChars(value.c_str()) == true)
    {
      LM_W(("Bad Input (found a forbidden value in '%s')", value.c_str()));
      ciP->httpStatusCode = SccBadRequest;
      ciP->answer = std::string("Illegal value for JSON field");
      return false;
    }

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
static std::string getArrayElementName(const std::string& arrayName)
{
  // Get the name of the array
  int pos = arrayName.find_last_of("/");
  std::string elementName = arrayName.substr(pos + 1);

  // Take out the last letter (if it is an array it will delete the 's' character
  // that is present in every case
  elementName = elementName.substr(0, elementName.length() - 1);

  // Is the singular form changing 'ie' for 'y' ??
  if (elementName.length() > 2 && elementName.substr(elementName.length() - 2).compare("ie") == 0)
  {
    elementName.replace(elementName.length() - 2, 2, "y");
  }

  return elementName;
}



/* ****************************************************************************
*
* nodeType -
*/
std::string nodeType(const std::string& nodeName, const std::string& value, orion::CompoundValueNode::Type* typeP)
{
  bool        isObject         = (nodeName == "") && (value == "");
  bool        isString           = (nodeName != "") && (value != "");
  bool        isVector         = (nodeName != "") && (value == "");

  if (isObject)
  {
    *typeP = orion::CompoundValueNode::Object;
    return "Object";
  }

  if (isString)
  {
    *typeP = orion::CompoundValueNode::String;
    return "String";
  }

  if (isVector)
  {
     *typeP = orion::CompoundValueNode::Vector;
     return "Vector";
  }

  // Simple String inside vector
  return "String";
}



/* ****************************************************************************
*
* eatCompound -
*/
void eatCompound
(
  ConnectionInfo*                           ciP,
  orion::CompoundValueNode*                 containerP,
  boost::property_tree::ptree::value_type&  v,
  const std::string&                        indent
)
{
  std::string                  nodeName     = v.first.data();
  std::string                  nodeValue    = v.second.data();
  boost::property_tree::ptree  subtree1     = (boost::property_tree::ptree) v.second;
  int                          noOfChildren = subtree1.size();

  if (containerP == NULL)
  {
    LM_T(LmtCompoundValue, ("COMPOUND: '%s'", nodeName.c_str()));
    containerP = new CompoundValueNode(orion::CompoundValueNode::Object);
    ciP->compoundValueRoot = containerP;
  }
  else
  {
    if ((nodeName != "") && (nodeValue != ""))  // Named String
    {
      if (forbiddenChars(nodeValue.c_str()) == true)
      {
        LM_W(("Bad Input (found a forbidden value in compound '%s')", nodeValue.c_str()));
        ciP->httpStatusCode = SccBadRequest;
        ciP->answer = std::string("Illegal value for JSON field");
        return;
      }

      containerP->add(orion::CompoundValueNode::String, nodeName, nodeValue);
      LM_T(LmtCompoundValue, ("Added string '%s' (value: '%s') under '%s'",
                              nodeName.c_str(),
                              nodeValue.c_str(),
                              containerP->cpath()));
    }
    else if ((nodeName == "") && (nodeValue == "") && (noOfChildren == 0))  // Unnamed String with EMPTY VALUE
    {
      LM_T(LmtCompoundValue, ("'Bad' input - looks like a container but it is an EMPTY STRING - no name, no value"));
      containerP->add(orion::CompoundValueNode::String, "item", "");
    }
    else if ((nodeName != "") && (nodeValue == ""))  // Named Container
    {
      LM_T(LmtCompoundValue, ("Adding container '%s' under '%s'", nodeName.c_str(), containerP->cpath()));
      containerP = containerP->add(orion::CompoundValueNode::Object, nodeName);
    }
    else if ((nodeName == "") && (nodeValue == ""))  // Name-Less container
    {
      LM_T(LmtCompoundValue, ("Adding name-less container under '%s' (parent may be a Vector!)", containerP->cpath()));
      containerP->type = orion::CompoundValueNode::Vector;
      containerP = containerP->add(orion::CompoundValueNode::Object, "item");
    }
    else if ((nodeName == "") && (nodeValue != ""))  // Name-Less String + its container is a vector
    {
      containerP->type = orion::CompoundValueNode::Vector;
      LM_T(LmtCompoundValue, ("Set '%s' to be a vector", containerP->cpath()));
      containerP->add(orion::CompoundValueNode::String, "item", nodeValue);
      LM_T(LmtCompoundValue, ("Added a name-less string (value: '%s') under '%s'",
                              nodeValue.c_str(), containerP->cpath()));
    }
    else
      LM_T(LmtCompoundValue, ("IMPOSSIBLE !!!"));
  }

  boost::property_tree::ptree subtree = (boost::property_tree::ptree) v.second;
  BOOST_FOREACH(boost::property_tree::ptree::value_type &v2, subtree)
  {
    eatCompound(ciP, containerP, v2, indent + "  ");
  }
}



/* ****************************************************************************
*
* jsonParse -
*/
static std::string jsonParse
(
  ConnectionInfo*                           ciP,
  boost::property_tree::ptree::value_type&  v,
  const std::string&                        _path,
  JsonNode*                                 parseVector,
  ParseData*                                parseDataP
)
{
  std::string                     nodeName         = v.first.data();
  std::string                     nodeValue        = v.second.data();
  std::string                     arrayElementName = getArrayElementName(_path);
  std::string                     path             = _path;
  bool                            treated;

  // If the node name is empty, boost will yield an empty name. This will happen only in the case of a vector.
  // See: http://www.boost.org/doc/libs/1_41_0/doc/html/boost_propertytree/parsers.html#boost_propertytree.parsers.json_parser
  if (nodeName != "")
  {
    // This detects whether we are trying to use an object within an object instead of an one-item array.
    // We don't allow the first case, hence the exception thrown.
    // However, this restriction is not valid inside Compound Values.
    if ((nodeName != arrayElementName) || (ciP->inCompoundValue == true))
    {
      path = path + "/" + nodeName;
    }
    else
    {
      throw std::logic_error("The object '" + path + "' may not have a child named '" + nodeName + "'");
    }
  }
  else
  {
    path = path + "/" + arrayElementName;
  }

  treated = treat(ciP, path, nodeValue, parseVector, parseDataP);


  boost::property_tree::ptree subtree = (boost::property_tree::ptree) v.second;
  int                         noOfChildren = subtree.size();
  if ((isCompoundPath(path.c_str()) == true) && (nodeValue == "") && (noOfChildren != 0))
  {
    std::string s;

    LM_T(LmtCompoundValue, ("Calling eatCompound for '%s'", path.c_str()));
    eatCompound(ciP, NULL, v, "");
    compoundValueEnd(ciP, parseDataP);

    if (ciP->httpStatusCode != SccOk)
    {
      return ciP->answer;
    }

    return "OK";
  }
  else if (treated == false)
  {
    ciP->httpStatusCode = SccBadRequest;
    if (ciP->answer == "")
    {
      ciP->answer = std::string("JSON Parse Error: unknown field: ") + path.c_str();
    }

    LM_W(("Bad Input (%s)", ciP->answer.c_str()));
    return ciP->answer;
  }

  if (noOfChildren == 0)
  {
    return "OK";
  }

  subtree = (boost::property_tree::ptree) v.second;
  BOOST_FOREACH(boost::property_tree::ptree::value_type &v2, subtree)
  {
    std::string out = jsonParse(ciP, v2, path, parseVector, parseDataP);
    if (out != "OK")
    {
      LM_W(("Bad Input (JSON parse error: '%s')", out.c_str()));
      return out;
    }
  }

  return "OK";
}

/* ****************************************************************************
*
* jsonParse -
*/
std::string jsonParse
(
  ConnectionInfo*     ciP,
  const char*         content,
  const std::string&  requestType,
  JsonNode*           parseVector,
  ParseData*          parseDataP
)
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
      LM_W(("Bad Input (JSON Parse error: '%s')", res.c_str()));
      return res;
    }
  }

  return "OK";
}
