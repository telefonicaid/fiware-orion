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
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/mqtt/mqttCheck.h"                            // Own interface



// ----------------------------------------------------------------------------
//
// mqttCheck -
//
bool mqttCheck(char* mqtt, char** detailP)
{
  // FIXME: Implement!

  if (!SCOMPARE7(mqtt, 'm', 'q', 't', 't', ':', '/', '/'))
  {
    if (detailP != NULL)
      *detailP = (char*) "protocol doesn't start with 'mqtt://'";
    return false;
  }

  return true;
}
