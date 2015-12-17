#ifndef SRC_LIB_XMLPARSE_XMLNODE_H_
#define SRC_LIB_XMLPARSE_XMLNODE_H_

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
#include <string>

#include "boost/property_tree/detail/rapidxml.hpp"
#include "ngsi/ParseData.h"



/* ****************************************************************************
*
* namespace problem avoiding definitions - 
*/
#ifdef LONG_RAPIDXML_NAME_SPACE
// boost 1.45 and beyond style
using namespace boost::property_tree::detail::rapidxml;
#else
// boost 1.44 and before style
using namespace rapidxml;
#endif



/* ****************************************************************************
*
* XmlNodeTreat - 
*/
typedef int (*XmlNodeTreat)(xml_node<>* node, ParseData* reqData);



/* ****************************************************************************
*
* XmlNode - 
*/
typedef struct XmlNode
{
  std::string   path;
  XmlNodeTreat  treat;
} XmlNode;

#endif  // SRC_LIB_XMLPARSE_XMLNODE_H_
