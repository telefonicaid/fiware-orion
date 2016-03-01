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
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/MetadataVector.h"



/* ****************************************************************************
*
* MetadataVector::MetadataVector -
*/
MetadataVector::MetadataVector(const std::string& _tag)
{
  vec.clear();
  tagSet(_tag);
}



/* ****************************************************************************
*
* MetadataVector::tagSet -
*/
void MetadataVector::tagSet(const std::string& tagName)
{
  tag = tagName;
}



/* ****************************************************************************
*
* MetadataVector::render -
*/
std::string MetadataVector::render(Format format, const std::string& indent, bool comma)
{
  std::string out     = "";
  std::string jsonTag = "metadatas";

  if (vec.size() == 0)
  {
    return "";
  }

  //out += startTag(indent, tag, jsonTag, format, true);
  out += startTag2(indent, jsonTag, true);
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    out += vec[ix]->render(format, indent + "  ", ix != vec.size() - 1);
  }
  out += endTag(indent, comma, true);


  return out;
}



/* ****************************************************************************
*
* MetadataVector::toJson -
*
* Metadatas named 'value' or 'type' are not rendered in API version 2, due to the 
* compact way in which API v2 is rendered. Metadatas named 'value' or 'type' would simply
* collide with the 'value' and 'type' of the attribute itself (holder of the metadata).
*
* If anybody needs a metadata named 'value' or 'type', then API v1
* will have to be used to retreive that information.
*/
std::string MetadataVector::toJson(bool isLastElement)
{
  if (vec.size() == 0)
  {
    return "";
  }


  //
  // Pass 1 - count the total number of metadatas valid for rendering.
  //
  // Metadatas named 'value' or 'type' are not rendered.
  // This gives us a small problem in the logic here, about knowing whether the
  // comma should be rendered or not.
  //
  // To fix this problem we need to do two passes over the vector, the first pass to
  // count the number of valid metadatas and the second to do the work.
  // In the second pass, if the number of rendered metadatas "so far" is less than the total
  // number of valid metadatas, then the comma must be rendered.
  //
  int validMetadatas = 0;
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    if ((vec[ix]->name == "value") || (vec[ix]->name == "type"))
    {
      continue;
    }

    ++validMetadatas;
  }


  //
  // And this is pass 2, where the real work is done.
  //
  std::string  out;
  int          renderedMetadatas = 0;
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    if ((vec[ix]->name == "value") || (vec[ix]->name == "type"))
    {
      continue;
    }

    ++renderedMetadatas;
    out += vec[ix]->toJson(renderedMetadatas == validMetadatas);
  }

  if (!isLastElement)
  {
    out += ",";
  }

  return out;
}



/* ****************************************************************************
*
* MetadataVector::check -
*/
std::string MetadataVector::check
(
  ConnectionInfo*     ciP,
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

    if ((res = vec[ix]->check(ciP, requestType, format, indent, predetectedError, counter)) != "OK")
    {
      return res;
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* MetadataVector::present -
*/
void MetadataVector::present(const std::string& metadataType, const std::string& indent)
{
  LM_T(LmtPresent, ("%s%lu %s Metadata%s", 
		    indent.c_str(), 
		    (uint64_t) vec.size(), 
		    metadataType.c_str(), 
		    (vec.size() == 1)? "" : "s"));

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    vec[ix]->present(metadataType, ix, indent + "  ");
  }
}



/* ****************************************************************************
*
* MetadataVector::push_back -
*/
void MetadataVector::push_back(Metadata* item)
{
  vec.push_back(item);
}



/* ****************************************************************************
*
* MetadataVector::operator[] -
*/
Metadata* MetadataVector::operator[] (unsigned int ix) const
{
   if (ix < vec.size())
   {
     return vec[ix];
   }
   return NULL;
}

/* ****************************************************************************
*
* MetadataVector::size -
*/
unsigned int MetadataVector::size(void) const
{
  return vec.size();
}



/* ****************************************************************************
*
* MetadataVector::release -
*/
void MetadataVector::release(void)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    if (vec[ix] != NULL)
    {
      vec[ix]->release();
      delete vec[ix];
      vec[ix] = NULL;
    }
  }

  vec.clear();
}



/* ****************************************************************************
*
* MetadataVector::fill -
*/
void MetadataVector::fill(MetadataVector* mvP)
{
  for (unsigned int ix = 0; ix < mvP->size(); ++ix)
  {
    Metadata* mP = new Metadata((*mvP)[ix]);

    push_back(mP);
  }
}



/* ****************************************************************************
*
* MetadataVector::lookupByName - 
*/
Metadata* MetadataVector::lookupByName(const std::string& _name)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    if (vec[ix]->name == _name)
    {
      return vec[ix];
    }
  }

  return NULL;
}
