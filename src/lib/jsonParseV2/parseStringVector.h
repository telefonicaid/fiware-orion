#ifndef SRC_LIB_JSONPARSEV2_PARSESTRINGVECTOR_H_
#define SRC_LIB_JSONPARSEV2_PARSESTRINGVECTOR_H_

/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <vector>

#include "rapidjson/document.h"



/* ****************************************************************************
*
* parseStringVector - 
*/
extern bool parseStringVector
(
  std::vector<std::string>*  sVecP,
  const rapidjson::Value&    jsonVector,
  const std::string&         fieldName,
  bool                       emptyStringNotAllowed,
  bool                       unique,
  std::string*               errorStringP
);

#endif  // SRC_LIB_JSONPARSEV2_PARSESTRINGVECTOR_H_
