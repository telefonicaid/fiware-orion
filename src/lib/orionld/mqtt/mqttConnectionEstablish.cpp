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

#include "orionld/mqtt/mqttConnectionLookup.h"                 // mqttConnectionLookup
#include "orionld/mqtt/mqttConnectionAdd.h"                    // mqttConnectionAdd
#include "orionld/mqtt/mqttConnectionEstablish.h"              // Own interface



// -----------------------------------------------------------------------------
//
// mqttConnectionEstablish - MOVE to its own module, perhaps even its own library - src/lib/orionld/mqtt
//
bool mqttConnectionEstablish(bool mqtts, const char* username, const char* password, const char* host, unsigned short port)
{
  if (mqttConnectionLookup(host, port) != NULL)
    return true;  // Already connected

  if (mqttConnectionAdd(mqtts, username, password, host, port) == NULL)
    return false;

  return true;
}
