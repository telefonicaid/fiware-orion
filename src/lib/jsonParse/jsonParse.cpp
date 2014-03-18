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

#include "JsonNode.h"
#include "jsonParse.h"

using boost::property_tree::ptree;



/* ****************************************************************************
*
* treat - 
*/
std::string treat(ConnectionInfo* ciP, int type, std::string path, std::string value, JsonNode* parseVector, ParseData* reqDataP)
{
  LM_T(LmtTreat, ("Treating path '%s', value '%s'", path.c_str(), value.c_str()));

  for (unsigned int ix = 0; parseVector[ix].path != "LAST"; ++ix)
  {
    if (path == parseVector[ix].path)
    {
      LM_T(LmtTreat, ("calling treat %d function for '%s': '%s'", type, path.c_str(), value.c_str()));
      std::string res = parseVector[ix].treat(path, value, reqDataP);
      LM_T(LmtTreat, ("called treat %d function for '%s'. result: '%s'", type, path.c_str(), res.c_str()));
      return res;
    }
  }

  ciP->httpStatusCode = SccBadRequest;
  if (ciP->answer == "")
    ciP->answer = std::string("JSON Parse Error (unknown field: '") + path.c_str() + "')";
  LM_W(("ERROR: '%s'", ciP->answer.c_str()));

  return ciP->answer;
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
* jsonParse - 
*/
static std::string jsonParse
(
   ConnectionInfo*                           ciP,
   boost::property_tree::ptree::value_type&  v,
   std::string                               path,
   JsonNode*                                 parseVector,
   ParseData*                                reqDataP
)
{
  std::string nodeName         = v.first.data();
  std::string value            = v.second.data();
  std::string res              = "OK";
  std::string arrayElementName = getArrayElementName(path);

  // If the node name is empty, boost will yield an empty name. This will happen only in the case of a vector.
  // See: http://www.boost.org/doc/libs/1_41_0/doc/html/boost_propertytree/parsers.html#boost_propertytree.parsers.json_parser
  if (nodeName != "")
  {
    // This detects whether we are trying to use an object within an object instead of a one-item array.
    // We don't allow the first case, hence the exception thrown.
    if (nodeName != arrayElementName)
      path = path + "/" + nodeName;
    else
      throw std::logic_error("The object '" + path + "' may not have a child named '" + nodeName + "'");
  }
  else
    path = path + "/" + arrayElementName;

  if (value == "")
    res = treat(ciP, 1, path, value, parseVector, reqDataP);
  else
    res = treat(ciP, 2, path, value, parseVector, reqDataP);

  if (res != "OK")
  {
    LM_E(("treat function returned error"));
    return res;
  }

  boost::property_tree::ptree subtree = (boost::property_tree::ptree) v.second;
  BOOST_FOREACH(boost::property_tree::ptree::value_type &v2, subtree)
  {
     jsonParse(ciP, v2, path, parseVector, reqDataP);
  }

  return res; // "OK"
}

/* ****************************************************************************
*
* jsonParse - 
*/
std::string jsonParse(ConnectionInfo* ciP, const char* content, std::string requestType, JsonNode* parseVector, ParseData* reqDataP)
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
    std::string res = jsonParse(ciP, v, path, parseVector, reqDataP);
    if (res != "OK")
      return res;
  }

  return "OK";
}
