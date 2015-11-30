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
#include "ngsi/ContextAttributeVector.h"
#include "ngsi/Request.h"
#include "rest/ConnectionInfo.h"
#include "rest/uriParamNames.h"



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
* ContextAttributeVector::toJson - 
*
* Attributes named 'id' or 'type' are not rendered in API version 2, due to the 
* compact way in which API v2 is rendered. Attributes named 'id' or 'type' would simply
* collide with the 'id' and 'type' of the entity itself (holder of the attribute).
*
* If anybody needs an attribute named 'id' or 'type', then API v1
* will have to be used to retrieve that information.
*/
std::string ContextAttributeVector::toJson(bool isLastElement, bool types)
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
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    if ((vec[ix]->name == "id") || (vec[ix]->name == "type"))
    {
      continue;
    }

    ++validAttributes;
  }


  //
  // Pass 2 - do the work, helped by the value of 'validAttributes'.
  //
  std::string  out;
  int          renderedAttributes = 0;
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    if ((vec[ix]->name == "id") || (vec[ix]->name == "type"))
    {
      continue;
    }

    ++renderedAttributes;
    out += vec[ix]->toJson(renderedAttributes == validAttributes, types);
  }

  return out;
}



/* ****************************************************************************
*
* ContextAttributeVector::render - 
*/
std::string ContextAttributeVector::render
(
  ConnectionInfo*     ciP,
  RequestType         request,
  const std::string&  indent,
  bool                comma,
  bool                omitValue,
  bool                attrsAsName
)
{
  std::string out      = "";
  std::string xmlTag   = "contextAttributeList";
  std::string jsonTag  = "attributes";

  if (vec.size() == 0)
  {
    if (ciP->outFormat == XML)
    {
      if (((request == IndividualContextEntityAttribute)    ||
           (request == AttributeValueInstance)              ||
           (request == IndividualContextEntityAttributes)))
      {
        return indent + "<contextAttributeList></contextAttributeList>\n";
      }
    }

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
  if ((ciP->uriParam[URI_PARAM_ATTRIBUTE_FORMAT] == "object") && (ciP->outFormat == JSON))
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
    out += startTag(indent, xmlTag, jsonTag, ciP->outFormat, attrsAsName, true);
    for (unsigned int ix = 0; ix < vec.size(); ++ix)
    {
      if (attrsAsName)
      {
        out += vec[ix]->renderAsNameString(ciP, request, indent + "  ", ix != vec.size() - 1);
      }
      else
      {
        out += vec[ix]->render(ciP, request, indent + "  ", ix != vec.size() - 1, omitValue);
      }
    }
    out += endTag(indent, xmlTag, ciP->outFormat, comma, attrsAsName);
  }
  else
  {
    out += startTag(indent, xmlTag, jsonTag, ciP->outFormat, true, true);
    for (unsigned int ix = 0; ix < vec.size(); ++ix)
    {
      if (attrsAsName)
      {
        out += vec[ix]->renderAsNameString(ciP, request, indent + "  ", ix != vec.size() - 1);
      }
      else
      {
        out += vec[ix]->render(ciP, request, indent + "  ", ix != vec.size() - 1, omitValue);
      }
    }
    out += endTag(indent, xmlTag, ciP->outFormat, comma, true);
  }

  return out;
}



/* ****************************************************************************
*
* ContextAttributeVector::check - 
*/
std::string ContextAttributeVector::check
(
  RequestType         requestType,
  Format              format,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    std::string res;

    if ((res = vec[ix]->check(requestType, format, indent, predetectedError, 0)) != "OK")
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
  LM_F(("%s%lu ContextAttributes", indent.c_str(), (uint64_t) vec.size()));

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
* ContextAttributeVector::get - 
*/
ContextAttribute* ContextAttributeVector::get(unsigned int ix)
{
  if (ix < vec.size())
    return vec[ix];
  return NULL;
}

const ContextAttribute* ContextAttributeVector::get(unsigned int ix) const
{
  if (ix < vec.size())
    return vec[ix];
  return NULL;
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
void ContextAttributeVector::fill(ContextAttributeVector* cavP)
{
  if (cavP == NULL)
    return;

  for (unsigned int ix = 0; ix < cavP->size(); ++ix)
  {
    ContextAttribute* from = cavP->get(ix);
    ContextAttribute* caP = new ContextAttribute(from);

    push_back(caP);
  }
}
