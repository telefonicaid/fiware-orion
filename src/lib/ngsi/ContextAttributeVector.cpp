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
#include "common/JsonHelper.h"
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
*
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
  JsonObjectHelper jh;

  std::map<std::string, std::map<std::string, int> >::iterator it;
  unsigned int                                                 ix;
  for (it = perAttrTypes.begin(), ix = 0; it != perAttrTypes.end(); ++it, ++ix)
  {
    std::string                 attrName  = it->first;
    std::map<std::string, int>  attrTypes = it->second;

    JsonVectorHelper jvh;

    std::map<std::string, int>::iterator jt;
    unsigned int                         jx;

    JsonObjectHelper jhTypes;

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
        jvh.addString(type);
      }

    }

    jhTypes.addRaw("types", jvh.str());

    jh.addRaw(attrName, jhTypes.str());
  }

  return jh.str();
}



/* ****************************************************************************
*
* ContextAttributeVector::toJsonV1 -
*
* FIXME P5: this method doesn't depend on the class object. Should be moved out of the class?
* Maybe included in the Entiy class render logic.
*/
std::string ContextAttributeVector::toJsonV1
(  
  bool                                   asJsonObject,
  RequestType                            request,
  const std::vector<ContextAttribute*>&  orderedAttrs,
  const std::vector<std::string>&        metadataFilter,
  bool                                   comma,
  bool                                   omitValue,
  bool                                   attrsAsName
)
{
  std::string out = "";

  if (orderedAttrs.size() == 0)
  {
    return "";
  }

  if (asJsonObject)
  {
    // If the URI parameter 'attributeFormat' is set to 'object', then the attribute vector
    // is to be rendered as objects for JSON, and not as a vector.
    // Also, if we have more than one attribute with the same name (possible if different metaID),
    // only one of them should be included in the vector. Any one of them.
    // The added vector helps to keep track and avoid duplicates
    std::vector<std::string> added;

    // Note that in the case of attribute as name, we have to use a vector, thus using
    // attrsAsName variable as value for isVector parameter
    out += startTag("attributes", attrsAsName);
    for (unsigned int ix = 0; ix < orderedAttrs.size(); ++ix)
    {
      if (addedLookup(added, orderedAttrs[ix]->name) == "")
      {
        added.push_back(orderedAttrs[ix]->name);
        if (attrsAsName)
        {
          out += orderedAttrs[ix]->toJsonV1AsNameString(true);
        }
        else
        {
          out += orderedAttrs[ix]->toJsonV1(asJsonObject, request, metadataFilter, true, omitValue);
        }
      }
      else
      {
        LM_T(LmtJsonAttributes, ("Attribute already added attribute '%s'", orderedAttrs[ix]->name.c_str()));
      }
    }

    // Remove final comma, as the addedLookup() check doesn't allow us to predict which the
    // last invocation to orderedAttrs[ix]->toJsonV1(...) and setting the comma paramter properly
    out = out.substr(0, out.length() - 1 );

    out += endTag(comma, attrsAsName);
  }
  else
  {
    out += startTag("attributes", true);
    for (unsigned int ix = 0; ix < orderedAttrs.size(); ++ix)
    {
      if (attrsAsName)
      {
        out += orderedAttrs[ix]->toJsonV1AsNameString(ix != orderedAttrs.size() - 1);
      }
      else
      {
        out += orderedAttrs[ix]->toJsonV1(asJsonObject, request, metadataFilter, ix != orderedAttrs.size() - 1, omitValue);
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
* ContextAttributeVector::push_back -
*
*/
void ContextAttributeVector::push_back(ContextAttribute* item)
{
  vec.push_back(item);
}



/* ****************************************************************************
*
* ContextAttributeVector::push_back - 
*
*/
void ContextAttributeVector::push_back(const ContextAttributeVector& caV, bool cloneCompound)
{
  for (unsigned int ix = 0; ix < caV.size(); ++ix)
  {
    vec.push_back(new ContextAttribute(caV[ix], false, cloneCompound));
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
*
*/
void ContextAttributeVector::fill(const ContextAttributeVector& caV, bool useDefaultType, bool cloneCompounds)
{
  if (caV.size() == 0)
  {
    return;
  }

  for (unsigned int ix = 0; ix < caV.size(); ++ix)
  {
    ContextAttribute* from = caV[ix];
    ContextAttribute* caP = new ContextAttribute(from, useDefaultType, cloneCompounds);

    push_back(caP);
  }
}



/* ****************************************************************************
*
* get -
*/
int ContextAttributeVector::get(const std::string& attributeName) const
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    if (vec[ix]->name == attributeName)
    {
      return ix;
    }
  }

  return -1;
}


/* ****************************************************************************
*
* getAll -
*
* Like get, but takes into account there could be several attributes with same name
* and differente ID and returns all them (in a vector of integers)
*
* FIXME #3168: to be removed along with all the metadata ID related code
*
*/
void ContextAttributeVector::getAll(const std::string& attributeName, std::vector<int>* foundP) const
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    if (vec[ix]->name == attributeName)
    {
      foundP->push_back(ix);
    }
  }
}
