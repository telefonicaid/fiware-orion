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

#include "mongoDriver/OID.h"

namespace orion
{
/* ****************************************************************************
*
* OID::OID -
*/
OID::OID()
{
}



/* ****************************************************************************
*
* OID::OID -
*/
OID::OID(const std::string& id)
{
  oid = mongo::OID(id);
}



/* ****************************************************************************
*
* OID::init -
*/
void OID::init(void)
{
  oid.init();
}



/* ****************************************************************************
*
* OID::toString -
*/
std::string OID::toString(void)
{
  return oid.toString();
}



///////// from now on, only methods with low-level driver types in return or parameters /////////



/* ****************************************************************************
*
* OID::get -
*/
mongo::OID OID::get(void) const
{
  return oid;
}



/* ****************************************************************************
*
* OID::OID -
*/
OID::OID(const mongo::OID& _oid)
{
  oid = _oid;
}
}
