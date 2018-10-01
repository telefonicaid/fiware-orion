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
#include <sstream>

#include "logMsg/logMsg.h"
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
* jsonInvalidCharsTransformation -
*
* FIXME P5: this is a quick fix for #1172. A better fix should de developed.
*
* Pretty much based in JsonHelper::toJsonString(). In fact, most of the code is
* duplicated
*
*/
std::string jsonInvalidCharsTransformation(const std::string& input)
{
  std::string ss;

  for (std::string::const_iterator iter = input.begin(); iter != input.end(); ++iter)
  {
    /* FIXME P3: This function ensures that if the DB holds special characters (which are
     * not supported in JSON according to its specification), they are converted to their escaped
     * representations. The process wouldn't be necessary if the DB couldn't hold such special characters,
     * but as long as we support NGSIv1, it is better to have the check (e.g. a newline could be
     * used in an attribute value using XML). Even removing NGSIv1, we have to ensure that the
     * input parser (rapidjson) doesn't inject not supported JSON characters in the DB (this needs to be
     * investigated in the rapidjson documentation)
     *
     * JSON specification is a bit obscure about the need of escaping / (what they call 'solidus'). The
     * picture at JSON specification (http://www.json.org/) seems suggesting so, but after a careful reading of
     * https://tools.ietf.org/html/rfc4627#section-2.5, we can conclude it is not mandatory. Online checkers
     * such as http://jsonlint.com confirm this. Looking in some online discussions
     * (http://andowebsit.es/blog/noteslog.com/post/the-solidus-issue/ and
     * https://groups.google.com/forum/#!topic/opensocial-and-gadgets-spec/FkLsC-2blbo) it seems that
     * escaping / may have sense in some situations related with JavaScript code, which is not the case of Orion.
     *
     */
    switch (char ch = *iter)
    {
    case '\\': ss += "\\\\"; break;
    case '"':  ss += "\\\""; break;
    case '\b': ss += "\\b";  break;
    case '\f': ss += "\\f";  break;
    case '\n': ss += "\\n";  break;
    case '\r': ss += "\\r";  break;
    case '\t': ss += "\\t";  break;

    default:
      /* Converting the rest of special chars 0-31 to \u00xx. Note that 0x80 - 0xFF are untouched as they
       * correspond to UTF-8 multi-byte characters */
      if (ch >= 0 && ch <= 0x1F)
      {
        static const char intToHex[16] =  { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' } ;

        ss += "\\u00" + intToHex[(ch & 0xF0) >> 4] + intToHex[ch & 0x0F];
      }
      else
      {
        ss += ch;
      }
      break;
    }  // end-switch

  }  // end-for

  return ss;
}



/* ****************************************************************************
*
* startTag -
*/
std::string startTag
(
  const std::string&  key,
  bool                isVector
)
{
  // Empty key is legal JSON. However, Orion doesn't use that kind of keys,
  // so we can use an empty key string as argument instead of a showkey boolean
  // parameter, keeping the function signature simpler
  bool showKey = (key != "");

  if (isVector && showKey)
  {
    return "\"" + key + "\":[";
  }
  else if (isVector && !showKey)
  {
    return "[";
  }
  else if (!isVector && showKey)
  {
    return "\"" + key + "\":{";
  }

  // else: !isVector && !showKey

  return "{";
}



/* ****************************************************************************
*
* endTag -  
*/
std::string endTag
(
  bool                comma,
  bool                isVector
)
{
  std::string out = "";

  out += isVector?  "]"  : "}";
  out += comma?     ","  : "";

  return out;
}



/* ****************************************************************************
*
* valueTag -  
*
* Function version for string values
*
*/
std::string valueTag
(
  const std::string&  key,
  const std::string&  unescapedValue,
  bool                showComma,
  bool                isVectorElement,
  bool                withoutQuotes
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

  std::string effectiveValue = jsonInvalidCharsTransformation(value);
  free(value);

  effectiveValue = withoutQuotes ? effectiveValue : std::string("\"") + effectiveValue + "\"";

  if (showComma == true)
  {
    if (isVectorElement == true)
    {
      std::string out = effectiveValue + ",";
      return out;
    }
    else
    {
      std::string out = "\"" + key + "\":" + effectiveValue + ",";
      return out;
    }
  }
  else
  {
    if (isVectorElement == true)
    {
      std::string out = effectiveValue;
      return out;
    }
    else
    {
      std::string out = "\"" + key + "\":" + effectiveValue;
      return out;
    }
  }
}



/* ****************************************************************************
*
* valueTag -
*
* Function version for integer values
*
*/
std::string valueTag
(
  const std::string&  key,
  int                 value,
  bool                showComma
)
{
  char val[32];

  snprintf(val, sizeof(val), "%d", value);

  return valueTag(key, val, showComma, false, false);
}


