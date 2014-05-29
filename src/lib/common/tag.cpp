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
#include <string>

#include "logMsg/logMsg.h"

#include "common/Format.h"
#include "common/tag.h"



/* ****************************************************************************
*
* startTag -  
*/
std::string startTag(const std::string& indent, const std::string& tagName, Format format, bool showTag)
{
  if (format == XML)
     return indent + "<" + tagName + ">\n";
  else if (format == JSON)
  {
    if (showTag == false)
      return indent + "{\n";
    else
      return indent + "\"" + tagName + "\" : {\n";
  }

  return "Format not supported";
}



/* ****************************************************************************
*
* startTag -  
*/
std::string startTag
(
  const std::string&  indent,
  const std::string&  xmlTag,
  const std::string&  jsonTag,
  Format              format,
  bool                isVector,
  bool                showTag,
  bool                isCompoundVector
)
{
  if (format == XML)
  {
    if (isCompoundVector)
      return indent + "<" + xmlTag + " type=\"vector\">\n";

    return indent + "<" + xmlTag + ">\n";
  }
  else if (format == JSON)
  {
    if (isVector && showTag)
       return indent + "\"" + jsonTag + "\" : [\n";
    else if (isVector && !showTag)
       return indent + "[\n";
    else if (!isVector && showTag)
       return indent + "\"" + jsonTag + "\" : {\n";
    else if (!isVector && !showTag)
       return indent + "{\n";
  }

  return "Format not supported";
}



/* ****************************************************************************
*
* endTag -  
*/
std::string endTag
(
  const std::string&  indent,
  const std::string&  tagName,
  Format              format,
  bool                comma,
  bool                isVector,
  bool                nl
)
{
  if (format == XML)
    return indent + "</" + tagName + ">\n";

  std::string out = indent;

  out += isVector?  "]"  : "}";
  out += comma?     ","  : "";
  out += nl?        "\n" : "";

  return out;
}



/* ****************************************************************************
*
* valueTag -  
*/
std::string valueTag
(
  const std::string&  indent,
  const std::string&  tagName,
  const std::string&  value,
  Format              format,
  bool                showComma,
  bool                isAssociation,
  bool                isVectorElement
)
{
  if (format == XML)
    return indent + "<" + tagName + ">" + value + "</" + tagName + ">" + "\n";

  if (showComma == true)
  {
    if (isAssociation == true)
      return indent + "\"" + tagName + "\" : " + value + ",\n";
    else if (isVectorElement == true)
      return indent + "\"" + value + "\",\n";
    else
      return indent + "\"" + tagName + "\" : \"" + value + "\",\n";
  }
  else
  {
    if (isAssociation == true)
      return indent + "\"" + tagName + "\" : " + value + "\n";
    else if (isVectorElement == true)
      return indent + "\"" + value + "\"\n";
    else
      return indent + "\"" + tagName + "\" : \"" + value + "\"\n";
  }
}


/* ****************************************************************************
*
* valueTag -  
*/
std::string valueTag
(
  const std::string&  indent,
  const std::string&  tagName,
  int                 value,
  Format              format,
  bool                showComma,
  bool                isAssociation
)
{
  char val[32];

  snprintf(val, sizeof(val), "%d", value);

  if (format == XML)
    return indent + "<" + tagName + ">" + val + "</" + tagName + ">" + "\n";

  if (showComma == true)
    return indent + "\"" + tagName + "\" : \"" + val + "\",\n";
  else
    return indent + "\"" + tagName + "\" : \"" + val + "\"\n";
}



/* ****************************************************************************
*
* valueTag -  
*/
std::string valueTag
(
  const std::string&  indent,
  const std::string&  xmlTag,
  const std::string&  jsonTag,
  const std::string&  value,
  Format              format,
  bool                showComma,
  bool                isAssociation
)
{
  if (format == XML)
    return indent + "<" + xmlTag + ">" + value + "</" + xmlTag + ">" + "\n";

  if (jsonTag == "")
  {
    if (showComma == true)
      return indent + "\"" + value + "\",\n";
    else
      return indent + "\"" + value + "\"\n";
  }
  else
  {
    if (showComma == true)
      return indent + "\"" + jsonTag + "\" : \"" + value + "\",\n";
    else
      return indent + "\"" + jsonTag + "\" : \"" + value + "\"\n";
  }
}
