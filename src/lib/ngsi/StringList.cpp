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
#include "ngsi/StringList.h"



/* ****************************************************************************
*
* StringList::fill -
*/
void StringList::fill(const std::vector<std::string>& sVec)
{
  for (unsigned int ix = 0; ix < sVec.size(); ++ix)
  {
    stringV.push_back(sVec[ix]);
  }
}



/* ****************************************************************************
*
* StringList::toJson -
*/
std::string StringList::toJson(void)
{
  JsonVectorHelper jh;

  for (unsigned int ix = 0; ix < stringV.size(); ++ix)
  {
    jh.addString(stringV[ix]);
  }

  return jh.str();
}



/* ****************************************************************************
*
* StringList::release -
*/
void StringList::release(void)
{
 stringV.clear();
}



/* ****************************************************************************
*
* lookup - 
*/
bool StringList::lookup(const std::string& string, const std::string& wildCard) const
{
  bool wildCardUsed = !wildCard.empty();
  for (unsigned int ix = 0; ix < stringV.size(); ++ix)
  {
    if ((stringV[ix] == string) || (wildCardUsed && (stringV[ix] == ALL_ATTRS)))
    {
      return true;
    }
  }

  return false;
}



/* ****************************************************************************
*
* push_back - 
*/
void StringList::push_back(const std::string& string)
{
  stringV.push_back(string);
}



/* ****************************************************************************
*
* push_back_if_absent - 
*/
void StringList::push_back_if_absent(const std::string& string)
{
  if (lookup(string) == false)
  {
    stringV.push_back(string);
  }
}



/* ****************************************************************************
*
* StringList::size -
*/
unsigned int StringList::size(void) const
{
  return stringV.size();
}





/* ****************************************************************************
*
* StringList::clone -
*/
void StringList::clone(const StringList& sList)
{
  for (unsigned int ix = 0; ix < sList.size(); ++ix)
  {
    push_back(sList[ix]);
  }
}

