/*
*
* Copyright 2019 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
#include <uuid/uuid.h>                                         // uuid_t, uuid_generate_time_safe, uuid_unparse_lower

#include "logMsg/logMsg.h"                                     // Log library
#include "logMsg/traceLevels.h"                                // LMT_*

#include "orionld/common/uuidGenerate.h"                       // Own interface



// -----------------------------------------------------------------------------
//
// uuidGenerate -
//
void uuidGenerate(char* buf, int bufSize, bool uri)
{
  uuid_t uuid;
  int    bufIx      = 0;
  int    minBufSize = 37 + (uri == true)? 33 : 0;

  if (bufSize < minBufSize)
    LM_X(1, ("Implementation Error (not enough room to generate a UUID (%d bytes needed, %d supplied)", minBufSize, bufSize));

  uuid_generate_time_safe(uuid);

  if (uri == true)
  {
    strncpy(buf, "urn:ngsi-ld:attribute:instance:", bufSize);
    bufIx = 31;
  }

  uuid_unparse_lower(uuid, &buf[bufIx]);
}
