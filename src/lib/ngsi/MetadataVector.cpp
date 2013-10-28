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
#include <vector>

#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/MetadataVector.h"

/* ****************************************************************************
*
* MetadataVector::MetadataVector -
*/
MetadataVector::MetadataVector(std::string _tag) {
  LM_T(LmtMetadataDoubleFree, ("MetadataVector is created at %p", this));
  vec.clear();
  tag = _tag;
}

/* ****************************************************************************
*
* MetadataVector::tagSet -
*/
void MetadataVector::tagSet(std::string tagName)
{
  tag = tagName;
}


/* ****************************************************************************
*
* MetadataVector::render - 
*/
std::string MetadataVector::render(Format format, std::string indent, bool comma)
{
  std::string out = "";

  if (vec.size() == 0)
    return "";

  out += startTag(indent, tag, format);
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
    out += vec[ix]->render(format, indent + "  ");
  out += endTag(indent, tag, format, comma);


  return out;
}



/* ****************************************************************************
*
* MetadataVector::check - 
*/
std::string MetadataVector::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
     std::string res;

     if ((res = vec[ix]->check(requestType, format, indent, predetectedError, counter)) != "OK")
       return res;
  }

  return "OK";
}



/* ****************************************************************************
*
* MetadataVector::present - 
*/
void MetadataVector::present(std::string metadataType, std::string indent)
{
   PRINTF("%lu %s Metadata%s\n", (unsigned long) vec.size(), metadataType.c_str(), (vec.size() == 1)? "" : "s");

   for (unsigned int ix = 0; ix < vec.size(); ++ix)
     vec[ix]->present(metadataType, ix, indent);
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
* MetadataVector::get - 
*/
Metadata* MetadataVector::get(int ix)
{
  return vec[ix];
}



/* ****************************************************************************
*
* MetadataVector::size - 
*/
unsigned int MetadataVector::size(void)
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
    LM_T(LmtRelease, ("Releasing Metadata %d", ix));
    vec[ix]->release();
    delete vec[ix];
  }

  vec.clear();
}
