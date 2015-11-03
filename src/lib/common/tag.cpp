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
#include <stdio.h>
#include <string>

#include "logMsg/logMsg.h"
#include "common/Format.h"
#include "common/tag.h"



/* ****************************************************************************
*
* htmlEscape - 
*
* Allocate a new buffer to hold an escaped version of the input buffer 's'.
* Escaping characters demands more space in the buffer, for some characters up to six
* characters - double-quote (") needs SIX chars: &quot;
* So, when allocating room for the output (escaped) buffer, we need to consider the worst case
* and six times the length of the input buffer is allocated (plus one byte for the zero-termination.
*
* See http://www.anglesanddangles.com/asciichart.php for more info on the 'html-escpaing' of ASCII chars.
*/
char* htmlEscape(const char* s)
{
  int   newLen  = strlen(s) * 6 + 1;  // See function header comment
  char* out     = (char*) calloc(1, newLen);
  int   sIx     = 0;
  int   outIx   = 0;
  
  if (out == NULL)
  {
    LM_E(("Internal Error (allocating %d bytes: %s)", newLen, strerror(errno)));
    return NULL;
  }

  while (s[sIx] != 0)
  {
    switch (s[sIx])
    {
    case '<':
      out[outIx++] = '&';
      out[outIx++] = 'l';
      out[outIx++] = 't';
      out[outIx++] = ';';
      ++sIx;
      break;

    case '>':
      out[outIx++] = '&';
      out[outIx++] = 'g';
      out[outIx++] = 't';
      out[outIx++] = ';';
      ++sIx;
      break;

    case '(':
      out[outIx++] = '&';
      out[outIx++] = '#';
      out[outIx++] = '4';
      out[outIx++] = '0';
      out[outIx++] = ';';
      ++sIx;
      break;

    case ')':
      out[outIx++] = '&';
      out[outIx++] = '#';
      out[outIx++] = '4';
      out[outIx++] = '1';
      out[outIx++] = ';';
      ++sIx;
      break;

    case '=':
      out[outIx++] = '&';
      out[outIx++] = '#';
      out[outIx++] = '6';
      out[outIx++] = '1';
      out[outIx++] = ';';
      ++sIx;
      break;

    case '\'':
      out[outIx++] = '&';
      out[outIx++] = '#';
      out[outIx++] = '3';
      out[outIx++] = '9';
      out[outIx++] = ';';
      ++sIx;
      break;

    case '"':
      out[outIx++] = '&';
      out[outIx++] = 'q';
      out[outIx++] = 'u';
      out[outIx++] = 'o';
      out[outIx++] = 't';
      out[outIx++] = ';';
      ++sIx;
      break;

    case ';':
      out[outIx++] = '&';
      out[outIx++] = '#';
      out[outIx++] = '5';
      out[outIx++] = '9';
      out[outIx++] = ';';
      ++sIx;
      break;

    default:
      out[outIx++] = s[sIx++];
    }
  }

  return out;
}



/* ****************************************************************************
*
* startTag -  
*/
std::string startTag
(
  const std::string&  indent,
  const std::string&  tagName,
  Format              format,
  bool                showTag,
  bool                isToplevel
)
{
  if (format == XML)
  {
    return indent + "<" + tagName + ">\n";
  }
  else if (format == JSON)
  {
    if (isToplevel)
    {
      if (showTag == false)
      {
        return indent + "{\n" + indent + "  {\n";
      }
      else
      {
        return indent + "{\n" + indent + "  " + "\"" + tagName + "\" : {\n";
      }
    }
    else
    {
      if (showTag == false)
      {
        return indent + "{\n";
      }
      else
      {
        return indent + "\"" + tagName + "\" : {\n";
      }
    }
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
    {
      return indent + "<" + xmlTag + " type=\"vector\">\n";
    }

    return indent + "<" + xmlTag + ">\n";
  }
  else if (format == JSON)
  {
    if (isVector && showTag)
    {
       return indent + "\"" + jsonTag + "\" : [\n";
    }
    else if (isVector && !showTag)
    {
      return indent + "[\n";
    }
    else if (!isVector && showTag)
    {
      return indent + "\"" + jsonTag + "\" : {\n";
    }
    else if (!isVector && !showTag)
    {
      return indent + "{\n";
    }
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
  bool                nl,
  bool                isToplevel
)
{
  if (format == XML)
  {
    return indent + "</" + tagName + ">\n";
  }

  if (isToplevel)
  {
    return indent + "}\n}\n";
  }

  std::string out = indent;

  out += isVector?    "]"  : "}";
  out += comma?       ","  : "";
  out += nl?          "\n" : "";
  out += isToplevel?  "}"  : "";

  return out;
}



/* ****************************************************************************
*
* valueTag -  
*
*/
std::string valueTag
(
  const std::string&  indent,
  const std::string&  tagName,
  const std::string&  unescapedValue,
  Format              format,
  bool                showComma,
  bool                isVectorElement,
  bool                valueIsNumberOrBool
)
{

  char* value;

  if (unescapedValue == "")
  {
    value = (char*) malloc(1);

    *value = 0;
  }
  else
  {
    value = htmlEscape(unescapedValue.c_str());
  }

  if (value == NULL)
  {
    return "ERROR: no memory";
  }

  if (format == XML)
  {
    std::string out = indent + "<" + tagName + ">" + value + "</" + tagName + ">" + "\n";

    free(value);
    return out;
  }

  std::string effectiveValue = valueIsNumberOrBool ? value : std::string("\"") + value + "\"";
  free(value);

  if (showComma == true)
  {
    if (isVectorElement == true)
    {
      std::string out = indent + effectiveValue + ",\n";
      return out;
    }
    else
    {
      std::string out = indent + "\"" + tagName + "\" : " + effectiveValue + ",\n";
      return out;
    }
  }
  else
  {
    if (isVectorElement == true)
    {
      std::string out = indent + effectiveValue + "\n";
      return out;
    }
    else
    {
      std::string out = indent + "\"" + tagName + "\" : " + effectiveValue + "\n";
      return out;
    }
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
  bool                showComma
)
{
  char val[32];

  snprintf(val, sizeof(val), "%d", value);

  if (format == XML)
  {
    return indent + "<" + tagName + ">" + val + "</" + tagName + ">" + "\n";
  }

  if (showComma == true)
  {
    return indent + "\"" + tagName + "\" : \"" + val + "\",\n";
  }

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
  bool                valueIsNumberOrBool
)
{
  if (format == XML)
  {
    return indent + "<" + xmlTag + ">" + value + "</" + xmlTag + ">" + "\n";
  }

  std::string eValue = valueIsNumberOrBool? value : JSON_STR(value);

  if (jsonTag == "")
  {
    if (showComma == true)
    {
      return indent + eValue + ",\n";
    }
    else
    {
      return indent + eValue + "\n";
    }
  }

  if (showComma == true)
  {
    return indent + "\"" + jsonTag + "\" : " + eValue + ",\n";
  }

  return indent + "\"" + jsonTag + "\" : " + eValue + "\n";
}
