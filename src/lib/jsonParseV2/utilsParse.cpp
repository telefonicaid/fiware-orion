/*
*
* Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Orion dev team
*/
#include <string>

#include "rapidjson/document.h"

#include "jsonParseV2/utilsParse.h"



/* ****************************************************************************
*
* getBoolAux -
*/
static Opt<bool> getBoolAux
(
  const rapidjson::Value&  parent,
  const char*              field,
  const std::string&       description,
  bool                     optional
)
{
  if (parent.HasMember(field))
  {
    const rapidjson::Value& value = parent[field];

    if (!value.IsBool())
    {
      return Opt<bool>((!description.empty() ? description : field) + " is not a bool");
    }

    return Opt<bool>(value.GetBool(), true);
  }
  else if (!optional)
  {
     return Opt<bool>((!description.empty() ? description : field) + " is missing");
  }

  return Opt<bool>("", false);
}



/* ****************************************************************************
*
* getBoolMust - get a mandatory bool from the rapidjson node
*/
extern Opt<bool> getBoolMust(const rapidjson::Value& parent, const char* field, const std::string& description)
{
  return getBoolAux(parent, field, description, false);
}



/* ****************************************************************************
*
* getBoolOpt - get an optional bool from the rapidjson node
*/
extern Opt<bool> getBoolOpt(const rapidjson::Value& parent, const char* field, const std::string& description)
{
  return getBoolAux(parent, field, description, true);
}



/* ****************************************************************************
*
* getStringAux - 
*/
static Opt<std::string> getStringAux
(
  const rapidjson::Value&  parent,
  const char*              field,
  const std::string&       description,
  bool optional
)
{
  if (parent.HasMember(field))
  {
    const rapidjson::Value& value = parent[field];

    if (!value.IsString())
    {
      return Opt<std::string>((!description.empty() ? description : field) + " is not a string");
    }

    return Opt<std::string>(value.GetString(), true);
  }
  else if (!optional)
  {
     return Opt<std::string>((!description.empty() ? description : field) + " is missing");
  }

  return Opt<std::string>("", false);
}



/* ****************************************************************************
*
* getStringMust - get a mandatory string from the rapidjson node
*/
extern Opt<std::string> getStringMust(const rapidjson::Value& parent, const char* field, const std::string& description)
{
  return getStringAux(parent, field, description, false);
}



/* ****************************************************************************
*
* getStringOpt - get an optional string from the rapidjson node
*/
extern Opt<std::string> getStringOpt(const rapidjson::Value& parent, const char* field, const std::string& description)
{
  return getStringAux(parent, field, description, true);
}



/* ****************************************************************************
*
* getInt64Aux - 
*/
static Opt<int64_t> getInt64Aux
(
  const rapidjson::Value&  parent,
  const char*              field,
  const std::string&       description,
  bool                     optional
)
{
  if (parent.HasMember(field))
  {
    const rapidjson::Value& value = parent[field];

    if (!value.IsInt64())
    {
      return Opt<int64_t>((!description.empty() ? description : field) + " is not an int");
    }

    return Opt<int64_t>(value.GetInt64(), true);
  }
  else if (!optional)
  {
    return Opt<int64_t>((!description.empty() ? description : field) + " is missing");
  }

  return Opt<int64_t>(0, false);
}



/* ****************************************************************************
*
* getInt64Must - get a mandatory int64_t from the rapidjson node
*/
extern Opt<int64_t> getInt64Must(const rapidjson::Value& parent, const char* field, const std::string& description)
{
  return getInt64Aux(parent, field, description, false);
}



/* ****************************************************************************
*
* getInt64Opt - get an optional int64_t from the rapidjson node
*/
extern Opt<int64_t> getInt64Opt(const rapidjson::Value& parent, const char* field, const std::string& description)
{
  return getInt64Aux(parent, field, description, true);
}



/* *****************************************************************************
*
* isNull - check if a given rapidjson node is null
*/
extern bool isNull(const rapidjson::Value& parent, const char* field)
{
  if (parent.HasMember(field))
  {
    const rapidjson::Value& value = parent[field];

    return value.IsNull();
  }

  return false;
}



/* *****************************************************************************
*
* parseErrorString - return text string associated to a given parsing error
*
* Based in the mapping at https://rapidjson.org/group___r_a_p_i_d_j_s_o_n___e_r_r_o_r_s.html#ga7d3acf640886b1f2552dc8c4cd6dea60
*/
std::string parseErrorString(rapidjson::ParseErrorCode code)
{
  switch (code)
  {
  case rapidjson::kParseErrorNone:                          return "No error";
  case rapidjson::kParseErrorDocumentEmpty:                 return "The document is empty";
  case rapidjson::kParseErrorDocumentRootNotSingular:       return "The document root must not follow by other values";
  case rapidjson::kParseErrorValueInvalid:                  return "Invalid value";
  case rapidjson::kParseErrorObjectMissName:                return "Missing a name for object member";
  case rapidjson::kParseErrorObjectMissColon:               return "Missing a colon after a name of object member";
  case rapidjson::kParseErrorObjectMissCommaOrCurlyBracket: return "Missing a comma or '}' after an object member";
  case rapidjson::kParseErrorArrayMissCommaOrSquareBracket: return "Missing a comma or ']' after an array element";
  case rapidjson::kParseErrorStringUnicodeEscapeInvalidHex: return "Incorrect hex digit after \\u escape in string";
  case rapidjson::kParseErrorStringUnicodeSurrogateInvalid: return "The surrogate pair in string is invalid";
  case rapidjson::kParseErrorStringEscapeInvalid:           return "Invalid escape character in string";
  case rapidjson::kParseErrorStringMissQuotationMark:       return "Missing a closing quotation mark in string";
  case rapidjson::kParseErrorStringInvalidEncoding:         return "Invalid encoding in string";
  case rapidjson::kParseErrorNumberTooBig:                  return "Number too big to be stored in double";
  case rapidjson::kParseErrorNumberMissFraction:            return "Miss fraction part in number";
  case rapidjson::kParseErrorNumberMissExponent:            return "Miss exponent in number";
  case rapidjson::kParseErrorTermination:                   return "Parsing was terminated";
  case rapidjson::kParseErrorUnspecificSyntaxError:         return "Unspecific syntax error";
  default:                                                  return "Unknown parse error";
  }
}
