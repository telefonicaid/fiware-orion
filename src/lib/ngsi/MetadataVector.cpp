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
#include "common/string.h"
#include "common/JsonHelper.h"
#include "ngsi/MetadataVector.h"

#include "mongoBackend/dbFieldEncoding.h"

/* ****************************************************************************
*
* MetadataVector::MetadataVector -
*/
MetadataVector::MetadataVector(void)
{
  vec.clear();
}



/* ****************************************************************************
*
* MetadataVector::render -
*
* FIXME P5: this method doesn't depend on the class object. Should be moved out of the class?
*/
std::string MetadataVector::render(const std::vector<Metadata*>& orderedMetadata, bool comma)
{
  std::string out = "";

  if (orderedMetadata.size() == 0)
  {
    return "";
  }

  out += startTag("metadatas", true);
  for (unsigned int ix = 0; ix < orderedMetadata.size(); ++ix)
  {
    out += orderedMetadata[ix]->render(ix != orderedMetadata.size() - 1);
  }
  out += endTag(comma, true);


  return out;
}



/* ****************************************************************************
*
* MetadataVector::matchFilter -
*
*/
bool MetadataVector::matchFilter(const std::string& mdName, const std::vector<std::string>& metadataFilter)
{
  /* Metadata filtering only in the case of actual metadata vector not containing "*" */
  if ((metadataFilter.size() == 0) || (std::find(metadataFilter.begin(), metadataFilter.end(), NGSI_MD_ALL) != metadataFilter.end()))
  {
    return true;
  }

  return std::find(metadataFilter.begin(), metadataFilter.end(), mdName) != metadataFilter.end();
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
*
*/
std::string MetadataVector::toJson(const std::vector<Metadata*>& orderedMetadata)
{
  JsonHelper jh;

  for (unsigned int ix = 0; ix < orderedMetadata.size(); ++ix)
  {
    jh.addRaw(orderedMetadata[ix]->name, orderedMetadata[ix]->toJson());
  }

  return jh.str();
}



/* ****************************************************************************
*
* MetadataVector::check -
*/
std::string MetadataVector::check(ApiVersion apiVersion)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    std::string res;

    if ((res = vec[ix]->check(apiVersion)) != "OK")
    {
      return res;
    }
  }

  return "OK";
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
    if (dbDotEncode(vec[ix]->name) == _name)
    {
      return vec[ix];
    }
  }

  return NULL;
}
