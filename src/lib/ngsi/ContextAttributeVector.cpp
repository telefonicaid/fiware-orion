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
#include <map>
#include <string>
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/tag.h"
#include "common/string.h"
#include "common/RenderFormat.h"
#include "ngsi/ContextAttributeVector.h"
#include "ngsi/Request.h"



/* ****************************************************************************
*
* ContextAttributeVector::ContextAttributeVector - 
*/
ContextAttributeVector::ContextAttributeVector()
{
  vec.clear();
}



/* ****************************************************************************
*
* addedLookup - 
*/
static std::string addedLookup(const std::vector<std::string>& added, std::string value)
{
  for (unsigned int ix = 0; ix < added.size(); ++ix)
  {
    if (added[ix] == value)
    {
      return value;
    }
  }

  return "";
}



/* ****************************************************************************
*
* ContextAttributeVector::toJsonTypes -
*/
std::string ContextAttributeVector::toJsonTypes(void)
{
  // Pass 1 - get per-attribute types
  std::map<std::string, std::map<std::string, int> > perAttrTypes;

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    ContextAttribute* caP = vec[ix];
    perAttrTypes[caP->name][caP->type] = 1;   // just to mark that type exists
  }

  // Pass 2 - generate JSON
  std::string out;

  std::map<std::string, std::map<std::string, int> >::iterator it;
  unsigned int                                                 ix;
  for (it = perAttrTypes.begin(), ix = 0; it != perAttrTypes.end(); ++it, ++ix)
  {
    std::string                 attrName  = it->first;
    std::map<std::string, int>  attrTypes = it->second;

    out += JSON_STR(attrName) + ":{" + JSON_STR("types") + ":[";

    std::map<std::string, int>::iterator jt;
    unsigned int                         jx;

    for (jt = attrTypes.begin(), jx = 0; jt != attrTypes.end(); ++jt, ++jx)
    {
      std::string type = jt->first;
      
      //
      // Special condition for 'options=noAttrDetail':
      //   When the 'options' URI parameter contains 'noAttrDetail',
      //   mongoBackend fills the attribute type vector with *just one item* (that is an empty string).
      //   This special condition is checked for here, to produce a [] for the vector for the response.
      //
      // See the origin of this in mongoQueryTypes.cpp. Look for "NOTE: here we add", in two locations.
      //
      if ((type != "") || (attrTypes.size() != 1))
      {
        out += JSON_STR(type);
      }

      if (jx != attrTypes.size() - 1)
      {
        out += ",";
      }
    }

    out += "]}";

    if (ix != perAttrTypes.size() - 1)
    {
      out += ",";
    }
  }

  return out;
}



/* ****************************************************************************
*
* ContextAttributeVector::toJson - 
*
* Attributes named 'id' or 'type' are not rendered in API version 2, due to the 
* compact way in which API v2 is rendered. Attributes named 'id' or 'type' would simply
* collide with the 'id' and 'type' of the entity itself (holder of the attribute).
*
* If anybody needs an attribute named 'id' or 'type', then API v1
* will have to be used to retrieve that information.
*/
std::string ContextAttributeVector::toJson
(
  RenderFormat                     renderFormat,
  const std::vector<std::string>&  attrsFilter,
  const std::vector<std::string>&  metadataFilter,
  bool                             blacklist
) const
{
  if (vec.size() == 0)
  {
    return "";
  }


  //
  // Pass 1 - count the total number of attributes valid for rendering.
  //
  // Attributes named 'id' or 'type' are not rendered.
  // This gives us a small problem in the logic here, about knowing whether the
  // comma should be rendered or not.
  //
  // To fix this problem we need to do two passes over the vector, the first pass to
  // count the number of valid attributes and the second to do the work.
  // In the second pass, if the number of rendered attributes "so far" is less than the total
  // number of valid attributes, then the comma must be rendered.
  //
  int validAttributes = 0;
  std::map<std::string, bool>  uniqueMap;
  if ((attrsFilter.size() == 0) || (std::find(attrsFilter.begin(), attrsFilter.end(), ALL_ATTRS) != attrsFilter.end()))
  {
    for (unsigned int ix = 0; ix < vec.size(); ++ix)
    {
      if ((vec[ix]->name == "id") || (vec[ix]->name == "type"))
      {
        continue;
      }

      if ((renderFormat == NGSI_V2_UNIQUE_VALUES) && (vec[ix]->valueType == orion::ValueTypeString))
      {
        if (uniqueMap[vec[ix]->stringValue] == true)
        {
          continue;
        }
      }

      ++validAttributes;

      if ((renderFormat == NGSI_V2_UNIQUE_VALUES) && (vec[ix]->valueType == orion::ValueTypeString))
      {
        uniqueMap[vec[ix]->stringValue] = true;
      }
    }
  }
  else if (!blacklist)
  {
    for (std::vector<std::string>::const_iterator it = attrsFilter.begin(); it != attrsFilter.end(); ++it)
    {
      if (lookup(*it) != NULL)
      {
        ++validAttributes;
      }
    }
  }
  else // attrsFilter is black list
  {
    for (unsigned ix = 0; ix < vec.size(); ++ix)
    {
      if (std::find(attrsFilter.begin(), attrsFilter.end(), vec[ix]->name) == attrsFilter.end())
      {
         ++validAttributes;
      }
    }
  }

  //
  // Pass 2 - do the work, helped by the value of 'validAttributes'.
  //
  std::string  out;
  int          renderedAttributes = 0;

  uniqueMap.clear();

  if (attrsFilter.size() == 0 || (std::find(attrsFilter.begin(), attrsFilter.end(), ALL_ATTRS) != attrsFilter.end()))
  {
    for (unsigned int ix = 0; ix < vec.size(); ++ix)
    {
      if ((vec[ix]->name == "id") || (vec[ix]->name == "type"))
      {
        continue;
      }

      ++renderedAttributes;

      if ((renderFormat == NGSI_V2_UNIQUE_VALUES) && (vec[ix]->valueType == orion::ValueTypeString))
      {
        if (uniqueMap[vec[ix]->stringValue] == true)
        {
          continue;
        }
      }

      out += vec[ix]->toJson(renderedAttributes == validAttributes, renderFormat, metadataFilter);

      if ((renderFormat == NGSI_V2_UNIQUE_VALUES) && (vec[ix]->valueType == orion::ValueTypeString))
      {
        uniqueMap[vec[ix]->stringValue] = true;
      }
    }
  }
  else if (!blacklist)
  {
    for (std::vector<std::string>::const_iterator it = attrsFilter.begin(); it != attrsFilter.end(); ++it)
    {
      ContextAttribute* caP = lookup(*it);
      if (caP != NULL)
      {
        ++renderedAttributes;
        out += caP->toJson(renderedAttributes == validAttributes, renderFormat, metadataFilter);
      }
    }
  }
  else // attrsFilter is black list
  {
    for (unsigned ix = 0; ix < vec.size(); ++ix)
    {
      if (std::find(attrsFilter.begin(), attrsFilter.end(), vec[ix]->name) == attrsFilter.end())
      {
        ++renderedAttributes;
        out += vec[ix]->toJson(renderedAttributes == validAttributes, renderFormat, metadataFilter);
      }
    }
  }

  return out;
}



/* ****************************************************************************
*
* ContextAttributeVector::render - 
*/
std::string ContextAttributeVector::render
(
  ApiVersion   apiVersion,
  bool         asJsonObject,
  RequestType  request,
  bool         comma,
  bool         omitValue,
  bool         attrsAsName
)
{
  std::string out = "";

  if (vec.size() == 0)
  {
    return "";
  }

  //
  // NOTE:
  // If the URI parameter 'attributeFormat' is set to 'object', then the attribute vector
  // is to be rendered as objects for JSON, and not as a vector.
  // Also, if we have more than one attribute with the same name (possible if different metaID),
  // only one of them should be included in the vector. Any one of them.
  // So, step 1 is to purge the context attribute vector from 'copies'.
  //
  // FIXME PR: fix step 1
  //
  if (asJsonObject)
  {
    std::vector<std::string> added;

    // 1. Remove attributes with attribute names already used.
    for (unsigned int ix = 0; ix < vec.size(); ++ix)
    {
      if (addedLookup(added, vec[ix]->name) == "")
      {
        added.push_back(vec[ix]->name);
        LM_T(LmtJsonAttributes, ("Keeping attribute '%s'", vec[ix]->name.c_str()));
      }
      else
      {
        LM_T(LmtJsonAttributes, ("Removing attribute '%s'", vec[ix]->name.c_str()));
        vec[ix]->release();
        delete vec[ix];
        vec.erase(vec.begin() + ix);
      }
    }

    // 2. Now it's time to render
    // Note that in the case of attribute as name, we have to use a vector, thus using
    // attrsAsName variable as value for isVector parameter
    out += startTag("attributes", attrsAsName);
    for (unsigned int ix = 0; ix < vec.size(); ++ix)
    {
      if (attrsAsName)
      {
        out += vec[ix]->renderAsNameString(ix != vec.size() - 1);
      }
      else
      {
        out += vec[ix]->render(apiVersion, asJsonObject, request, ix != vec.size() - 1, omitValue);
      }
    }
    out += endTag(comma, attrsAsName);
  }
  else
  {
    out += startTag("attributes", true);
    for (unsigned int ix = 0; ix < vec.size(); ++ix)
    {
      if (attrsAsName)
      {
        out += vec[ix]->renderAsNameString(ix != vec.size() - 1);
      }
      else
      {
        out += vec[ix]->render(apiVersion, asJsonObject, request, ix != vec.size() - 1, omitValue);
      }
    }
    out += endTag(comma, true);
  }

  return out;
}



/* ****************************************************************************
*
* ContextAttributeVector::check - 
*/
std::string ContextAttributeVector::check(ApiVersion apiVersion, RequestType requestType)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    std::string res;

    if ((res = vec[ix]->check(apiVersion, requestType)) != "OK")
      return res;
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextAttributeVector::present - 
*/
void ContextAttributeVector::present(const std::string& indent)
{
  LM_T(LmtPresent, ("%s%lu ContextAttributes", 
		    indent.c_str(), 
		    (uint64_t) vec.size()));

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    vec[ix]->present(indent + "  ", ix);
  }
}



/* ****************************************************************************
*
* ContextAttributeVector::push_back - 
*/
void ContextAttributeVector::push_back(ContextAttribute* item)
{
  vec.push_back(item);
}



/* ****************************************************************************
*
* ContextAttributeVector::push_back - 
*/
void ContextAttributeVector::push_back(ContextAttributeVector* aVec)
{
  for (unsigned int ix = 0; ix < aVec->size(); ++ix)
  {
    vec.push_back(new ContextAttribute((*aVec)[ix]));
  }
}



/* ****************************************************************************
*
* ContextAttributeVector::size - 
*/
unsigned int ContextAttributeVector::size(void) const
{
  return vec.size();
}


/* ****************************************************************************
*
* ContextAttributeVector::operator[] -
*/
ContextAttribute*  ContextAttributeVector::operator[](unsigned int ix) const
{
  if (ix < vec.size())
  {
    return vec[ix];
  }
  return NULL;
}



/* ****************************************************************************
*
* ContextAttributeVector::release -
*/
void ContextAttributeVector::release(void)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    vec[ix]->release();
    delete vec[ix];
  }

  vec.clear();
}



/* ****************************************************************************
*
* ContextAttributeVector::fill - 
*/
void ContextAttributeVector::fill(ContextAttributeVector* cavP, bool useDefaultType)
{
  if (cavP == NULL)
  {
    return;
  }

  for (unsigned int ix = 0; ix < cavP->size(); ++ix)
  {
    ContextAttribute* from = (*cavP)[ix];
    ContextAttribute* caP = new ContextAttribute(from, useDefaultType);

    push_back(caP);
  }
}



/* ****************************************************************************
*
* lookup -
*/
ContextAttribute* ContextAttributeVector::lookup(const std::string& attributeName) const
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    if (vec[ix]->name == attributeName)
    {
      return vec[ix];
    }
  }

  return NULL;
}
