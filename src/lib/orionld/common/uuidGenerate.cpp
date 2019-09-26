/*
*
* Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <uuid/uuid.h>                                         // uuid_t, uuid_generate_time_safe, uuid_unparse_lower

#include "orionld/common/uuidGenerate.h"                       // Own interface



// -----------------------------------------------------------------------------
//
// uuidGenerate -
//
void uuidGenerate(char* buf)
{
  uuid_t uuid;

  uuid_generate_time_safe(uuid);
  uuid_unparse_lower(uuid, buf);
}
