#ifndef SRC_LIB_COMMON_TAG_H_
#define SRC_LIB_COMMON_TAG_H_

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
#include <string>

#include "common/Format.h"



/* ****************************************************************************
*
* startTag -  
*/
extern std::string startTag
(
  const std::string&  indent,
  const std::string&  tagName,
  Format              format,
  bool                showTag    = true,
  bool                isToplevel = false
);

extern std::string startTag
(
  const std::string&  indent,
  const std::string&  xmlTag,
  const std::string&  jsonTag,
  Format              format,
  bool                isVector         = false,
  bool                showTag          = true,
  bool                isCompoundVector = false
);



/* ****************************************************************************
*
* endTag -  
*/
extern std::string endTag
(
  const std::string&  indent,
  const std::string&  tagName,
  Format              format,
  bool                comma      = false,
  bool                isVector   = false,
  bool                nl         = true,
  bool                isToplevel = false
);



/* ****************************************************************************
*
* valueTag -  
*/
extern std::string valueTag
(
  const std::string&  indent,
  const std::string&  tagName,
  const std::string&  value,
  Format              format,
  bool                showComma       = false,
  bool                isAssociation   = false,
  bool                isVectorElement = false
);

extern std::string valueTag
(
  const std::string&  indent,
  const std::string&  tagName,
  int                 value,
  Format              format,
  bool                showComma     = false,
  bool                isAssociation = false
);

extern std::string valueTag
(
  const std::string&  indent,
  const std::string&  xmlTag,
  const std::string&  jsonTag,
  const std::string&  value,
  Format              format,
  bool                showComma     = false,
  bool                isAssociation = false
);



/* ****************************************************************************
*
* startArray -
*/
extern std::string startArray
(
  const std::string&  indent,
  const std::string&  tagName,
  Format              format,
  bool                showTag = true
);



/* ****************************************************************************
*
* endArray -
*/
extern std::string endArray(const std::string& indent, const std::string& tagName, Format format);

#endif  // SRC_LIB_COMMON_TAG_H_
