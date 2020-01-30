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
#include <vector>

#include "mongoDriver/BSONElement.h"

namespace orion
{
/* ****************************************************************************
*
* BSONElement::BSONElement -
*/
BSONElement::BSONElement(void)
{
}



/* ****************************************************************************
*
* BSONElement::OID -
*/
std::string BSONElement::OID(void)
{
  return be.OID().toString();
}



/* ****************************************************************************
*
* BSONElement::String -
*/
std::string BSONElement::String(void)
{
  return String();
}



/* ****************************************************************************
*
* BSONElement::Array -
*/
std::vector<BSONElement> BSONElement::Array(void)
{
  std::vector<BSONElement> v;

  std::vector<mongo::BSONElement> bea = be.Array();
  for (unsigned int ix = 0; ix < bea.size(); ++ix)
  {
    v.push_back(BSONElement(bea[ix]));
  }
  return v;
}



/* ****************************************************************************
*
* BSONElement::embeddedObject -
*/
BSONObj BSONElement::embeddedObject(void)
{
  return BSONObj(be.embeddedObject());
}



/* ****************************************************************************
*
* BSONElement::fieldName -
*/
std::string BSONElement::fieldName(void)
{
  return be.fieldName();
}



/* ****************************************************************************
*
* BSONElement::eoo -
*/
bool BSONElement::eoo(void) const
{
  return be.eoo();
}



///////// from now on, only methods with low-level driver types in return or parameters /////////



/* ****************************************************************************
*
* BSONElement::BSONElement -
*/
BSONElement::BSONElement(const mongo::BSONElement& _be)
{
  be = _be;
}



/* ****************************************************************************
*
* BSONElement::get -
*/
mongo::BSONElement BSONElement::get(void) const
{
  return be;
}
}
