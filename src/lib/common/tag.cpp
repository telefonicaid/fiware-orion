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
std::string startTag(std::string indent, std::string tagName, Format format, bool showTag)
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
std::string startTag(std::string indent, std::string xmlTag, std::string jsonTag, Format format, bool isVector, bool showTag)
{
  if (format == XML)
    return indent + "<" + xmlTag + ">\n";
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
std::string endTag(std::string indent, std::string tagName, Format format, bool comma, bool isVector)
{
  if (format == XML)
    return indent + "</" + tagName + ">\n";

  if (isVector && comma)
     return indent + "],\n";
  else if (isVector && !comma)
     return indent + "]\n";
  else if (!isVector && comma)
    return indent + "},\n";
  else
    return indent + "}\n";
}



/* ****************************************************************************
*
* valueTag -  
*/
std::string valueTag(std::string indent, std::string tagName, std::string value, Format format, bool showComma, bool isAssociation)
{
  if (format == XML)
    return indent + "<" + tagName + ">" + value + "</" + tagName + ">" + "\n";

  if (showComma == true)
  {
    if (isAssociation == true)
      return indent + "\"" + tagName + "\" : " + value + ",\n";
    else
      return indent + "\"" + tagName + "\" : \"" + value + "\",\n";
  }
  else
  {
    if (isAssociation == true)
      return indent + "\"" + tagName + "\" : " + value + "\n";
    else
      return indent + "\"" + tagName + "\" : \"" + value + "\"\n";
  }
}


/* ****************************************************************************
*
* valueTag -  
*/
std::string valueTag(std::string indent, std::string tagName, int value, Format format, bool showComma, bool isAssociation)
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
