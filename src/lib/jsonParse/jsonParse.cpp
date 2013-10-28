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
std::string treat(int type, std::string path, std::string value, JsonNode* parseVector, ParseData* reqDataP)
{
  LM_T(LmtTreat, ("Treating path '%s', value '%s'", path.c_str(), value.c_str()));

  for (unsigned int ix = 0; parseVector[ix].path != "LAST"; ++ix)
  {
    if (path == parseVector[ix].path)
    {
      LM_T(LmtTreat, ("calling treat %d function for '%s': '%s'", type, path.c_str(), value.c_str()));

      std::string res = parseVector[ix].treat(path, value, reqDataP);
      return res;
    }
  }

  LM_T(LmtTreat, ("The path '%s' is not treated", path.c_str()));
  return "OK";
}

/* ****************************************************************************
*
* getArrayElementName -
*/
static std::string getArrayElementName(std::string arrayName)
{
  int pos = arrayName.find_last_of("/");
  std::string elementName = arrayName.substr(pos + 1);
  elementName = elementName.substr(0, elementName.length()-1);

  if(elementName.substr(elementName.length()-2).compare("ie") == 0)
    elementName.replace(elementName.length()-2, 2, "y");

  return elementName;
}

/* ****************************************************************************
*
* jsonParse - 
*/
static std::string jsonParse
(
   boost::property_tree::ptree::value_type&  v,
   std::string                               path,
   JsonNode*                                 parseVector,
   ParseData*                                reqDataP
)
{
  std::string  nodeName  = v.first.data();
  std::string  value     = v.second.data();
  std::string  res       = "OK";

  // If the node name is empty, boost will yield an empty name.
  // See: http://www.boost.org/doc/libs/1_41_0/doc/html/boost_propertytree/parsers.html#boost_propertytree.parsers.json_parser
  if (nodeName != "")
    path = path + "/" + nodeName;
  else
    path = path + "/" + getArrayElementName(path);

  if (value == "")
    res = treat(1, path, value, parseVector, reqDataP);
  else
    res = treat(2, path, value, parseVector, reqDataP);

  boost::property_tree::ptree subtree = (boost::property_tree::ptree) v.second;
  BOOST_FOREACH(boost::property_tree::ptree::value_type &v2, subtree)
  {
     jsonParse(v2, path, parseVector, reqDataP);
  }

  return res; // "OK"
}

/* ****************************************************************************
*
* jsonParse - 
*/
std::string jsonParse(const char* content, std::string requestType, JsonNode* parseVector, ParseData* reqDataP)
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
    jsonParse(v, path, parseVector, reqDataP);
  }

  return "OK";
}
