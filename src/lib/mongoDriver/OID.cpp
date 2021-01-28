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


/* ****************************************************************************
*
* FAILBACK_OID constant
*
* 24 zeros
*
*/
#define FAILBACK_OID "000000000000000000000000"


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
  // If the id is not valid (i.e. an empty string) we use a failback id
  if (bson_oid_is_valid(id.c_str(), strlen(id.c_str())))
  {
    bson_oid_init_from_string(&oid, id.c_str());
  }
  else
  {
    bson_oid_init_from_string(&oid, FAILBACK_OID);
  }
}



/* ****************************************************************************
*
* OID::init -
*/
void OID::init(void)
{
  bson_oid_init(&oid, NULL);
}



/* ****************************************************************************
*
* OID::toString -
*/
std::string OID::toString(void)
{
  char str[25];  // OID fixed length is 24 chars
  bson_oid_to_string(&oid, str);
  return std::string(str);
}



///////// from now on, only methods with low-level driver types in return or parameters /////////



/* ****************************************************************************
*
* OID::get -
*/
bson_oid_t OID::get(void) const
{
  return oid;
}



}
