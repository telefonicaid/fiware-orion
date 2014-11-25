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

  out += startTag(indent, tag, jsonTag, format, true);
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    out += vec[ix]->render(format, indent + "  ", ix != vec.size() - 1);
  }
  out += endTag(indent, tag, format, comma, true);


  return out;
}



/* ****************************************************************************
*
* MetadataVector::check -
*/
std::string MetadataVector::check
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

    if ((res = vec[ix]->check(requestType, format, indent, predetectedError, counter)) != "OK")
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
  PRINTF("%lu %s Metadata%s\n", (uint64_t) vec.size(), metadataType.c_str(), (vec.size() == 1)? "" : "s");

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    vec[ix]->present(metadataType, ix, indent);
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



/* ****************************************************************************
*
* MetadataVector::fill -
*/
void MetadataVector::fill(MetadataVector* mvP)
{
  for (unsigned int ix = 0; ix < mvP->size(); ++ix)
  {
    Metadata* mP = new Metadata(mvP->get(ix));

    push_back(mP);
  }
}
