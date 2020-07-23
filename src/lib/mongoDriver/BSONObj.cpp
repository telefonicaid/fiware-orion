/*
*
* Copyright 2020 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermín Galán
*/

#include <string>
#include <map>

#include "mongoDriver/BSONObj.h"

namespace orion
{
/* ****************************************************************************
*
* BSONObj::BSONObj -
*/
BSONObj::BSONObj()
{
}



/* ****************************************************************************
*
* BSONObj::getFieldNames -
*/
int BSONObj::getFieldNames(std::set<std::string>& fields) const
{
  return bo.getFieldNames(fields);
}



/* ****************************************************************************
*
* BSONObj::hasField -
*/
bool BSONObj::hasField(const std::string& field) const
{
  return bo.hasField(field);
}



/* ****************************************************************************
*
* BSONObj::nFields -
*/
int BSONObj::nFields(void) const
{
  return bo.nFields();
}



/* ****************************************************************************
*
* BSONObj::toString -
*/
std::string BSONObj::toString(void) const
{
  return bo.toString();
}



/* ****************************************************************************
*
* BSONObj::isEmpty -
*/
bool BSONObj::isEmpty(void)
{
  return bo.isEmpty();
}



/* ****************************************************************************
*
* BSONObj::toStringMap -
*/
void BSONObj::toStringMap(std::map<std::string, std::string>* m)
{
  for (mongo::BSONObj::iterator i = bo.begin(); i.more();)
  {
    mongo::BSONElement e = i.next();

    (*m)[e.fieldName()] = e.String();
  }
}



/* ****************************************************************************
*
* BSONObj::toElementsVector -
*/
void BSONObj::toElementsVector(std::vector<BSONElement>* v)
{
  for (mongo::BSONObj::iterator i = bo.begin(); i.more();)
  {
    BSONElement e(i.next());
    v->push_back(e);
  }
}



///////// from now on, only methods with low-level driver types in return or parameters /////////



/* ****************************************************************************
*
* BSONObj::BSONObj -
*/
BSONObj::BSONObj(const mongo::BSONObj& _bo)
{
  bo = _bo;
}



/* ****************************************************************************
*
* BSONObj::get -
*/
mongo::BSONObj BSONObj::get(void) const
{
  return bo;
}
}
