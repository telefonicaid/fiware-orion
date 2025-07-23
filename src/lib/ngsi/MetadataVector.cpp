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
* MetadataVector::toJson -
*
*/
std::string MetadataVector::toJson(const std::vector<Metadata*>& orderedMetadata)
{
  JsonObjectHelper jh;

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
std::string MetadataVector::check(void)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    std::string res;

    if ((res = vec[ix]->check()) != "OK")
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
* MetadataVector::lookupByName - 
*/
Metadata* MetadataVector::lookupByName(const std::string& _name) const
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    if (dbEncode(vec[ix]->name) == _name)
    {
      return vec[ix];
    }
  }

  return NULL;
}



/* ****************************************************************************
*
* MetadataVector::toBson -
*/
void MetadataVector::toBson(orion::BSONObjBuilder* md, orion::BSONArrayBuilder* mdNames)
{
  for (unsigned int ix = 0; ix < this->vec.size(); ++ix)
  {
    this->vec[ix]->appendToBson(md, mdNames);

    LM_T(LmtMongo, ("new custom metadata: {name: %s, type: %s, value: %s}",
                      this->vec[ix]->name.c_str(), this->vec[ix]->type.c_str(), this->vec[ix]->toJson().c_str()));
  }
}
